#include "scan.h"
#include "../apps/flake.h"
#include "../apps/pkglist.h"

#include <QDir>
#include <QFile>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSysInfo>
#include <algorithm>
#include <memory>

namespace {

struct Info {
    QString version;
    QString changelog;
    QString source;
};

using InfoMap = QHash<QString, Info>;

/* Everything the nested async steps below share. */
struct ScanState {
    std::function<void(const QString &)> status;
    std::function<void(const QList<UpdateEntry> &, const QString &)> done;
    QStringList stable;
    QStringList unstable;
    QString curNixpkgs, curUnstable, curNixly; /* what the system is built from */
    QString newNixpkgs, newUnstable, newNixly; /* the freshly prefetched trees */
    InfoMap curStableInfo, newStableInfo, curUnstableInfo, newUnstableInfo;
};

void runProc(const QString &program, const QStringList &args,
             std::function<void(int, const QByteArray &)> done)
{
    auto *proc = new QProcess;
    QObject::connect(proc, &QProcess::finished, proc,
                     [proc, done](int code, QProcess::ExitStatus) {
                         const QByteArray out = proc->readAllStandardOutput();
                         proc->deleteLater();
                         done(code, out);
                     });
    proc->start(program, args);
}

/* The URL an input was locked from, e.g. "github:NixOS/nixpkgs/nixos-26.05" —
 * prefetching that URL again is how the newest revision is found. */
QString lockUrl(const QString &input)
{
    QFile file(QDir::homePath() + "/.nixlyos/flake.lock");
    if (!file.open(QIODevice::ReadOnly))
        return QString();
    const QJsonObject nodes =
        QJsonDocument::fromJson(file.readAll()).object().value("nodes").toObject();
    const QString key =
        nodes.value("root").toObject().value("inputs").toObject().value(input).toString();
    const QJsonObject orig = nodes.value(key).toObject().value("original").toObject();
    if (orig.value("type").toString() == "github") {
        QString url =
            "github:" + orig.value("owner").toString() + "/" + orig.value("repo").toString();
        const QString ref = orig.value("ref").toString();
        return ref.isEmpty() ? url : url + "/" + ref;
    }
    return orig.value("url").toString();
}

void prefetch(const QString &url, std::function<void(const QString &)> done)
{
    if (url.isEmpty()) {
        done(QString());
        return;
    }
    runProc("nix", { "flake", "prefetch", "--json", "--refresh", url },
            [done](int code, const QByteArray &out) {
                if (code != 0) {
                    done(QString());
                    return;
                }
                done(QJsonDocument::fromJson(out).object().value("storePath").toString());
            });
}

/* attr → "version \t changelog-url \t source-url" for every attribute,
 * evaluated against one nixpkgs tree. The nixlypkgs overlay is applied when a
 * flake path is given, because packages.nix installs nixly packages from the
 * same pkgs scope. tryEval keeps a package that throws — insecure, broken —
 * from taking the whole batch down. */
QString infoExpr(const QString &nixpkgs, const QString &nixlyFlake, const QStringList &attrs)
{
    QString list;
    for (const QString &attr : attrs)
        list += " \"" + attr + "\"";
    const QString overlays =
        nixlyFlake.isEmpty()
            ? QString("[ ]")
            : QString("[ (builtins.getFlake \"%1\").overlays.default ]").arg(nixlyFlake);

    return QString(R"(let
  p = import %1 { system = "%2"; config = { allowUnfree = true; }; overlays = %3; };
  parts = s: builtins.filter builtins.isString (builtins.split "\\." s);
  getPath = v: path:
    if path == [] then v
    else if builtins.isAttrs v && builtins.hasAttr (builtins.head path) v
      then getPath (builtins.getAttr (builtins.head path) v) (builtins.tail path)
      else null;
  srcUrl = d:
    let s = d.src or null;
    in if s == null then toString (d.meta.homepage or "")
       else toString (s.meta.homepage or (builtins.head (s.urls or [ (s.url or (d.meta.homepage or "")) ])));
  info = a:
    let r = builtins.tryEval (
      let d = getPath p (parts a);
      in if d == null then ""
         else toString (d.version or "") + "\t" + toString (d.meta.changelog or "") + "\t" + srcUrl d);
    in if r.success then r.value else "";
in builtins.listToAttrs (map (a: { name = a; value = info a; }) [%4 ]))")
        .arg(nixpkgs, QSysInfo::currentCpuArchitecture() + "-linux", overlays, list);
}

void evalInfo(const QString &nixpkgs, const QString &nixlyFlake, const QStringList &attrs,
              std::function<void(const InfoMap &)> done)
{
    if (nixpkgs.isEmpty() || attrs.isEmpty()) {
        done(InfoMap());
        return;
    }
    /* --impure: getFlake on a store path and an expression naming store paths
     * are both rejected in pure mode. */
    runProc("nix", { "eval", "--json", "--impure", "--expr", infoExpr(nixpkgs, nixlyFlake, attrs) },
            [done](int code, const QByteArray &out) {
                InfoMap map;
                if (code == 0) {
                    const QJsonObject obj = QJsonDocument::fromJson(out).object();
                    for (auto it = obj.begin(); it != obj.end(); ++it) {
                        const QStringList f = it.value().toString().split('\t');
                        if (f.size() == 3)
                            map.insert(it.key(), { f[0], f[1], f[2] });
                    }
                }
                done(map);
            });
}

void diffInto(const QStringList &attrs, const InfoMap &cur, const InfoMap &fresh,
              QList<UpdateEntry> &out)
{
    for (const QString &attr : attrs) {
        const Info oldInfo = cur.value(attr);
        const Info newInfo = fresh.value(attr);
        if (oldInfo.version.isEmpty() || newInfo.version.isEmpty()
            || oldInfo.version == newInfo.version)
            continue;
        UpdateEntry e;
        e.attr = attr;
        e.oldVersion = oldInfo.version;
        e.newVersion = newInfo.version;
        e.changelog = newInfo.changelog.isEmpty() ? oldInfo.changelog : newInfo.changelog;
        e.source = newInfo.source.isEmpty() ? oldInfo.source : newInfo.source;
        out.append(e);
    }
}

void finishScan(const std::shared_ptr<ScanState> &st)
{
    QList<UpdateEntry> out;
    diffInto(st->stable, st->curStableInfo, st->newStableInfo, out);
    diffInto(st->unstable, st->curUnstableInfo, st->newUnstableInfo, out);
    std::sort(out.begin(), out.end(),
              [](const UpdateEntry &a, const UpdateEntry &b) { return a.attr < b.attr; });
    st->done(out, QString());
}

} // namespace

void scanForUpdates(std::function<void(const QString &)> onStatus,
                    std::function<void(const QList<UpdateEntry> &, const QString &)> done)
{
    auto st = std::make_shared<ScanState>();
    st->status = std::move(onStatus);
    st->done = std::move(done);
    for (const PkgEntry &e : installedPackages())
        (e.unstable ? st->unstable : st->stable) << e.attr;
    if (st->stable.isEmpty() && st->unstable.isEmpty()) {
        st->done({}, "No packages found in packages.nix.");
        return;
    }

    /* Each step feeds the next; the indentation is kept flat because the
     * chain is purely sequential. */
    st->status("Locating current package sources…");
    flakeInputPath("nixpkgs", [st](const QString &path) {
    st->curNixpkgs = path;
    flakeInputPath("nixpkgs-unstable", [st](const QString &path) {
    st->curUnstable = path;
    flakeInputPath("nixlypkgs", [st](const QString &path) {
    st->curNixly = path;
    if (st->curNixpkgs.isEmpty()) {
        st->done({}, "Could not locate the flake's nixpkgs input — is ~/.nixlyos a flake?");
        return;
    }

    st->status("Fetching the latest package sources…");
    prefetch(lockUrl("nixpkgs"), [st](const QString &path) {
    st->newNixpkgs = path;
    prefetch(lockUrl("nixpkgs-unstable"), [st](const QString &path) {
    st->newUnstable = path;
    prefetch(lockUrl("nixlypkgs"), [st](const QString &path) {
    st->newNixly = path;
    if (st->newNixpkgs.isEmpty()) {
        st->done({}, "Could not fetch the latest nixpkgs — check the network connection.");
        return;
    }

    st->status("Comparing package versions… (this takes a moment)");
    evalInfo(st->curNixpkgs, st->curNixly, st->stable, [st](const InfoMap &m) {
    st->curStableInfo = m;
    evalInfo(st->newNixpkgs, st->newNixly.isEmpty() ? st->curNixly : st->newNixly, st->stable,
             [st](const InfoMap &m) {
    st->newStableInfo = m;
    evalInfo(st->curUnstable, QString(), st->unstable, [st](const InfoMap &m) {
    st->curUnstableInfo = m;
    evalInfo(st->newUnstable.isEmpty() ? st->curUnstable : st->newUnstable, QString(),
             st->unstable, [st](const InfoMap &m) {
    st->newUnstableInfo = m;
    finishScan(st);
    }); }); }); }); }); }); }); }); }); });
}
