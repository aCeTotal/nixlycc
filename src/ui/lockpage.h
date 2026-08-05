#pragma once

#include <QByteArray>
#include <QWidget>
#include <functional>

class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QTimer;

/* A page that stays locked until PAM accepts the user's password, exactly as
 * the Git page does: the panel is built only after the password is accepted,
 * torn down when the page hides, and three wrong attempts lock the prompt for
 * five minutes.
 *
 * makePanel receives the accepted password so the panel can drive sudo; busy
 * is asked before tearing the panel down, so a running rebuild is not killed
 * by switching page. */
class LockPage : public QWidget {
public:
    LockPage(const QString &title, const QString &hint,
             std::function<QWidget *(const QByteArray &)> makePanel,
             std::function<bool(QWidget *)> busy = {});

protected:
    void hideEvent(QHideEvent *event) override;

private:
    QWidget *buildLock(const QString &hint);
    void unlock();
    void lock();
    void tickLockout();

    std::function<QWidget *(const QByteArray &)> m_makePanel;
    std::function<bool(QWidget *)> m_busy;
    QStackedWidget *m_stack;
    QLineEdit *m_password;
    QPushButton *m_unlockButton;
    QLabel *m_status;
    QTimer *m_lockoutTimer;
    QWidget *m_panel = nullptr;
};
