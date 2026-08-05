#pragma once

#include <QHash>
#include <QString>

/* What the page lets you set for the pointer. accelSpeed is libinput's
 * -1 … 1; dpi and pollingRate are hardware settings only libratbag-supported
 * mice accept, and 0 means "leave the device alone". */
struct MouseSettings {
    QString device;
    QString id; /* vendor:product, as keyd and ratbagctl match on */
    QString accelProfile = "adaptive";
    double accelSpeed = 0.0;
    bool naturalScroll = false;
    bool leftHanded = false;
    bool middleEmulation = false;
    QString scrollMethod = "wheel";
    QString buttonMap = "lrm";
    int dpi = 0;
    int pollingRate = 0;
    QHash<int, QString> actions; /* evdev button → keyd action, empty = default */
};

struct KeyboardSettings {
    QString device;
    QString id;
    QString layout = "no";
    QString variant;
    int repeatDelay = 300;
    int repeatRate = 100;
    QHash<int, QString> remaps; /* evdev key → keyd action */
    QString rgbMode = "off";    /* off | static | breathing | rainbow */
    QString rgbColor = "#00beff";
    int brightness = 100;
};

struct InputSettings {
    MouseSettings mouse;
    KeyboardSettings keyboard;
};

/* ~/.local/nixlyos/input.state — what the page had last time. The nix module
 * is generated from this on Apply; this file only remembers the UI. */
InputSettings readInputSettings();
QString writeInputSettings(const InputSettings &settings);

/* Actions a button or key can be bound to: display name → keyd action. The
 * first entry is "Default", which removes the binding. */
const QList<QPair<QString, QString>> &inputActions();

/* keyd's name for an evdev button, or an empty string when keyd cannot bind
 * it. */
QString keydButtonName(int code);

/* keyd's name for an evdev key — only the keys the models can show. */
QString keydKeyName(int code);
