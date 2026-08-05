#pragma once

#include "mesh.h"

#include <QHash>
#include <QPointF>

struct InputDevice;

/* The keyboard as modelled, plus where each key ended up — the view swings
 * towards the key you are editing. */
struct KeyboardModel {
    Mesh mesh;
    QHash<int, QPointF> keyCentre; /* evdev code → (x, z) in model space */
    float halfWidth = 1.0f;
};

/* Rows are picked from what the device reports: function row, number pad and
 * macro keys only appear when the keyboard has them. */
KeyboardModel buildKeyboardModel(const InputDevice &device);

/* Camera angles that bring this key to the front. */
void keyboardViewFor(const KeyboardModel &model, int part, float *yaw, float *pitch);
