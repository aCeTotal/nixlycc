#include "rebuild.h"
#include "repo.h"

#include <QProcess>

QString rebuildSwitch(const QByteArray &password, std::function<void(const QString &)> onLine)
{
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    /* -k drops any cached sudo timestamp so a wrong password fails here
     * instead of silently succeeding; -p "" keeps the prompt out of the log. */
    proc.start("sudo", { "-S", "-k", "-p", "", "nixos-rebuild", "switch", "--flake",
                         repoPath() + "#nixlyos" });
    if (!proc.waitForStarted(10000))
        return "Could not start sudo";

    proc.write(password);
    proc.write("\n");
    proc.closeWriteChannel();

    QString pending;
    QString lastLine;
    auto drain = [&]() {
        pending += QString::fromUtf8(proc.readAll());
        int cut;
        while ((cut = pending.indexOf('\n')) >= 0) {
            const QString line = pending.left(cut).trimmed();
            pending.remove(0, cut + 1);
            if (line.isEmpty())
                continue;
            lastLine = line;
            if (onLine)
                onLine(line);
        }
    };

    while (proc.state() != QProcess::NotRunning) {
        if (proc.waitForReadyRead(500))
            drain();
    }
    proc.waitForFinished(-1);
    drain();

    if (proc.exitCode() == 0)
        return QString();
    if (lastLine.contains("incorrect password", Qt::CaseInsensitive)
        || lastLine.contains("Sorry, try again", Qt::CaseInsensitive))
        return "sudo rejected the password.";
    return lastLine.isEmpty() ? QString("nixos-rebuild switch failed") : lastLine;
}
