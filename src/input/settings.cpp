#include "settings.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>

#include <linux/input.h>

namespace {

QString statePath()
{
    const QString path = QDir::homePath() + "/.local/nixlyos/input.state";
    QDir().mkpath(QFileInfo(path).absolutePath());
    return path;
}

/* "272=f13,275=C-c" — the binding maps live in one line each. */
QHash<int, QString> readMap(const QString &text)
{
    QHash<int, QString> out;
    for (const QString &pair : text.split(',', Qt::SkipEmptyParts)) {
        const int eq = pair.indexOf('=');
        if (eq > 0)
            out.insert(pair.left(eq).toInt(), pair.mid(eq + 1));
    }
    return out;
}

QString writeMap(const QHash<int, QString> &map)
{
    QStringList parts;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        if (!it.value().isEmpty())
            parts << QString("%1=%2").arg(it.key()).arg(it.value());
    }
    parts.sort();
    return parts.join(',');
}

} // namespace

const QList<QPair<QString, QString>> &inputActions()
{
    static const QList<QPair<QString, QString>> actions = {
        { "Default", "" },
        { "Disabled", "noop" },
        { "Left click", "leftmouse" },
        { "Right click", "rightmouse" },
        { "Middle click", "middlemouse" },
        { "Back", "mouseback" },
        { "Forward", "mouseforward" },
        { "Scroll up", "scrollup" },
        { "Scroll down", "scrolldown" },
        { "Copy (Ctrl+C)", "C-c" },
        { "Paste (Ctrl+V)", "C-v" },
        { "Undo (Ctrl+Z)", "C-z" },
        { "Close window (Alt+F4)", "A-f4" },
        { "Super", "leftmeta" },
        { "Escape", "esc" },
        { "Enter", "enter" },
        { "Tab", "tab" },
        { "Play / pause", "playpause" },
        { "Next track", "nextsong" },
        { "Previous track", "previoussong" },
        { "Volume up", "volumeup" },
        { "Volume down", "volumedown" },
        { "Macro key F13", "f13" },
        { "Macro key F14", "f14" },
        { "Macro key F15", "f15" },
        { "Macro key F16", "f16" },
    };
    return actions;
}

/* keyd handles mice as a special case and only names these five buttons. */
QString keydButtonName(int code)
{
    switch (code) {
    case BTN_LEFT:
        return "leftmouse";
    case BTN_RIGHT:
        return "rightmouse";
    case BTN_MIDDLE:
        return "middlemouse";
    case BTN_SIDE:
    case BTN_BACK:
        return "mouseback";
    case BTN_EXTRA:
    case BTN_FORWARD:
        return "mouseforward";
    default:
        return QString();
    }
}

QString keydKeyName(int code)
{
    static const QHash<int, QString> names = {
        { KEY_ESC, "esc" },
        { KEY_TAB, "tab" },
        { KEY_CAPSLOCK, "capslock" },
        { KEY_ENTER, "enter" },
        { KEY_SPACE, "space" },
        { KEY_BACKSPACE, "backspace" },
        { KEY_LEFTSHIFT, "leftshift" },
        { KEY_RIGHTSHIFT, "rightshift" },
        { KEY_LEFTCTRL, "leftcontrol" },
        { KEY_RIGHTCTRL, "rightcontrol" },
        { KEY_LEFTALT, "leftalt" },
        { KEY_RIGHTALT, "rightalt" },
        { KEY_LEFTMETA, "leftmeta" },
        { KEY_RIGHTMETA, "rightmeta" },
        { KEY_COMPOSE, "compose" },
        { KEY_DELETE, "delete" },
        { KEY_NUMLOCK, "numlock" },
        { KEY_GRAVE, "grave" },
        { KEY_MINUS, "minus" },
        { KEY_EQUAL, "equal" },
        { KEY_LEFTBRACE, "leftbrace" },
        { KEY_RIGHTBRACE, "rightbrace" },
        { KEY_BACKSLASH, "backslash" },
        { KEY_SEMICOLON, "semicolon" },
        { KEY_APOSTROPHE, "apostrophe" },
        { KEY_COMMA, "comma" },
        { KEY_DOT, "dot" },
        { KEY_SLASH, "slash" },
    };
    const auto it = names.constFind(code);
    if (it != names.constEnd())
        return *it;

    if (code >= KEY_1 && code <= KEY_9)
        return QString::number(code - KEY_1 + 1);
    if (code == KEY_0)
        return "0";
    if (code >= KEY_F1 && code <= KEY_F10)
        return QString("f%1").arg(code - KEY_F1 + 1);
    if (code == KEY_F11 || code == KEY_F12)
        return QString("f%1").arg(code == KEY_F11 ? 11 : 12);
    if (code >= KEY_F13 && code <= KEY_F16)
        return QString("f%1").arg(code - KEY_F13 + 13);

    static const QHash<int, QString> letters = {
        { KEY_A, "a" }, { KEY_B, "b" }, { KEY_C, "c" }, { KEY_D, "d" }, { KEY_E, "e" },
        { KEY_F, "f" }, { KEY_G, "g" }, { KEY_H, "h" }, { KEY_I, "i" }, { KEY_J, "j" },
        { KEY_K, "k" }, { KEY_L, "l" }, { KEY_M, "m" }, { KEY_N, "n" }, { KEY_O, "o" },
        { KEY_P, "p" }, { KEY_Q, "q" }, { KEY_R, "r" }, { KEY_S, "s" }, { KEY_T, "t" },
        { KEY_U, "u" }, { KEY_V, "v" }, { KEY_W, "w" }, { KEY_X, "x" }, { KEY_Y, "y" },
        { KEY_Z, "z" },
    };
    return letters.value(code);
}

InputSettings readInputSettings()
{
    QSettings state(statePath(), QSettings::IniFormat);
    InputSettings out;

    state.beginGroup("mouse");
    out.mouse.accelProfile = state.value("accel-profile", out.mouse.accelProfile).toString();
    out.mouse.accelSpeed = state.value("accel-speed", out.mouse.accelSpeed).toDouble();
    out.mouse.naturalScroll = state.value("natural-scroll", false).toBool();
    out.mouse.leftHanded = state.value("left-handed", false).toBool();
    out.mouse.middleEmulation = state.value("middle-emulation", false).toBool();
    out.mouse.scrollMethod = state.value("scroll-method", out.mouse.scrollMethod).toString();
    out.mouse.buttonMap = state.value("button-map", out.mouse.buttonMap).toString();
    out.mouse.dpi = state.value("dpi", 0).toInt();
    out.mouse.pollingRate = state.value("polling-rate", 0).toInt();
    out.mouse.actions = readMap(state.value("actions").toString());
    state.endGroup();

    state.beginGroup("keyboard");
    out.keyboard.layout = state.value("layout", out.keyboard.layout).toString();
    out.keyboard.variant = state.value("variant").toString();
    out.keyboard.repeatDelay = state.value("repeat-delay", out.keyboard.repeatDelay).toInt();
    out.keyboard.repeatRate = state.value("repeat-rate", out.keyboard.repeatRate).toInt();
    out.keyboard.remaps = readMap(state.value("remaps").toString());
    out.keyboard.rgbMode = state.value("rgb-mode", out.keyboard.rgbMode).toString();
    out.keyboard.rgbColor = state.value("rgb-color", out.keyboard.rgbColor).toString();
    out.keyboard.brightness = state.value("brightness", out.keyboard.brightness).toInt();
    state.endGroup();

    return out;
}

QString writeInputSettings(const InputSettings &settings)
{
    QSettings state(statePath(), QSettings::IniFormat);

    state.beginGroup("mouse");
    state.setValue("device", settings.mouse.device);
    state.setValue("id", settings.mouse.id);
    state.setValue("accel-profile", settings.mouse.accelProfile);
    state.setValue("accel-speed", settings.mouse.accelSpeed);
    state.setValue("natural-scroll", settings.mouse.naturalScroll);
    state.setValue("left-handed", settings.mouse.leftHanded);
    state.setValue("middle-emulation", settings.mouse.middleEmulation);
    state.setValue("scroll-method", settings.mouse.scrollMethod);
    state.setValue("button-map", settings.mouse.buttonMap);
    state.setValue("dpi", settings.mouse.dpi);
    state.setValue("polling-rate", settings.mouse.pollingRate);
    state.setValue("actions", writeMap(settings.mouse.actions));
    state.endGroup();

    state.beginGroup("keyboard");
    state.setValue("device", settings.keyboard.device);
    state.setValue("id", settings.keyboard.id);
    state.setValue("layout", settings.keyboard.layout);
    state.setValue("variant", settings.keyboard.variant);
    state.setValue("repeat-delay", settings.keyboard.repeatDelay);
    state.setValue("repeat-rate", settings.keyboard.repeatRate);
    state.setValue("remaps", writeMap(settings.keyboard.remaps));
    state.setValue("rgb-mode", settings.keyboard.rgbMode);
    state.setValue("rgb-color", settings.keyboard.rgbColor);
    state.setValue("brightness", settings.keyboard.brightness);
    state.endGroup();

    state.sync();
    if (state.status() != QSettings::NoError)
        return QString("Could not write %1").arg(statePath());
    return QString();
}
