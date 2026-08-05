#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>

/* Runs `sudo <args>` with the unlock password on stdin. input is written
 * after the password, for commands that read from stdin themselves.
 * Blocking — call it from a worker thread. */
QString sudoRun(const QByteArray &password, const QStringList &args,
                const QByteArray &input = QByteArray());

/* usermod, so the account keeps its uid and its home moves with it. Fails
 * while the user has processes running — the account you are logged in as
 * cannot be renamed from inside its own session. */
QString renameSystemUser(const QByteArray &password, const QString &oldName,
                         const QString &newName);

/* chpasswd. The account has to exist, so this runs after the rebuild. */
QString setUserPassword(const QByteArray &password, const QString &user, const QString &secret);
