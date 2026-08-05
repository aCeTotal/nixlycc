#pragma once

#include <QHash>
#include <QString>
#include <QStringList>
#include <functional>

/* Versions these attributes have in the flake's nixpkgs-unstable input.
 *
 * One nix eval for the whole batch — a few hundred milliseconds — and the
 * answers are kept for the rest of the session. Attributes unstable does not
 * have, or that fail to evaluate, come back as an empty string. */
void unstableVersions(const QStringList &attrs,
                      std::function<void(const QHash<QString, QString> &)> done);
