#pragma once

#include <QString>

struct InputSettings;

/* ~/.nixlyos/modules/core/input.nix — generated whole, owned by the Mouse &
 * Keyboard page. */
QString inputModulePath();

/* Writes the module: xkb layout, keyd bindings for the mouse buttons and
 * keys, the libinput settings the compositor reads from
 * /etc/nixlyos/input.conf, and a one-shot service for DPI, polling rate and
 * RGB on devices that support them. Returns an empty string on success. */
QString writeInputModule(const InputSettings &settings);
