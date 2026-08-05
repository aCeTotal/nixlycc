#include "pages.h"
#include "git/auth.h"
#include "git/panel.h"
#include "git/style.h"
#include "git/task.h"

#include <QHBoxLayout>
#include <QHideEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {

/* The page is locked whenever it is not on screen: the panel is built only
 * after PAM accepts the password, and torn down again when the page hides.
 * The accepted password is kept in memory for the panel's rebuild, and is
 * wiped together with the panel. */
class GitPage : public QWidget {
public:
    GitPage();

protected:
    void hideEvent(QHideEvent *event) override;

private:
    QWidget *buildLock();
    void unlock();
    void lock();

    QStackedWidget *m_stack;
    QLineEdit *m_password;
    QPushButton *m_unlockButton;
    QLabel *m_lockStatus;
    GitPanel *m_panel = nullptr;
};

GitPage::GitPage()
{
    auto *layout = new QVBoxLayout(this);
    auto *title = new QLabel("Git");
    title->setStyleSheet("color: white; font-size: 24px; font-weight: bold; margin-bottom: 20px;");
    layout->addWidget(title);

    m_stack = new QStackedWidget;
    m_stack->addWidget(buildLock());
    layout->addWidget(m_stack, 1);
}

QWidget *GitPage::buildLock()
{
    auto *lock = new QWidget;
    auto *layout = new QVBoxLayout(lock);
    layout->setSpacing(12);
    layout->addStretch();

    auto *heading = new QLabel("This page is locked");
    heading->setStyleSheet("color: #f0f0f2; font-size: 17px; font-weight: bold;");
    heading->setAlignment(Qt::AlignHCenter);
    layout->addWidget(heading);

    auto *hint = new QLabel("Enter your user password to manage SSH keys and the nixlyOS repository.");
    hint->setStyleSheet("color: #8b8f9a; font-size: 13px;");
    hint->setAlignment(Qt::AlignHCenter);
    layout->addWidget(hint);

    m_password = makeField("Password");
    m_password->setEchoMode(QLineEdit::Password);
    m_password->setMaximumWidth(320);

    m_unlockButton = makeButton("Unlock");

    auto *row = new QHBoxLayout;
    row->setSpacing(10);
    row->addStretch();
    row->addWidget(m_password);
    row->addWidget(m_unlockButton);
    row->addStretch();
    layout->addLayout(row);

    m_lockStatus = makeStatus();
    m_lockStatus->setAlignment(Qt::AlignHCenter);
    layout->addWidget(m_lockStatus);
    layout->addStretch();

    connect(m_unlockButton, &QPushButton::clicked, this, &GitPage::unlock);
    connect(m_password, &QLineEdit::returnPressed, this, &GitPage::unlock);
    return lock;
}

void GitPage::unlock()
{
    const QString typed = m_password->text();
    if (typed.isEmpty())
        return;

    m_password->setEnabled(false);
    m_unlockButton->setEnabled(false);
    m_lockStatus->setText("Checking…");

    QPointer<GitPage> self(this);
    runAsync([typed]() { return verifyPassword(typed) ? QString() : QString("Wrong password."); },
             [self, typed](const QString &error) {
                 if (!self)
                     return;
                 self->m_password->clear();
                 self->m_password->setEnabled(true);
                 self->m_unlockButton->setEnabled(true);
                 self->m_lockStatus->setText(error);
                 if (!error.isEmpty())
                     return;
                 self->m_panel = new GitPanel(typed.toUtf8());
                 self->m_stack->addWidget(self->m_panel);
                 self->m_stack->setCurrentWidget(self->m_panel);
             });
}

void GitPage::lock()
{
    if (!m_panel || m_panel->busy())
        return;
    m_stack->removeWidget(m_panel);
    delete m_panel;
    m_panel = nullptr;
    m_password->clear();
    m_lockStatus->clear();
    m_stack->setCurrentIndex(0);
}

void GitPage::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    lock();
}

} // namespace

QWidget *createGitPage()
{
    return new GitPage;
}
