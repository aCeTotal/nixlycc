#include "lockpage.h"
#include "../git/auth.h"
#include "../git/lockout.h"
#include "../git/style.h"
#include "../git/task.h"

#include <QHBoxLayout>
#include <QHideEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

LockPage::LockPage(const QString &title, const QString &hint,
                   std::function<QWidget *(const QByteArray &)> makePanel,
                   std::function<bool(QWidget *)> busy)
    : m_makePanel(std::move(makePanel)), m_busy(std::move(busy))
{
    auto *layout = new QVBoxLayout(this);
    auto *heading = new QLabel(title);
    heading->setStyleSheet("color: white; font-size: 24px; font-weight: bold; margin-bottom: 20px;");
    layout->addWidget(heading);

    m_stack = new QStackedWidget;
    m_stack->addWidget(buildLock(hint));
    layout->addWidget(m_stack, 1);

    m_lockoutTimer = new QTimer(this);
    m_lockoutTimer->setInterval(1000);
    connect(m_lockoutTimer, &QTimer::timeout, this, &LockPage::tickLockout);
    tickLockout();
}

QWidget *LockPage::buildLock(const QString &hint)
{
    auto *lock = new QWidget;
    auto *layout = new QVBoxLayout(lock);
    layout->setSpacing(12);
    layout->addStretch();

    auto *heading = new QLabel("This page is locked");
    heading->setStyleSheet("color: #f0f0f2; font-size: 17px; font-weight: bold;");
    heading->setAlignment(Qt::AlignHCenter);
    layout->addWidget(heading);

    auto *hintLabel = new QLabel(hint);
    hintLabel->setStyleSheet("color: #8b8f9a; font-size: 13px;");
    hintLabel->setAlignment(Qt::AlignHCenter);
    hintLabel->setWordWrap(true);
    layout->addWidget(hintLabel);

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

    m_status = makeStatus();
    m_status->setAlignment(Qt::AlignHCenter);
    layout->addWidget(m_status);
    layout->addStretch();

    connect(m_unlockButton, &QPushButton::clicked, this, &LockPage::unlock);
    connect(m_password, &QLineEdit::returnPressed, this, &LockPage::unlock);
    return lock;
}

void LockPage::tickLockout()
{
    const int left = lockoutRemaining();
    if (left <= 0) {
        m_lockoutTimer->stop();
        m_password->setEnabled(true);
        m_unlockButton->setEnabled(true);
        return;
    }
    m_lockoutTimer->start();
    m_password->setEnabled(false);
    m_unlockButton->setEnabled(false);
    m_status->setText(QString("Too many wrong passwords — try again in %1:%2.")
                          .arg(left / 60)
                          .arg(left % 60, 2, 10, QChar('0')));
}

void LockPage::unlock()
{
    if (lockoutRemaining() > 0) {
        tickLockout();
        return;
    }

    const QString typed = m_password->text();
    if (typed.isEmpty())
        return;

    m_password->setEnabled(false);
    m_unlockButton->setEnabled(false);
    m_status->setText("Checking…");

    QPointer<LockPage> self(this);
    runAsync([typed]() { return verifyPassword(typed) ? QString() : QString("Wrong password."); },
             [self, typed](const QString &error) {
                 if (!self)
                     return;
                 self->m_password->clear();
                 self->m_password->setEnabled(true);
                 self->m_unlockButton->setEnabled(true);
                 self->m_status->setText(error);
                 if (!error.isEmpty()) {
                     registerFailure();
                     self->tickLockout();
                     return;
                 }
                 clearFailures();
                 self->m_panel = self->m_makePanel(typed.toUtf8());
                 self->m_stack->addWidget(self->m_panel);
                 self->m_stack->setCurrentWidget(self->m_panel);
             });
}

void LockPage::lock()
{
    if (!m_panel || (m_busy && m_busy(m_panel)))
        return;
    m_stack->removeWidget(m_panel);
    delete m_panel;
    m_panel = nullptr;
    m_password->clear();
    m_status->clear();
    m_stack->setCurrentIndex(0);
}

void LockPage::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    lock();
}
