#include "pttbind.h"
#include "capture.h"
#include "style.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

PttBindWidget::PttBindWidget(QWidget *parent)
    : QWidget(parent)
    , m_capture(new BindCapture(this))
{
    setFocusPolicy(Qt::StrongFocus);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    layout->addWidget(makeHeading("Push-to-talk"));
    layout->addWidget(makeHint(
        "The microphone is always muted and only opens while the bound key or "
        "mouse button is held down. The bind works everywhere — including "
        "fullscreen games. Side mouse buttons and Ctrl/Alt/Shift/Super combos "
        "are supported. Meeting apps (Teams, Citrix) always hear you."));

    auto *row = new QHBoxLayout;
    row->setSpacing(10);

    m_bindLabel = makeBindChip();
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
    connect(m_clearButton, &QPushButton::clicked, this, [this]() { saveBind({}); });
    m_capture->onDone = [this](const Bind &bind) {
        if (bind.value.isEmpty()) {
            refreshDisplay();
            return;
        }
        saveBind(bind);
    };

    m_bind = loadGamingConf().talk;
    refreshDisplay();
}

void PttBindWidget::startCapture()
{
    m_capture->start();
    m_bindLabel->setText("Press a key or mouse button…");
    m_status->setText("Hold any modifiers and press the key, or click a "
                      "side/middle mouse button. Esc cancels.");
}

/* Reload first so push-to-mute binds survive. */
void PttBindWidget::saveBind(const Bind &bind)
{
    GamingConf conf = loadGamingConf();
    conf.talk = bind;
    if (!saveGamingConf(conf)) {
        m_status->setText("Could not write " + gamingConfPath());
        return;
    }
    m_bind = bind;
    refreshDisplay();
    m_status->setText(m_bind.value.isEmpty()
                          ? "Bind cleared — microphone is always open."
                          : "Saved. Applied immediately by nixlytile.");
}

void PttBindWidget::refreshDisplay()
{
    if (m_bind.value.isEmpty()) {
        m_bindLabel->setText("No bind set");
        m_status->setText("No bind — the microphone is always open.");
        return;
    }
    m_bindLabel->setText(m_bind.label.isEmpty() ? m_bind.value : m_bind.label);
    m_status->setText("Hold the bind to talk; release to mute.");
}
