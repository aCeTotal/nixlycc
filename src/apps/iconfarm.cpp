#include "iconfarm.h"

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QImage>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPainter>
#include <QProcess>
#include <QSet>
#include <QStandardPaths>
#include <QSvgRenderer>
#include <thread>

namespace {

const int kSize = 64;
const char *kDep11Url =
    "https://ftp.debian.org/debian/dists/sid/main/dep11/icons-64x64.tar.gz";

/* Source tiers. A package's own icon beats Debian's rendition, which beats a
 * themed stand-in; the size/format score decides within a tier. */
const int kTierStore = 3000000;
const int kTierDebian = 2000000;
const int kTierTheme = 1000000;

/* Theme order copied from nixly_launcher (crates/appd/src/icon.rs), so the
 * stand-ins here look like the ones the launcher shows. */
int themeRank(const QString &name)
{
    const QString user = qEnvironmentVariable("XDG_ICON_THEME");
    if (!user.isEmpty() && name == user) return 5;
    if (name == "Adwaita") return 4;
    if (name == "Papirus") return 3;
    if (name == "breeze") return 2;
    return 1;
}

int sizePriority(const QString &name)
{
    if (name == "48x48") return 10;
    if (name == "64x64") return 9;
    if (name == "32x32") return 8;
    if (name == "128x128") return 7;
    if (name == "256x256") return 6;
    if (name == "scalable") return 5;
    if (name == "24x24") return 4;
    if (name == "16x16") return 3;
    return 0;   /* symbolic and odd sizes are skipped entirely */
}

int extPriority(const QString &suffix)
{
    if (suffix == "png") return 30;
    if (suffix == "jpg" || suffix == "jpeg") return 20;
    if (suffix == "svg") return 15;
    return 0;
}

/* Best source file per wanted name. Everything outside `want` is dropped as it
 * is seen, which keeps the walk over 200k store paths cheap. */
struct Index {
    QSet<QString> want;
    QHash<QString, QString> path;
    QHash<QString, int> prio;

    void put(const QString &key, int p, const QString &file)
    {
        if (!want.contains(key) || prio.value(key, -1) >= p)
            return;
        prio[key] = p;
        path[key] = file;
    }

    /* Keys are lowercased (icons ship as "Alacritty.svg" but the package is
     * "alacritty"), and reverse-DNS names are aliased to their last component
     * so "org.xfce.thunar.png" also answers to "thunar". */
    void putStem(const QString &stem, int p, const QString &file)
    {
        const QString key = stem.toLower();
        put(key, p, file);
        const int dot = key.lastIndexOf('.');
        if (dot > 0 && dot < key.size() - 1)
            put(key.mid(dot + 1), p - 1, file);
    }
};

void scanFlatDir(const QString &dir, int base, Index &idx)
{
    const QFileInfoList files = QDir(dir).entryInfoList(QDir::Files);
    for (const QFileInfo &f : files) {
        const int ep = extPriority(f.suffix().toLower());
        if (ep == 0)
            continue;
        idx.putStem(f.completeBaseName(), base + ep, f.absoluteFilePath());
    }
}

/* <root>/<theme>/<size>/<category>/<name>.<ext> — the layout shared by every
 * XDG icon theme and by the icon dirs nix packages install. Packages install
 * into hicolor, so that is the one theme holding real per-package art; every
 * other theme is a catalogue of stand-ins and scores a tier lower. */
void scanThemeRoot(const QString &root, Index &idx)
{
    const QFileInfoList themes = QDir(root).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &theme : themes) {
        const bool own = theme.fileName() == "hicolor";
        const int tier =
            own ? kTierStore : kTierTheme + themeRank(theme.fileName()) * 10000;
        const QFileInfoList sizes =
            QDir(theme.absoluteFilePath()).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &size : sizes) {
            const int sp = sizePriority(size.fileName());
            if (sp == 0)
                continue;
            const QFileInfoList cats =
                QDir(size.absoluteFilePath()).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QFileInfo &cat : cats)
                scanFlatDir(cat.absoluteFilePath(), tier + sp * 100, idx);
        }
    }
}

void scanPrefix(const QString &prefix, Index &idx)
{
    if (QFileInfo::exists(prefix + "/icons"))
        scanThemeRoot(prefix + "/icons", idx);
    if (QFileInfo::exists(prefix + "/pixmaps"))
        scanFlatDir(prefix + "/pixmaps", kTierStore + 100, idx);
}

QStringList xdgRoots()
{
    QStringList roots;
    const QString home = QDir::homePath();
    const QString dataHome = qEnvironmentVariable("XDG_DATA_HOME", home + "/.local/share");
    roots << dataHome << home + "/.local/share";
    for (const QString &d : qEnvironmentVariable("XDG_DATA_DIRS").split(':', Qt::SkipEmptyParts))
        roots << d;
    roots << "/run/current-system/sw/share" << "/usr/share";
    roots.removeDuplicates();
    return roots;
}

void scanStore(Index &idx, bool &sawThemePack)
{
    const QStringList entries =
        QDir("/nix/store").entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Unsorted);
    for (const QString &entry : entries) {
        sawThemePack = sawThemePack || entry.contains("icon-theme");
        scanPrefix("/nix/store/" + entry + "/share", idx);
    }
    for (const QString &root : xdgRoots())
        scanPrefix(root, idx);
}

/* Only reached when the store holds no icon theme at all — one 254 MB fetch
 * buys ~8000 stand-in icons. */
void fetchThemePack(Index &idx)
{
    QProcess nix;
    nix.start("nix", {"build", "--no-link", "--print-out-paths",
                      "nixpkgs#papirus-icon-theme"});
    if (!nix.waitForFinished(-1) || nix.exitCode() != 0)
        return;
    const QString out = QString::fromUtf8(nix.readAllStandardOutput()).trimmed();
    if (!out.isEmpty())
        scanPrefix(out + "/share", idx);
}

QByteArray download(const QString &url)
{
    QNetworkAccessManager nam;
    QNetworkRequest req{QUrl(url)};
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    QEventLoop loop;
    QNetworkReply *reply = nam.get(req);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QByteArray data;
    if (reply->error() == QNetworkReply::NoError)
        data = reply->readAll();
    delete reply;
    return data;
}

/* DEP-11 ships one flat tarball of 64x64 PNGs named <package>_<iconname>.png. */
void scanDebian(const QString &work, Index &idx)
{
    const QByteArray tarball = download(kDep11Url);
    if (tarball.isEmpty())
        return;

    const QString archive = work + "/dep11.tar.gz";
    QFile file(archive);
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.write(tarball);
    file.close();

    const QString out = work + "/dep11";
    QDir().mkpath(out);
    QProcess tar;
    tar.start("tar", {"-xzf", archive, "-C", out});
    const bool ok = tar.waitForFinished(-1) && tar.exitCode() == 0;
    QFile::remove(archive);
    if (!ok)
        return;

    const QFileInfoList files = QDir(out).entryInfoList(QDir::Files);
    for (const QFileInfo &f : files) {
        const QString stem = f.completeBaseName();
        const int sep = stem.indexOf('_');
        if (sep <= 0)
            continue;
        idx.put(stem.left(sep).toLower(), kTierDebian + 30, f.absoluteFilePath());
        idx.putStem(stem.mid(sep + 1), kTierDebian + 29, f.absoluteFilePath());
    }
}

bool bake(const QString &src, const QString &dst)
{
    QImage img;
    if (QFileInfo(src).suffix().compare("svg", Qt::CaseInsensitive) == 0) {
        QSvgRenderer renderer(src);
        if (!renderer.isValid())
            return false;
        img = QImage(kSize, kSize, QImage::Format_ARGB32);
        img.fill(Qt::transparent);
        QPainter p(&img);
        renderer.render(&p);
    } else {
        img = QImageReader(src).read();
        if (img.isNull())
            return false;
        if (img.width() > kSize || img.height() > kSize)
            img = img.scaled(kSize, kSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    return img.save(dst, "PNG");
}

void post(std::function<void()> fn)
{
    QMetaObject::invokeMethod(QCoreApplication::instance(), std::move(fn));
}

void farm(const QStringList &keys, const QString &stamp,
          const std::function<void(const QString &)> &onStatus)
{
    const QString dir = IconFarm::dir();
    const QString stampFile = dir + "/.stamp";

    QFile existing(stampFile);
    if (existing.open(QIODevice::ReadOnly) && existing.readAll().trimmed() == stamp.toUtf8())
        return;
    existing.close();

    auto status = [&onStatus](const QString &text) {
        post([onStatus, text]() { onStatus(text); });
    };

    Index idx;
    idx.want = QSet<QString>(keys.begin(), keys.end());
    /* The icon nixly_launcher falls back to (crates/appd/src/icon.rs). */
    idx.want.insert("application-x-executable");

    status("Farming icons: scanning /nix/store…");
    bool sawThemePack = false;
    scanStore(idx, sawThemePack);

    if (!sawThemePack) {
        status("Farming icons: fetching papirus-icon-theme (254 MB, once)…");
        fetchThemePack(idx);
    }

    QDir().mkpath(dir);
    status("Farming icons: fetching Debian catalogue (7.6 MB)…");
    scanDebian(dir, idx);

    status(QString("Farming icons: baking %1…").arg(idx.path.size()));
    int baked = 0;
    for (auto it = idx.path.constBegin(); it != idx.path.constEnd(); ++it) {
        if (bake(it.value(), IconFarm::fileFor(it.key())))
            ++baked;
    }
    QDir(dir + "/dep11").removeRecursively();

    QFile out(stampFile);
    if (out.open(QIODevice::WriteOnly))
        out.write(stamp.toUtf8());
    status(QString("%1 icons cached").arg(baked));
}

} // namespace

QString IconFarm::dir()
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/icons";
}

QString IconFarm::fileFor(const QString &key)
{
    QString safe = key.toLower();
    for (QChar &c : safe) {
        const bool plain = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
        if (!plain && c != '.' && c != '-' && c != '_' && c != '+')
            c = '_';
    }
    return dir() + "/" + safe + ".png";
}

void IconFarm::run(const QStringList &keys, const QString &stamp,
                   std::function<void(const QString &)> onStatus,
                   std::function<void()> onDone)
{
    std::thread([keys, stamp, onStatus = std::move(onStatus),
                 onDone = std::move(onDone)]() {
        farm(keys, stamp, onStatus);
        post([onDone]() {
            if (onDone)
                onDone();
        });
    }).detach();
}
