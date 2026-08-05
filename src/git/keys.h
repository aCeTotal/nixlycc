#pragma once

#include <QString>

/* ~/.ssh/<name> — the private key; the public half is that plus ".pub". */
QString keyPath(const QString &name);
bool keyExists(const QString &name);

/* Generates an ed25519 key pair, replacing any existing one with that name.
 * Returns an empty string on success, else the error text. */
QString generateKey(const QString &name, const QString &comment);

/* Contents of ~/.ssh/<name>.pub, or an empty string when there is none. */
QString readPublicKey(const QString &name);
