#pragma once

#include <QString>
#include <QStringList>
#include <functional>

/* Bakes a local icon cache: one 64x64 PNG per package name under
 * ~/.cache/nixlycc/icons.
 *
 * nixpkgs carries no icon data outside built package outputs, and the binary
 * cache only serves whole NARs, so icons for packages you never built have to
 * come from name-keyed catalogues. Sources, best first: icons shipped by
 * packages that happen to be in /nix/store, Debian's DEP-11 catalogue (a 7.6 MB
 * download, ~2300 upstream app icons keyed by package name), and the icon-theme
 * packs in the store.
 *
 * Only names present in the package index are baked, so the cache stays a few
 * thousand small files. The farm runs once per nixpkgs revision on a worker
 * thread; both callbacks fire on the GUI thread. */
namespace IconFarm {

QString dir();

/* Where a baked icon lives. Missing file means the farm found nothing. */
QString fileFor(const QString &key);

/* keys: lowercased package names worth an icon. stamp: refarm key — while it
 * is unchanged the existing cache is reused untouched. */
void run(const QStringList &keys, const QString &stamp,
         std::function<void(const QString &)> onStatus,
         std::function<void()> onDone);

} // namespace IconFarm
