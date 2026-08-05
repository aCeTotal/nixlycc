#pragma once

#include <QByteArray>
#include <QString>
#include <functional>

/* Runs `sudo nixos-rebuild switch --flake ~/.nixlyos#nixlyos`, feeding sudo
 * the password on stdin. home-manager runs as a NixOS module here, so switch
 * is what activates a changed git.nix — there is no per-module rebuild, but
 * only the changed derivations are built.
 *
 * Blocking for as long as the rebuild takes; call it from a worker thread.
 * onLine receives each output line on that same thread. Returns an empty
 * string on success, else the error text. */
QString rebuildSwitch(const QByteArray &password, std::function<void(const QString &)> onLine);
