#pragma once

#include <QString>
#include <functional>

/* Store path of a ~/.nixlyos flake input, e.g. "nixpkgs" or
 * "nixpkgs-unstable". This is the nixpkgs the system is actually built from —
 * NIX_PATH on a flake system points at "flake:nixpkgs", which no tool can
 * enumerate.
 *
 * Async, and evaluated once per input: done() gets the path, or an empty
 * string when the flake or the input is missing. */
void flakeInputPath(const QString &input, std::function<void(const QString &)> done);
