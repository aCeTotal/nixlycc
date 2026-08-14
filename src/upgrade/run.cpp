#include "run.h"
#include "../git/rebuild.h"
#include "../git/repo.h"

#include <QProcess>
#include <cmath>

QString upgradeSystem(const QByteArray &password,
                      std::function<void(int, const QString &)> onProgress)
{
    onProgress(2, "Updating flake inputs…");
    QProcess update;
    update.setProcessChannelMode(QProcess::MergedChannels);
    update.start("nix", { "flake", "update", "--flake", repoPath() });
    if (!update.waitForStarted(10000))
        return "Could not start nix flake update";
    update.waitForFinished(-1);
    const QString out = QString::fromUtf8(update.readAll()).trimmed();
    if (update.exitCode() != 0) {
        const QString last = out.section('\n', -1).trimmed();
        return last.isEmpty() ? QString("nix flake update failed") : last;
    }

    onProgress(5, "Rebuilding the system…");
    /* nixos-rebuild gives no machine-readable progress, so the bar creeps
     * asymptotically towards 95% as output lines arrive. */
    int lines = 0;
    const QString error = rebuildSwitch(password, [&](const QString &line) {
        ++lines;
        const int percent = 5 + (int)(90.0 * (1.0 - std::exp(-lines / 120.0)));
        onProgress(percent, line);
    });
    if (!error.isEmpty())
        return error;

    onProgress(100, "Done.");
    return QString();
}
