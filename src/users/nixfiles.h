#pragma once

#include <QString>
#include <QStringList>

/* Every .nix file under ~/.nixlyos that names this user as a whole word —
 * users.users.<name>, /home/<name>, the home-manager entry in flake.nix, and
 * anything else that hard-codes it. Shown before a rename so nothing is
 * rewritten behind your back. */
QStringList filesMentioning(const QString &user);

/* Renames the user in all of those files. Whole-word only, so totalvim
 * survives a rename of total. Returns an empty string on success. */
QString renameUserInFiles(const QString &oldName, const QString &newName);

/* Declares a new user in users.nix and gives it the same home-manager
 * configuration as the existing users in flake.nix. */
QString addUserToFiles(const QString &name, const QString &description, bool admin);
