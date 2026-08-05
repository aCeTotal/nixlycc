#pragma once

#include <QList>
#include <QString>

/* A user as the configuration declares it, plus what the running system knows
 * about it. uid is -1 for an account that has been written to the module but
 * not yet built. */
struct Account {
    QString name;
    QString description;
    bool admin = false;
    int uid = -1;
    QString home;
};

/* ~/.nixlyos/modules/core/users.nix — where users.users.<name> lives. */
QString usersModulePath();

/* Every user declared in that module, with /etc/passwd filled in. */
QList<Account> declaredAccounts();
