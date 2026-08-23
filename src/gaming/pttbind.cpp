#include "pttbind.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QSaveFile>
#include <QTextStream>
#include <QVBoxLayout>

namespace {

QString gamingConfPath()
{
    return QDir::homePath() + "/.local/nixlyos/gaming.conf";
}

/* Qt mouse button -> evdev button code (matches what nixlytile receives
 * from libinput).  qtwayland maps evdev 0x112..0x11f one-to-one onto
 * Middle + ExtraButton1..13, so the reverse is exact. */
int evdevButton(Qt::MouseButton b)
{
    if (b == Qt::MiddleButton)
        return 0x112; /* BTN_MIDDLE */
    for (int i = 0; i < 13; i++)
        if (b == Qt::MouseButton(Qt::ExtraButton1 << i))
            return 0x113 + i; /* BTN_SIDE .. */
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
    for (int i = 0; i < 13; i++)
        if (b == Qt::MouseButton(Qt::ExtraButton1 << i))
            return QString("Mouse Extra %1").arg(i + 1);
    return "Mouse Button";
}

QString modPrefix(Qt::KeyboardModifiers mods, bool forLabel)
{
    QString out;
    if (mods & Qt::ControlModifier) out += forLabel ? "Ctrl+" : "ctrl+";
    if (mods & Qt::AltModifier)     out += forLabel ? "Alt+"  : "alt+";
    if (mods & Qt::ShiftModifier)   out += forLabel ? "Shift+" : "shift+";
    if (mods & Qt::MetaModifier)    out += forLabel ? "Super+" : "super+";
    return out;
}

} // namespace

PttBindWidget::PttBindWidget(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto *heading = new QLabel("Push-to-talk");
    heading->setStyleSheet("color: #f0f0f2; font-size: 17px; font-weight: bold;");
    layout->addWidget(heading);

    auto *hint = new QLabel(
        "The microphone is always muted and only opens while the bound key or "
        "mouse button is held down. The bind works everywhere — including "
        "fullscreen games. Side mouse buttons and Ctrl/Alt/Shift/Super combos "
        "are supported.");
    hint->setStyleSheet("color: #8b8f9a; font-size: 13px;");
    hint->setWordWrap(true);
    layout->addWidget(hint);

    auto *row = new QHBoxLayout;
    row->setSpacing(10);

    m_bindLabel = new QLabel;
    m_bindLabel->setStyleSheet(
        "color: #f0f0f2; font-size: 14px; font-weight: bold;"
        "background: #23252e; border: 1px solid #3a3d49; border-radius: 6px;"
        "padding: 8px 16px;");
    row->addWidget(m_bindLabel);

    m_changeButton = new QPushButton("Set bind");
    m_changeButton->setCursor(Qt::PointingHandCursor);
    m_changeButton->setStyleSheet(
        "QPushButton { color: #f0f0f2; background: #2d5bd1; border: none;"
        " border-radius: 6px; padding: 8px 18px; font-size: 13px; }"
        "QPushButton:hover { background: #3a6ae8; }");
    row->addWidget(m_changeButton);

    m_clearButton = new QPushButton("Clear");
    m_clearButton->setCursor(Qt::PointingHandCursor);
    m_clearButton->setStyleSheet(
        "QPushButton { color: #cccccc; background: #2a2c35; border: 1px solid #3a3d49;"
        " border-radius: 6px; padding: 8px 18px; font-size: 13px; }"
        "QPushButton:hover { background: #343744; }");
    row->addWidget(m_clearButton);
    row->addStretch();
    layout->addLayout(row);

    m_status = new QLabel;
    m_status->setStyleSheet("color: #8b8f9a; font-size: 12px;");
    layout->addWidget(m_status);

    connect(m_changeButton, &QPushButton::clicked, this, &PttBindWidget::startCapture);
    connect(m_clearButton, &QPushButton::clicked, this, &PttBindWidget::clearBind);

    loadBind();
    refreshDisplay();
}

void PttBindWidget::startCapture()
{
    if (m_capturing)
        return;
    m_capturing = true;
    setFocus();
    grabKeyboard();
    qApp->installEventFilter(this);
    m_bindLabel->setText("Press a key or mouse button…");
    m_status->setText("Hold any modifiers and press the key, or click a "
                      "side/middle mouse button. Esc cancels.");
}

void PttBindWidget::stopCapture()
{
    if (!m_capturing)
        return;
    m_capturing = false;
    releaseKeyboard();
    qApp->removeEventFilter(this);
    refreshDisplay();
}

void PttBindWidget::keyPressEvent(QKeyEvent *event)
{
    if (!m_capturing) {
        QWidget::keyPressEvent(event);
        return;
    }

    const int key = event->key();
    if (key == Qt::Key_Escape) {
        stopCapture();
        return;
    }
    /* A bare modifier is not a bind — wait for the real key. */
    if (key == Qt::Key_Control || key == Qt::Key_Alt || key == Qt::Key_Shift
        || key == Qt::Key_Meta || key == Qt::Key_AltGr || key == 0) {
        return;
    }

    const QString mods = modPrefix(event->modifiers(), false);
    const QString labelMods = modPrefix(event->modifiers(), true);
    const QString keyText = QKeySequence(key).toString();

    /* On Wayland nativeVirtualKey() is the xkb keysym — layout-exact.
     * Fall back to the key name, which nixlytile resolves with
     * xkb_keysym_from_name. */
    QString keyToken;
    if (event->nativeVirtualKey() != 0)
        keyToken = QString("key:0x%1").arg(event->nativeVirtualKey(), 0, 16);
    else
        keyToken = keyText.toLower();

    stopCapture();
    saveBind(mods + keyToken, labelMods + keyText);
}

bool PttBindWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (!m_capturing)
        return QWidget::eventFilter(watched, event);

    if (event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent *>(event);
        const int code = evdevButton(me->button());
        /* Left/right stay usable for the UI itself and cannot be bound. */
        if (code != 0) {
            const QString mods = modPrefix(me->modifiers(), false);
            const QString labelMods = modPrefix(me->modifiers(), true);
            stopCapture();
            saveBind(QString("%1mouse:%2").arg(mods).arg(code),
                     labelMods + buttonName(me->button()));
            return true;
        }
        if (me->button() == Qt::LeftButton || me->button() == Qt::RightButton) {
            stopCapture();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void PttBindWidget::focusOutEvent(QFocusEvent *event)
{
    stopCapture();
    QWidget::focusOutEvent(event);
}

void PttBindWidget::clearBind()
{
    stopCapture();
    saveBind(QString(), QString());
}

/* Atomic write (temp + rename): nixlytile watches the file with inotify
 * and must never read a half-written config. */
void PttBindWidget::saveBind(const QString &bind, const QString &label)
{
    m_bind = bind;
    m_label = label;

    const QString path = gamingConfPath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_status->setText("Could not write " + path);
        return;
    }

    QTextStream out(&file);
    out << "# Auto-generated by nixlycc gaming page\n";
    if (!m_bind.isEmpty()) {
        out << "ptt-bind=" << m_bind << "\n";
        out << "ptt-label=" << m_label << "\n";
    }
    out.flush();

    if (!file.commit()) {
        m_status->setText("Could not write " + path);
        return;
    }
    refreshDisplay();
    m_status->setText(m_bind.isEmpty()
                          ? "Bind cleared — microphone stays muted."
                          : "Saved. Applied immediately by nixlytile.");
}

void PttBindWidget::loadBind()
{
    QFile file(gamingConfPath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.startsWith("ptt-bind="))
            m_bind = line.mid(9);
        else if (line.startsWith("ptt-label="))
            m_label = line.mid(10);
    }
}

void PttBindWidget::refreshDisplay()
{
    if (m_bind.isEmpty()) {
        m_bindLabel->setText("No bind set");
        m_status->setText("Microphone stays muted until a bind is set.");
    } else {
        m_bindLabel->setText(m_label.isEmpty() ? m_bind : m_label);
        m_status->setText("Hold the bind to talk; release to mute.");
    }
}
