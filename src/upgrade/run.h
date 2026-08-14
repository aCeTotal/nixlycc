#pragma once

#include <QByteArray>
#include <QString>
#include <functional>

/* Updates every ~/.nixlyos flake input, then runs nixos-rebuild switch with
 * sudo fed the password LockPage collected.
 *
 * Blocking for as long as the rebuild takes — call it from runAsync.
 * onProgress gets a 0–100 percentage and a one-line description of what is
 * running, on the worker thread. Returns an empty string on success, else the
 * error text. */
QString upgradeSystem(const QByteArray &password,
                      std::function<void(int, const QString &)> onProgress);
