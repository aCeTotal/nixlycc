#pragma once

#include <QList>
#include <QString>
#include <QStringList>

/* One package in packages.nix, and which channel it comes from: pkgs is the
 * flake's stable nixpkgs, pkgs-unstable the nixos-unstable input that
 * flake.nix already hands to home-manager. */
struct PkgEntry {
    QString attr;
    bool unstable = false;
};

/* ~/.nixlyos/modules/core/packages.nix — owned by the Applications page. */
QString packagesPath();

/* What the file installs right now, in file order. */
QList<PkgEntry> installedPackages();

/* Regenerates the file from these entries, sorted, with a stable list and an
 * unstable list. Returns an empty string on success, else the error text. */
QString writeInstalledPackages(const QList<PkgEntry> &entries);
