#pragma once

#include <QList>
#include <QString>

/* One evdev node, classified by what it can report. Buttons and keys come
 * straight from the device's capability bits, so a mouse with two side
 * buttons is modelled with two side buttons. */
struct InputDevice {
    QString node; /* /dev/input/eventN */
    QString name;
    quint16 vendor = 0;
    quint16 product = 0;

    bool mouse = false;
    bool keyboard = false;
    bool touchpad = false;

    QList<int> buttons; /* BTN_* the device reports, in evdev order */
    bool wheel = false;
    bool hwheel = false; /* tilt wheel */

    bool numpad = false; /* KEY_KP0 … — a full-size keyboard */
    bool fnRow = false;  /* KEY_F1 … F12 */
    bool extraKeys = false; /* KEY_F13 and up — macro keys */
    int keys = 0;
};

/* Every readable /dev/input/event* node, mice and keyboards first. */
QList<InputDevice> enumerateInputDevices();

/* "Left", "Right", "Middle", "Side 1", "Side 2", … for an evdev button. */
QString buttonLabel(int code);

/* "1038:1830" — what keyd matches a device on. */
QString deviceId(const InputDevice &device);
