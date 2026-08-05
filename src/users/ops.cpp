#include "ops.h"

#include <QProcess>

QString sudoRun(const QByteArray &password, const QStringList &args, const QByteArray &input)
{
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    /* -k drops a cached timestamp so a wrong password fails here; -p "" keeps
     * the prompt out of the output. */
    proc.start("sudo", QStringList{ "-S", "-k", "-p", "" } + args);
    if (!proc.waitForStarted(10000))
        return "Could not start sudo";

    proc.write(password);
    proc.write("\n");
    if (!input.isEmpty())
        proc.write(input);
    proc.closeWriteChannel();

    if (!proc.waitForFinished(120000))
        return "sudo timed out";

    const QString out = QString::fromUtf8(proc.readAll()).trimmed();
    if (proc.exitCode() == 0)
        return QString();
    if (out.contains("incorrect password", Qt::CaseInsensitive)
        || out.contains("Sorry, try again", Qt::CaseInsensitive))
        return "sudo rejected the password.";
    return out.isEmpty() ? QString("%1 failed").arg(args.value(0)) : out;
}

QString renameSystemUser(const QByteArray &password, const QString &oldName,
                         const QString &newName)
{
    const QString error = sudoRun(password, { "usermod", "--login", newName, "--home",
                                              "/home/" + newName, "--move-home", oldName });
    if (!error.isEmpty())
        return error;

    /* The primary group usually carries the old name too; not every setup has
     * one, so a failure here is not fatal. */
    sudoRun(password, { "groupmod", "-n", newName, oldName });
    return QString();
}

QString setUserPassword(const QByteArray &password, const QString &user, const QString &secret)
{
    const QByteArray line = (user + ":" + secret + "\n").toUtf8();
    return sudoRun(password, { "chpasswd" }, line);
}
