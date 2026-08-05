#include "pages.h"
#include "apps/password.h"
#include "git/rebuild.h"
#include "git/repo.h"
#include "git/style.h"
#include "git/task.h"
#include "input/kbdpanel.h"
#include "input/module.h"
#include "input/mousepanel.h"
#include "input/settings.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {

const char *kApplyStyle =
    "QPushButton { background-color: rgba(122, 162, 247, 70);"
    " border: 1px solid rgba(122, 162, 247, 150); border-radius: 10px;"
    " padding: 10px 28px; color: #f0f0f2; font-size: 14px; font-weight: bold; }"
    "QPushButton:hover { background-color: rgba(122, 162, 247, 110); }"
    "QPushButton:disabled { color: #8b8f9a; border-color: rgba(255, 255, 255, 30);"
    " background-color: rgba(255, 255, 255, 12); }";

QScrollArea *wrap(QWidget *inner)
{
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->viewport()->setAutoFillBackground(false);
    scroll->setWidget(inner);
    return scroll;
}

/* Mouse and keyboard in one page: each half edits its own settings, and Apply
 * writes modules/core/input.nix and switches the system onto it. */
class MouseKeyboardPage : public QWidget {
public:
    MouseKeyboardPage();

private:
    void apply();
    void markDirty();

    MousePanel *m_mouse;
    KeyboardPanel *m_keyboard;
    QStackedWidget *m_stack;
    QPushButton *m_mouseTab;
    QPushButton *m_keyboardTab;
    QWidget *m_applyBar;
    QLabel *m_applyStatus;
    QPushButton *m_applyButton;
    bool m_ready = false;
};

MouseKeyboardPage::MouseKeyboardPage()
{
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(10);

    auto *title = new QLabel("Mouse & Keyboard");
    title->setStyleSheet("color: white; font-size: 24px; font-weight: bold; margin-bottom: 10px;");
    layout->addWidget(title);

    m_mouseTab = makeTab("Mouse");
    m_keyboardTab = makeTab("Keyboard");
    m_mouseTab->setChecked(true);
    auto *tabs = new QHBoxLayout;
    tabs->setSpacing(10);
    tabs->addWidget(m_mouseTab);
    tabs->addWidget(m_keyboardTab);
    tabs->addStretch();
    layout->addLayout(tabs);

    m_mouse = new MousePanel;
    m_keyboard = new KeyboardPanel;
    m_mouse->onChanged = [this]() { markDirty(); };
    m_keyboard->onChanged = [this]() { markDirty(); };

    m_stack = new QStackedWidget;
    m_stack->addWidget(wrap(m_mouse));
    m_stack->addWidget(wrap(m_keyboard));
    layout->addWidget(m_stack, 1);

    m_applyBar = new QWidget;
    auto *bar = new QHBoxLayout(m_applyBar);
    bar->setContentsMargins(0, 4, 0, 0);
    m_applyStatus = new QLabel("Unapplied changes.");
    m_applyStatus->setStyleSheet("color: #f0f0f2; font-size: 13px;");
    m_applyStatus->setWordWrap(true);
    m_applyButton = new QPushButton("Apply");
    m_applyButton->setCursor(Qt::PointingHandCursor);
    m_applyButton->setStyleSheet(kApplyStyle);
    bar->addWidget(m_applyStatus, 1);
    bar->addWidget(m_applyButton);
    layout->addWidget(m_applyBar);
    m_applyBar->hide();

    connect(m_mouseTab, &QPushButton::clicked, this, [this]() {
        m_mouseTab->setChecked(true);
        m_keyboardTab->setChecked(false);
        m_stack->setCurrentIndex(0);
    });
    connect(m_keyboardTab, &QPushButton::clicked, this, [this]() {
        m_keyboardTab->setChecked(true);
        m_mouseTab->setChecked(false);
        m_stack->setCurrentIndex(1);
    });
    connect(m_applyButton, &QPushButton::clicked, this, &MouseKeyboardPage::apply);

    m_ready = true;
}

void MouseKeyboardPage::markDirty()
{
    if (!m_ready)
        return;
    m_applyStatus->setText("Unapplied changes.");
    m_applyBar->show();
}

/* Writes the module first, then switches onto it, so the settings are live
 * when the rebuild returns. */
void MouseKeyboardPage::apply()
{
    QByteArray password = askPassword(this);
    if (password.isEmpty())
        return;

    InputSettings settings;
    settings.mouse = m_mouse->settings();
    settings.keyboard = m_keyboard->settings();

    QString error = writeInputSettings(settings);
    if (error.isEmpty())
        error = writeInputModule(settings);
    if (!error.isEmpty()) {
        m_applyStatus->setText(error);
        return;
    }

    m_applyButton->setEnabled(false);
    m_applyStatus->setText("Rebuilding…");

    QPointer<MouseKeyboardPage> self(this);
    runAsync(
        [password, self]() {
            const QString error = rebuildSwitch(password, [self](const QString &line) {
                postToGui([self, line]() {
                    if (self)
                        self->m_applyStatus->setText(line);
                });
            });
            if (!error.isEmpty())
                return error;
            return commitAndPush("Changes Mouse & Keyboard");
        },
        [self](const QString &error) {
            if (!self)
                return;
            self->m_applyButton->setEnabled(true);
            if (!error.isEmpty()) {
                self->m_applyStatus->setText(error);
                return;
            }
            self->m_applyBar->hide();
        });
    password.fill('\0');
}

} // namespace

QWidget *createMouseKeyboardPage()
{
    return new MouseKeyboardPage;
}
