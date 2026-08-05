#include "flake.h"

#include <QDir>
#include <QHash>
#include <QProcess>

namespace {

QHash<QString, QString> g_paths;

} // namespace

void flakeInputPath(const QString &input, std::function<void(const QString &)> done)
{
    const auto cached = g_paths.constFind(input);
    if (cached != g_paths.constEnd()) {
        done(*cached);
        return;
    }

    const QString repo = QDir::homePath() + "/.nixlyos";
    auto *proc = new QProcess;
    QObject::connect(proc, &QProcess::finished, proc,
                     [proc, input, done](int code, QProcess::ExitStatus) {
                         QString path;
                         if (code == 0)
                             path = QString::fromUtf8(proc->readAllStandardOutput()).trimmed();
                         proc->deleteLater();
                         g_paths.insert(input, path);
                         done(path);
                     });

    /* --impure because the local flake is usually a dirty git tree. */
    proc->start("nix", { "eval", "--raw", "--impure", "--expr",
                         QString("(builtins.getFlake \"%1\").inputs.%2.outPath")
                             .arg(repo, input) });
}
