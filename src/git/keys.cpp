#include "keys.h"

#include <QDir>
#include <QFile>
#include <QProcess>

namespace {

QString sshDir()
{
    return QDir::homePath() + "/.ssh";
}

} // namespace

QString keyPath(const QString &name)
{
    return sshDir() + "/" + name;
}

bool keyExists(const QString &name)
{
    return QFile::exists(keyPath(name));
}

QString generateKey(const QString &name, const QString &comment)
{
    const QString dir = sshDir();
    if (!QDir().mkpath(dir))
        return QString("Could not create %1").arg(dir);
    QFile::setPermissions(dir, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);

    /* ssh-keygen prompts before overwriting, and there is no terminal to
     * answer it — clear the pair first. */
    const QString path = keyPath(name);
    QFile::remove(path);
    QFile::remove(path + ".pub");

    QProcess proc;
    proc.start("ssh-keygen",
               { "-t", "ed25519", "-f", path, "-C", comment, "-N", "" });
    if (!proc.waitForFinished(30000))
        return "ssh-keygen timed out";
    if (proc.exitCode() != 0)
        return QString::fromUtf8(proc.readAllStandardError()).trimmed();
    return QString();
}

QString readPublicKey(const QString &name)
{
    QFile file(keyPath(name) + ".pub");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(file.readAll()).trimmed();
}
