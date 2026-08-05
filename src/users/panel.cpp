#include "panel.h"
#include "../git/rebuild.h"
#include "../git/repo.h"
#include "../git/style.h"
#include "../git/task.h"
#include "nixfiles.h"
#include "ops.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include <unistd.h>

namespace {

QLineEdit *makeSecret(const QString &placeholder)
{
    QLineEdit *field = makeField(placeholder);
    field->setEchoMode(QLineEdit::Password);
    return field;
}

QString currentUser()
{
    return QString::fromLocal8Bit(qgetenv("USER"));
}

} // namespace

UserPanel::UserPanel(const QByteArray &password) : m_password(password)
{
    m_accounts = declaredAccounts();

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 12, 0);
    layout->setSpacing(10);

    buildAccounts(layout);
    buildPassword(layout);
    buildNewUser(layout);
    buildApply(layout);
    layout->addStretch();

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->viewport()->setAutoFillBackground(false);
    scroll->setWidget(content);
    outer->addWidget(scroll);

    selectAccount(0);
}

UserPanel::~UserPanel()
{
    m_password.fill('\0');
}

bool UserPanel::busy() const
{
    return !m_applyButton->isEnabled();
}

void UserPanel::buildAccounts(QVBoxLayout *layout)
{
    addSection(layout, "Accounts");

    QStringList names;
    for (const Account &account : m_accounts)
        names << account.name;
    if (names.isEmpty())
        names << "No users declared";

    m_account = makeCombo(names);
    m_accountInfo = makeStatus();

    auto *row = new QHBoxLayout;
    row->setSpacing(10);
    row->addWidget(makeFieldLabel("User"));
    row->addWidget(m_account, 1);
    layout->addLayout(row);
    layout->addWidget(m_accountInfo);

    m_rename = makeField("New user name");
    auto *renameRow = new QHBoxLayout;
    renameRow->setSpacing(10);
    renameRow->addWidget(makeFieldLabel("Rename to"));
    renameRow->addWidget(m_rename, 1);
    layout->addLayout(renameRow);

    m_renameInfo = makeStatus();
    m_renameInfo->setText("The name is replaced in every .nix file that uses it, and usermod "
                          "moves the home directory. The account you are logged in as cannot be "
                          "renamed from its own session.");
    layout->addWidget(m_renameInfo);
    layout->addSpacing(16);

    connect(m_account, &QComboBox::currentIndexChanged, this,
            [this](int index) { selectAccount(index); });
    connect(m_rename, &QLineEdit::textChanged, this, [this](const QString &text) {
        const QString from = m_account->currentText();
        if (text.trimmed().isEmpty() || text.trimmed() == from) {
            m_renameInfo->setText("The name is replaced in every .nix file that uses it.");
            return;
        }
        m_renameInfo->setText(QString("%1 → %2 touches %3 files in ~/.nixlyos.")
                                  .arg(from, text.trimmed())
                                  .arg(filesMentioning(from).size()));
    });
}

void UserPanel::buildPassword(QVBoxLayout *layout)
{
    addSection(layout, "Password");

    m_newPassword = makeSecret("New password");
    m_confirm = makeSecret("Repeat password");

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(20);
    grid->setVerticalSpacing(8);
    grid->setColumnMinimumWidth(0, 130);
    grid->setColumnStretch(1, 1);
    grid->addWidget(makeFieldLabel("New password"), 0, 0);
    grid->addWidget(m_newPassword, 0, 1);
    grid->addWidget(makeFieldLabel("Repeat"), 1, 0);
    grid->addWidget(m_confirm, 1, 1);
    layout->addLayout(grid);

    auto *hint = makeStatus();
    hint->setText("Set on the selected account. Leave empty to keep the current password.");
    layout->addWidget(hint);
    layout->addSpacing(16);
}

void UserPanel::buildNewUser(QVBoxLayout *layout)
{
    addSection(layout, "New user");

    m_newName = makeField("username");
    m_newDescription = makeField("Full name");
    m_newUserPassword = makeSecret("Password");
    m_newAdmin = new QCheckBox("Administrator (wheel — may use sudo)");
    m_newAdmin->setStyleSheet("QCheckBox { color: #f0f0f2; font-size: 13px; }"
                              "QCheckBox::indicator { width: 16px; height: 16px; }");

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(20);
    grid->setVerticalSpacing(8);
    grid->setColumnMinimumWidth(0, 130);
    grid->setColumnStretch(1, 1);
    grid->addWidget(makeFieldLabel("User name"), 0, 0);
    grid->addWidget(m_newName, 0, 1);
    grid->addWidget(makeFieldLabel("Description"), 1, 0);
    grid->addWidget(m_newDescription, 1, 1);
    grid->addWidget(makeFieldLabel("Password"), 2, 0);
    grid->addWidget(m_newUserPassword, 2, 1);
    layout->addLayout(grid);
    layout->addWidget(m_newAdmin);

    auto *hint = makeStatus();
    hint->setText("Gets the same home-manager configuration as the existing users, so the new "
                  "account has the full desktop on first login.");
    layout->addWidget(hint);
    layout->addSpacing(16);
}

void UserPanel::buildApply(QVBoxLayout *layout)
{
    m_status = makeStatus();
    m_applyButton = makeButton("Apply");

    auto *row = new QHBoxLayout;
    row->addWidget(m_status, 1);
    row->addWidget(m_applyButton);
    layout->addLayout(row);

    connect(m_applyButton, &QPushButton::clicked, this, &UserPanel::apply);
}

void UserPanel::selectAccount(int index)
{
    if (index < 0 || index >= m_accounts.size()) {
        m_accountInfo->setText(QString());
        return;
    }

    const Account &account = m_accounts[index];
    QStringList facts;
    facts << (account.uid >= 0 ? QString("uid %1").arg(account.uid) : QString("not built yet"));
    if (!account.home.isEmpty())
        facts << account.home;
    if (account.admin)
        facts << "administrator";
    if (account.name == currentUser())
        facts << "this session";
    m_accountInfo->setText(facts.join(" · "));
    m_rename->clear();
}

/* Files first, then the system: usermod runs before the rebuild so the account
 * keeps its uid, and passwords are set after it so a brand new user exists. */
void UserPanel::apply()
{
    const QString selected = m_account->currentText();
    const QString rename = m_rename->text().trimmed();
    const QString secret = m_newPassword->text();
    const QString confirm = m_confirm->text();
    const QString newName = m_newName->text().trimmed();
    const QString newSecret = m_newUserPassword->text();
    const QString newDescription = m_newDescription->text().trimmed();
    const bool newAdmin = m_newAdmin->isChecked();

    if (!secret.isEmpty() && secret != confirm) {
        m_status->setText("The two passwords do not match.");
        return;
    }
    if (!newName.isEmpty() && newSecret.isEmpty()) {
        m_status->setText("Give the new user a password.");
        return;
    }
    if (rename.isEmpty() && secret.isEmpty() && newName.isEmpty()) {
        m_status->setText("Nothing to apply.");
        return;
    }
    if (!rename.isEmpty() && selected == currentUser()) {
        m_status->setText("Log in as another administrator to rename this account — usermod "
                          "refuses while the user has running processes.");
        return;
    }

    m_applyButton->setEnabled(false);
    m_status->setText("Applying…");

    QPointer<UserPanel> self(this);
    const QByteArray password = m_password;
    runAsync(
        [self, password, selected, rename, secret, newName, newSecret, newDescription,
         newAdmin]() {
            auto progress = [self](const QString &line) {
                postToGui([self, line]() {
                    if (self)
                        self->m_status->setText(line);
                });
            };

            if (!rename.isEmpty()) {
                progress(QString("Renaming %1 to %2…").arg(selected, rename));
                QString error = renameUserInFiles(selected, rename);
                if (error.isEmpty())
                    error = renameSystemUser(password, selected, rename);
                if (!error.isEmpty())
                    return error;
            }

            if (!newName.isEmpty()) {
                progress(QString("Declaring %1…").arg(newName));
                const QString error = addUserToFiles(newName, newDescription, newAdmin);
                if (!error.isEmpty())
                    return error;
            }

            progress("Rebuilding…");
            QString error = rebuildSwitch(password, progress);
            if (!error.isEmpty())
                return error;

            const QString target = rename.isEmpty() ? selected : rename;
            if (!secret.isEmpty()) {
                progress(QString("Setting the password for %1…").arg(target));
                error = setUserPassword(password, target, secret);
                if (!error.isEmpty())
                    return error;
            }
            if (!newName.isEmpty()) {
                progress(QString("Setting the password for %1…").arg(newName));
                error = setUserPassword(password, newName, newSecret);
                if (!error.isEmpty())
                    return error;
            }

            return commitAndPush("Changes User Settings");
        },
        [self](const QString &error) {
            if (!self)
                return;
            self->m_applyButton->setEnabled(true);
            if (!error.isEmpty()) {
                self->m_status->setText(error);
                return;
            }
            self->m_newPassword->clear();
            self->m_confirm->clear();
            self->m_newUserPassword->clear();
            self->m_newName->clear();
            self->m_newDescription->clear();
            self->m_rename->clear();
            self->m_accounts = declaredAccounts();
            self->m_status->setText("Applied — accounts rebuilt, committed and pushed.");
        });
}
