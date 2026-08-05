#include "kbdmesh.h"
#include "devices.h"

#include <linux/input.h>

namespace {

const QColor kCase(46, 49, 58);
const QColor kCap(84, 89, 104);
const QColor kModifier(66, 71, 86);

const float kUnit = 19.0f;  /* one key pitch */
const float kGap = 2.0f;
const float kRowPitch = 19.0f;

struct Key {
    int code;
    const char *label;
    float units;
};

/* One entry per physical key, in rows. Wide keys carry their width. */
const QList<QList<Key>> &mainRows()
{
    static const QList<QList<Key>> rows = {
        { { KEY_GRAVE, "`", 1 },  { KEY_1, "1", 1 },     { KEY_2, "2", 1 },
          { KEY_3, "3", 1 },      { KEY_4, "4", 1 },     { KEY_5, "5", 1 },
          { KEY_6, "6", 1 },      { KEY_7, "7", 1 },     { KEY_8, "8", 1 },
          { KEY_9, "9", 1 },      { KEY_0, "0", 1 },     { KEY_MINUS, "-", 1 },
          { KEY_EQUAL, "=", 1 },  { KEY_BACKSPACE, "Bksp", 2 } },
        { { KEY_TAB, "Tab", 1.5f }, { KEY_Q, "Q", 1 },   { KEY_W, "W", 1 },
          { KEY_E, "E", 1 },        { KEY_R, "R", 1 },   { KEY_T, "T", 1 },
          { KEY_Y, "Y", 1 },        { KEY_U, "U", 1 },   { KEY_I, "I", 1 },
          { KEY_O, "O", 1 },        { KEY_P, "P", 1 },   { KEY_LEFTBRACE, "[", 1 },
          { KEY_RIGHTBRACE, "]", 1 }, { KEY_BACKSLASH, "\\", 1.5f } },
        { { KEY_CAPSLOCK, "Caps", 1.75f }, { KEY_A, "A", 1 }, { KEY_S, "S", 1 },
          { KEY_D, "D", 1 },        { KEY_F, "F", 1 },   { KEY_G, "G", 1 },
          { KEY_H, "H", 1 },        { KEY_J, "J", 1 },   { KEY_K, "K", 1 },
          { KEY_L, "L", 1 },        { KEY_SEMICOLON, ";", 1 }, { KEY_APOSTROPHE, "'", 1 },
          { KEY_ENTER, "Enter", 2.25f } },
        { { KEY_LEFTSHIFT, "Shift", 2.25f }, { KEY_Z, "Z", 1 }, { KEY_X, "X", 1 },
          { KEY_C, "C", 1 },        { KEY_V, "V", 1 },   { KEY_B, "B", 1 },
          { KEY_N, "N", 1 },        { KEY_M, "M", 1 },   { KEY_COMMA, ",", 1 },
          { KEY_DOT, ".", 1 },      { KEY_SLASH, "/", 1 }, { KEY_RIGHTSHIFT, "Shift", 2.75f } },
        { { KEY_LEFTCTRL, "Ctrl", 1.25f }, { KEY_LEFTMETA, "Super", 1.25f },
          { KEY_LEFTALT, "Alt", 1.25f },   { KEY_SPACE, "Space", 6.25f },
          { KEY_RIGHTALT, "Alt", 1.25f },  { KEY_RIGHTMETA, "Super", 1.25f },
          { KEY_COMPOSE, "Menu", 1.25f },  { KEY_RIGHTCTRL, "Ctrl", 1.25f } },
    };
    return rows;
}

const QList<Key> &functionRow()
{
    static const QList<Key> row = {
        { KEY_ESC, "Esc", 1 },  { KEY_F1, "F1", 1 },   { KEY_F2, "F2", 1 },
        { KEY_F3, "F3", 1 },    { KEY_F4, "F4", 1 },   { KEY_F5, "F5", 1 },
        { KEY_F6, "F6", 1 },    { KEY_F7, "F7", 1 },   { KEY_F8, "F8", 1 },
        { KEY_F9, "F9", 1 },    { KEY_F10, "F10", 1 }, { KEY_F11, "F11", 1 },
        { KEY_F12, "F12", 1 },  { KEY_DELETE, "Del", 1 },
    };
    return row;
}

/* Number pad, laid out as four rows to the right of the main block. */
const QList<QList<Key>> &numpadRows()
{
    static const QList<QList<Key>> rows = {
        { { KEY_NUMLOCK, "Num", 1 }, { KEY_KPSLASH, "/", 1 }, { KEY_KPASTERISK, "*", 1 },
          { KEY_KPMINUS, "-", 1 } },
        { { KEY_KP7, "7", 1 }, { KEY_KP8, "8", 1 }, { KEY_KP9, "9", 1 }, { KEY_KPPLUS, "+", 1 } },
        { { KEY_KP4, "4", 1 }, { KEY_KP5, "5", 1 }, { KEY_KP6, "6", 1 } },
        { { KEY_KP1, "1", 1 }, { KEY_KP2, "2", 1 }, { KEY_KP3, "3", 1 },
          { KEY_KPENTER, "Ent", 1 } },
        { { KEY_KP0, "0", 2 }, { KEY_KPDOT, ".", 1 } },
    };
    return rows;
}

const QList<Key> &macroColumn()
{
    static const QList<Key> keys = {
        { KEY_F13, "M1", 1 }, { KEY_F14, "M2", 1 }, { KEY_F15, "M3", 1 }, { KEY_F16, "M4", 1 },
    };
    return keys;
}

bool isModifier(int code)
{
    switch (code) {
    case KEY_LEFTSHIFT:
    case KEY_RIGHTSHIFT:
    case KEY_LEFTCTRL:
    case KEY_RIGHTCTRL:
    case KEY_LEFTALT:
    case KEY_RIGHTALT:
    case KEY_LEFTMETA:
    case KEY_RIGHTMETA:
    case KEY_CAPSLOCK:
    case KEY_TAB:
    case KEY_ENTER:
    case KEY_BACKSPACE:
    case KEY_SPACE:
        return true;
    default:
        return false;
    }
}

void addRow(KeyboardModel &model, const QList<Key> &row, float x, float z)
{
    for (const Key &key : row) {
        const float w = key.units * kUnit - kGap;
        model.mesh.addBox(QVector3D(x, -12.0f, z), QVector3D(x + w, -6.0f, z + kUnit - kGap),
                          isModifier(key.code) ? kModifier : kCap, key.code,
                          QString::fromUtf8(key.label));
        model.keyCentre.insert(key.code, QPointF(x + w / 2, z + kUnit / 2));
        x += key.units * kUnit;
    }
}

} // namespace

KeyboardModel buildKeyboardModel(const InputDevice &device)
{
    KeyboardModel model;

    const bool macros = device.extraKeys;
    const float left = macros ? -kUnit * 1.2f : 0.0f;
    float z = 0.0f;

    if (device.fnRow) {
        addRow(model, functionRow(), left, z);
        z += kRowPitch + 4.0f;
    }
    for (const QList<Key> &row : mainRows()) {
        addRow(model, row, left, z);
        z += kRowPitch;
    }

    const float mainRight = left + 15.0f * kUnit;
    if (device.numpad) {
        float padZ = device.fnRow ? kRowPitch + 4.0f : 0.0f;
        for (const QList<Key> &row : numpadRows()) {
            addRow(model, row, mainRight + kUnit * 0.5f, padZ);
            padZ += kRowPitch;
        }
    }
    if (macros) {
        float macroZ = device.fnRow ? kRowPitch + 4.0f : 0.0f;
        for (const Key &key : macroColumn()) {
            addRow(model, { key }, left - kUnit * 1.3f, macroZ);
            macroZ += kRowPitch;
        }
    }

    /* Case: a slab under every key, plus a lip in front. */
    float maxX = 0.0f, maxZ = 0.0f, minX = 0.0f;
    for (auto it = model.keyCentre.constBegin(); it != model.keyCentre.constEnd(); ++it) {
        maxX = std::max(maxX, float(it.value().x()));
        minX = std::min(minX, float(it.value().x()));
        maxZ = std::max(maxZ, float(it.value().y()));
    }
    const float pad = kUnit * 0.9f;
    Mesh caseMesh;
    caseMesh.addBox(QVector3D(minX - pad, -7.0f, -pad), QVector3D(maxX + pad, 4.0f, maxZ + pad),
                    kCase);

    /* Centre the whole thing on the origin so it spins about its middle. */
    const float cx = (minX - pad + maxX + pad) / 2;
    const float cz = (-pad + maxZ + pad) / 2;
    Mesh out = caseMesh;
    const int base = out.verts.size();
    out.verts += model.mesh.verts;
    for (Face face : model.mesh.faces) {
        for (int &i : face.idx)
            i += base;
        out.faces << face;
    }
    for (QVector3D &v : out.verts)
        v = QVector3D(v.x() - cx, v.y(), v.z() - cz);
    for (auto it = model.keyCentre.begin(); it != model.keyCentre.end(); ++it)
        *it = QPointF(it.value().x() - cx, it.value().y() - cz);

    model.mesh = out;
    model.halfWidth = std::max(1.0f, maxX + pad - cx);
    return model;
}

void keyboardViewFor(const KeyboardModel &model, int part, float *yaw, float *pitch)
{
    *yaw = 0.0f;
    *pitch = -1.0f;
    const auto it = model.keyCentre.constFind(part);
    if (it == model.keyCentre.constEnd())
        return;

    /* Swing towards the side the key is on, and lean in a little. */
    *yaw = float(-it.value().x() / model.halfWidth) * 0.45f;
    *pitch = -0.78f;
}
