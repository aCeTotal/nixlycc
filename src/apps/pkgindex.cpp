#include "pkgindex.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTextStream>
#include <algorithm>

/* ── Paths ───────────────────────────────────────────────────────── */

/* NIX_PATH carries the nixpkgs the user's system actually evaluates; the root
 * channel is the fallback for setups that don't export it. */
static QString nixpkgsPath()
{
    const QString nixPath = qEnvironmentVariable("NIX_PATH");
    for (const QString &part : nixPath.split(':', Qt::SkipEmptyParts)) {
        if (part.startsWith("nixpkgs="))
            return part.mid(8);
    }
    const QString channel = "/nix/var/nix/profiles/per-user/root/channels/nixos";
    return QFile::exists(channel) ? channel : QString();
}

static QString cachePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/packages.tsv";
}

static QString nixSystem()
{
    return QSysInfo::currentCpuArchitecture() + "-linux";
}

/* ── Entry helpers ───────────────────────────────────────────────── */

/* nix-env reports "firefox-133.0.3"; the version starts at the last dash that
 * is followed by a digit. */
static void splitVersion(const QString &name, QString &pname, QString &version)
{
    for (int i = name.size() - 2; i > 0; --i) {
        if (name[i] == '-' && name[i + 1].isDigit()) {
            pname = name.left(i);
            version = name.mid(i + 1);
            return;
        }
    }
    pname = name;
    version.clear();
}

static void addPkg(std::vector<Pkg> &out, const QString &attr, const QString &name,
                   const QString &desc, bool nixly)
{
    Pkg p;
    p.attr = attr;
    splitVersion(name, p.pname, p.version);
    p.desc = desc;
    p.nixly = nixly;
    p.attrLower = attr.toLower().toUtf8();
    p.descLower = desc.toLower().toUtf8();
    out.push_back(std::move(p));
}

/* ── Load ────────────────────────────────────────────────────────── */

void PkgIndex::load(std::function<void(const QString &)> onStatus,
                    std::function<void()> onReady)
{
    m_onStatus = std::move(onStatus);
    m_onReady = std::move(onReady);
    m_stamp = QFileInfo(nixpkgsPath()).canonicalFilePath();

    if (readCache()) {
        finish();
        return;
    }
    startBuild();
}

bool PkgIndex::readCache()
{
    QFile file(cachePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    /* First line stamps which nixpkgs the cache was built from. */
    const QByteArray header = file.readLine().trimmed();
    if (header.isEmpty() || header.mid(1) != m_stamp.toUtf8())
        return false;

    m_pkgs.reserve(80000);
    while (!file.atEnd()) {
        const QByteArray line = file.readLine();
        const QList<QByteArray> f = line.trimmed().split('\t');
        if (f.size() < 5)
            continue;
        Pkg p;
        p.attr = QString::fromUtf8(f[0]);
        p.pname = QString::fromUtf8(f[1]);
        p.version = QString::fromUtf8(f[2]);
        p.desc = QString::fromUtf8(f[3]);
        p.nixly = (f[4] == "y");
        p.attrLower = p.attr.toLower().toUtf8();
        p.descLower = p.desc.toLower().toUtf8();
        m_pkgs.push_back(std::move(p));
    }
    return !m_pkgs.empty();
}

void PkgIndex::writeCache() const
{
    const QString path = cachePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << "#" << m_stamp << "\n";
    for (const Pkg &p : m_pkgs) {
        out << p.attr << "\t" << p.pname << "\t" << p.version << "\t"
            << p.desc << "\t" << (p.nixly ? "y" : "n") << "\n";
    }
    out.flush();
    file.commit();
}

/* ── Build ───────────────────────────────────────────────────────── */

/* `nix-env -qaP --description` prints three space-padded columns: attribute
 * path, name-version, description. Neither of the first two contains a space,
 * so the split is unambiguous. */
void PkgIndex::startBuild()
{
    const QString nixpkgs = nixpkgsPath();
    if (nixpkgs.isEmpty()) {
        if (m_onStatus)
            m_onStatus("No nixpkgs found — set NIX_PATH or add the nixos channel.");
        return;
    }

    if (m_onStatus)
        m_onStatus("Indexing nixpkgs… (once, about 20 seconds)");

    auto *proc = new QProcess;
    QObject::connect(proc, &QProcess::finished, proc, [this, proc](int code, QProcess::ExitStatus) {
        const QByteArray out = proc->readAllStandardOutput();
        proc->deleteLater();
        if (code != 0 && out.isEmpty()) {
            if (m_onStatus)
                m_onStatus("Could not enumerate nixpkgs.");
            return;
        }

        m_pkgs.reserve(80000);
        for (const QByteArray &line : out.split('\n')) {
            const int a = line.indexOf(' ');
            if (a <= 0)
                continue;
            int b = a;
            while (b < line.size() && line[b] == ' ')
                ++b;
            int c = line.indexOf(' ', b);
            if (c < 0)
                c = line.size();
            addPkg(m_pkgs, QString::fromUtf8(line.left(a)),
                   QString::fromUtf8(line.mid(b, c - b)),
                   QString::fromUtf8(line.mid(c).trimmed()), false);
        }
        runNixlyEval();
    });

    proc->start("nix-env", {"-qaP", "--description", "-f", nixpkgs});
}

/* nixlypkgs only publishes a handful of packages, and forcing their meta
 * currently fails on the published flake, so names are all we ask for. */
void PkgIndex::runNixlyEval()
{
    if (m_onStatus)
        m_onStatus("Indexing nixlypkgs…");

    auto *proc = new QProcess;
    QObject::connect(proc, &QProcess::finished, proc, [this, proc](int code, QProcess::ExitStatus) {
        if (code == 0) {
            const QJsonDocument doc = QJsonDocument::fromJson(proc->readAllStandardOutput());
            for (const QJsonValue &v : doc.array()) {
                const QString attr = v.toString();
                if (attr.isEmpty() || attr == "default")
                    continue;
                addPkg(m_pkgs, attr, attr, "nixlypkgs package", true);
            }
        }
        proc->deleteLater();
        writeCache();
        finish();
    });

    proc->start("nix", {"eval", "--json",
                        "github:aCeTotal/nixlypkgs#packages." + nixSystem(),
                        "--apply", "builtins.attrNames"});
}

void PkgIndex::finish()
{
    m_ready = true;
    if (m_onStatus)
        m_onStatus(QString("%1 packages indexed").arg(m_pkgs.size()));
    if (m_onReady)
        m_onReady();
}

QStringList PkgIndex::iconKeys() const
{
    QSet<QString> keys;
    keys.reserve(m_pkgs.size() * 2);
    for (const Pkg &p : m_pkgs) {
        const QString attr = QString::fromUtf8(p.attrLower);
        keys.insert(attr);
        keys.insert(p.pname.toLower());
        const int dot = attr.lastIndexOf('.');
        if (dot > 0 && dot < attr.size() - 1)
            keys.insert(attr.mid(dot + 1));
    }
    return QStringList(keys.begin(), keys.end());
}

/* ── Search ──────────────────────────────────────────────────────── */

std::vector<const Pkg *> PkgIndex::search(const QString &query, int limit) const
{
    std::vector<const Pkg *> out;
    const QByteArray q = query.trimmed().toLower().toUtf8();
    if (q.isEmpty())
        return out;

    struct Hit { const Pkg *p; int score; int len; };
    std::vector<Hit> hits;
    hits.reserve(4096);

    for (const Pkg &p : m_pkgs) {
        int score;
        const int pos = p.attrLower.indexOf(q);
        if (pos == 0) {
            score = (p.attrLower.size() == q.size()) ? 0 : 1;
        } else if (pos > 0) {
            const char prev = p.attrLower[pos - 1];
            score = (prev == '-' || prev == '.' || prev == '_') ? 2 : 3;
        } else if (p.descLower.contains(q)) {
            score = 4;
        } else {
            continue;
        }
        /* Own packages outrank equally-matched nixpkgs entries. */
        if (p.nixly && score > 0)
            --score;
        hits.push_back({&p, score, (int)p.attrLower.size()});
    }

    const int n = std::min((int)hits.size(), limit);
    std::partial_sort(hits.begin(), hits.begin() + n, hits.end(),
                      [](const Hit &a, const Hit &b) {
                          if (a.score != b.score) return a.score < b.score;
                          if (a.len != b.len) return a.len < b.len;
                          return a.p->attr < b.p->attr;
                      });

    out.reserve(n);
    for (int i = 0; i < n; ++i)
        out.push_back(hits[i].p);
    return out;
}
