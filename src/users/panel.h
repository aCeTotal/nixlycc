#pragma once

#include "accounts.h"

#include <QByteArray>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QVBoxLayout;

/* Everything behind the password prompt on the User Settings page: rename an
 * account, set a password, add a user. The declaration in ~/.nixlyos is
 * rewritten and the system is switched onto it when you press Apply.
 *
 * password is the secret PAM just accepted; it stays in memory, drives sudo,
 * and is wiped in the destructor. */
class UserPanel : public QWidget {
public:
    explicit UserPanel(const QByteArray &password);
    ~UserPanel() override;

    /* True while a rebuild is running, so the page is not torn down under it. */
    bool busy() const;

private:
    void buildAccounts(QVBoxLayout *layout);
    void buildPassword(QVBoxLayout *layout);
    void buildNewUser(QVBoxLayout *layout);
    void buildApply(QVBoxLayout *layout);
    void selectAccount(int index);
    void apply();

    QByteArray m_password;
    QList<Account> m_accounts;

    QComboBox *m_account;
    QLabel *m_accountInfo;
    QLineEdit *m_rename;
    QLabel *m_renameInfo;
    QLineEdit *m_newPassword;
    QLineEdit *m_confirm;
    QLineEdit *m_newName;
    QLineEdit *m_newDescription;
    QCheckBox *m_newAdmin;
    QLineEdit *m_newUserPassword;
    QPushButton *m_applyButton;
    QLabel *m_status;
};
