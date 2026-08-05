#pragma once

#include <QString>

/* ~/.nixlyos/modules/user/git.nix — the home-manager module that owns the git
 * identity and ~/.ssh/config. Both files are read-only symlinks into the nix
 * store, so this module is the only place they can be changed from. */
QString modulePath();

/* Current value of a field in the module, so the UI can prefill from what is
 * actually deployed. Empty when the module has no such entry. */
QString moduleName();
QString moduleEmail();
QString moduleHost();
QString moduleKey();

/* Rewrites the module from these four values. Returns an empty string on
 * success, else the error text. Takes effect only after a rebuild. */
QString writeModule(const QString &name, const QString &email, const QString &host,
                    const QString &key);
