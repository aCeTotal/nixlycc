#pragma once

#include "mesh.h"

struct InputDevice;

/* A mouse body carrying exactly the buttons the device reports: two side
 * buttons on the device means two side buttons on the model. */
Mesh buildMouseMesh(const InputDevice &device);

/* Camera angles that put this button in view — the model turns to the side
 * the button sits on. */
void mouseViewFor(int part, float *yaw, float *pitch);
