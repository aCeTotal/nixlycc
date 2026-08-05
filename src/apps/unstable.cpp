#include "unstable.h"
#include "flake.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSysInfo>

namespace {

QHash<QString, QString> g_versions;

/* Walks the attribute path so "python3Packages.requests" resolves, and keeps
 * a package that throws on evaluation — insecure, unfree, broken — from
 * taking the whole batch down. */
QString versionExpr(const QString &nixpkgs, const QStringList &attrs)
{
    QString list;
    for (const QString &attr : attrs)
        list += " \"" + attr + "\"";

    return QString(R"(let
  p = import %1 { system = "%2"; config = { allowUnfree = true; }; };
  parts = s: builtins.filter builtins.isString (builtins.split "\\." s);
  getPath = v: path:
    if path == [] then v
    else if builtins.isAttrs v && builtins.hasAttr (builtins.head path) v
      then getPath (builtins.getAttr (builtins.head path) v) (builtins.tail path)
      else null;
  ver = s:
    let r = builtins.tryEval (
      let v = getPath p (parts s); in if v == null then "" else toString (v.version or ""));
    in if r.success then r.value else "";
in builtins.listToAttrs (map (s: { name = s; value = ver s; }) [%3 ]))")
        .arg(nixpkgs, QSysInfo::currentCpuArchitecture() + "-linux", list);
}

} // namespace

void unstableVersions(const QStringList &attrs,
                      std::function<void(const QHash<QString, QString> &)> done)
{
    QStringList missing;
    for (const QString &attr : attrs) {
        if (!g_versions.contains(attr))
            missing << attr;
    }
    if (missing.isEmpty()) {
        done(g_versions);
        return;
    }

    flakeInputPath("nixpkgs-unstable", [missing, done](const QString &nixpkgs) {
        if (nixpkgs.isEmpty()) {
            done(g_versions);
            return;
        }

        auto *proc = new QProcess;
        QObject::connect(proc, &QProcess::finished, proc,
                         [proc, done](int code, QProcess::ExitStatus) {
                             if (code == 0) {
                                 const QJsonObject obj =
                                     QJsonDocument::fromJson(proc->readAllStandardOutput()).object();
                                 for (auto it = obj.begin(); it != obj.end(); ++it)
                                     g_versions.insert(it.key(), it.value().toString());
                             }
                             proc->deleteLater();
                             done(g_versions);
                         });
        /* --impure: an expression on the command line may not name a store
         * path in pure mode. */
        proc->start("nix",
                    { "eval", "--json", "--impure", "--expr", versionExpr(nixpkgs, missing) });
    });
}
