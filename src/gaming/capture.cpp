#include "capture.h"

#include <QApplication>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMouseEvent>
#include <QWidget>

namespace {

const int kBtnMiddle = 0x112;
const int kBtnSide = 0x113;
const int kExtraButtons = 13;

/* qtwayland maps evdev 0x112..0x11f onto Middle + Extra1..13. */
int evdevButton(Qt::MouseButton b)
{
    if (b == Qt::MiddleButton)
        return kBtnMiddle;
    for (int i = 0; i < kExtraButtons; i++)
        if (b == Qt::MouseButton(Qt::ExtraButton1 << i))
            return kBtnSide + i;
    return 0;
}

QString buttonName(Qt::MouseButton b)
{
    switch (b) {
    case Qt::MiddleButton:  return "Mouse Middle";
    case Qt::BackButton:    return "Mouse Side (back)";
    case Qt::ForwardButton: return "Mouse Side (forward)";
    default:                break;
    }
    for (int i = 0; i < kExtraButtons; i++)
        if (b == Qt::MouseButton(Qt::ExtraButton1 << i))
            return QString("Mouse Extra %1").arg(i + 1);
    return "Mouse Button";
}

struct ModName {
    Qt::KeyboardModifier mod;
    const char *value;
    const char *label;
};

const ModName kMods[] = {
    {Qt::ControlModifier, "ctrl+", "Ctrl+"},
    {Qt::AltModifier, "alt+", "Alt+"},
    {Qt::ShiftModifier, "shift+", "Shift+"},
    {Qt::MetaModifier, "super+", "Super+"},
};

Bind modPrefix(Qt::KeyboardModifiers mods)
{
    Bind prefix;
    for (const ModName &m : kMods) {
        if (!(mods & m.mod))
            continue;
        prefix.value += m.value;
        prefix.label += m.label;
    }
    return prefix;
}

bool isModifierKey(int key)
{
    return key == Qt::Key_Control || key == Qt::Key_Alt || key == Qt::Key_Shift
        || key == Qt::Key_Meta || key == Qt::Key_AltGr || key == 0;
}

} // namespace

BindCapture::BindCapture(QWidget *owner)
    : QObject(owner)
    , m_owner(owner)
{
}

void BindCapture::start()
{
    if (m_active)
        return;
    m_active = true;
    m_owner->setFocus();
    m_owner->grabKeyboard();
    qApp->installEventFilter(this);
}

void BindCapture::finish(const Bind &bind)
{
    m_active = false;
    m_owner->releaseKeyboard();
    qApp->removeEventFilter(this);
    if (onDone)
        onDone(bind);
}

/* On Wayland nativeVirtualKey() is the layout-exact xkb keysym. */
void BindCapture::handleKey(QKeyEvent *event)
{
    const int key = event->key();
    if (key == Qt::Key_Escape) {
        finish({});
        return;
    }
    if (isModifierKey(key))
        return;

    const Bind prefix = modPrefix(event->modifiers());
    const QString keyText = QKeySequence(key).toString();
    const QString token = event->nativeVirtualKey() != 0
        ? QString("key:0x%1").arg(event->nativeVirtualKey(), 0, 16)
        : keyText.toLower();
    finish({prefix.value + token, prefix.label + keyText});
}

/* Left and right stay for the UI and cancel. */
void BindCapture::handleMouse(QMouseEvent *event)
{
    const int code = evdevButton(event->button());
    if (code == 0) {
        finish({});
        return;
    }
    const Bind prefix = modPrefix(event->modifiers());
    finish({QString("%1mouse:%2").arg(prefix.value).arg(code),
            prefix.label + buttonName(event->button())});
}

bool BindCapture::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress:
        handleKey(static_cast<QKeyEvent *>(event));
        return true;
    case QEvent::MouseButtonPress:
        handleMouse(static_cast<QMouseEvent *>(event));
        return true;
    case QEvent::ApplicationDeactivate:
        finish({});
        return false;
    default:
        return QObject::eventFilter(watched, event);
    }
}
