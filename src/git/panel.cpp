#include "panel.h"
#include "keys.h"
#include "module.h"
#include "rebuild.h"
#include "repo.h"
#include "style.h"
#include "task.h"

#include <QCheckBox>
#include <QClipboard>
#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace {

/* The deployed module is the source of truth; the global git config is only a
 * fallback for a module that has not been generated yet. */
QString prefill(const QString &fromModule, const QString &fromConfig)
{
    return fromModule.isEmpty() ? fromConfig : fromModule;
}

} // namespace

GitPanel::GitPanel(const QByteArray &password) : m_password(password)
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 12, 0);
    layout->setSpacing(10);

    buildIdentity(layout);
    buildKey(layout);
    buildRepo(layout);
    buildSystem(layout);
    buildAutomation(layout);
    layout->addStretch();

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->viewport()->setAutoFillBackground(false);
    scroll->setWidget(content);
    outer->addWidget(scroll);
}

GitPanel::~GitPanel()
{
    m_password.fill('\0');
}

bool GitPanel::busy() const
{
    return !m_createButton->isEnabled() || !m_applyButton->isEnabled();
}

void GitPanel::buildIdentity(QVBoxLayout *layout)
{
    addSection(layout, "Identity");

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(20);
    grid->setVerticalSpacing(8);
    grid->setColumnMinimumWidth(0, 130);
    grid->setColumnStretch(1, 1);

    m_name = makeField("Your name", prefill(moduleName(), gitConfigValue("user.name")));
    m_email = makeField("you@example.com", prefill(moduleEmail(), gitConfigValue("user.email")));
    grid->addWidget(makeFieldLabel("Name"), 0, 0);
    grid->addWidget(m_name, 0, 1);
    grid->addWidget(makeFieldLabel("Email"), 1, 0);
    grid->addWidget(m_email, 1, 1);
    layout->addLayout(grid);
    layout->addSpacing(16);
}

void GitPanel::buildKey(QVBoxLayout *layout)
{
    addSection(layout, "SSH key");

    const QString key = moduleKey();
    m_keyName = makeField("Key name", key.isEmpty() ? "github" : key);
    auto *generateButton = makeButton("Generate SSH key");

    auto *row = new QHBoxLayout;
    row->setSpacing(10);
    row->addWidget(makeFieldLabel("Key name"));
    row->addWidget(m_keyName, 1);
    row->addWidget(generateButton);
    layout->addLayout(row);

    m_keyBox = new QPlainTextEdit;
    m_keyBox->setReadOnly(true);
    m_keyBox->setFixedHeight(78);
    m_keyBox->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_keyBox->setPlaceholderText("No public key yet — generate one above.");
    m_keyBox->setStyleSheet(
        "QPlainTextEdit { background-color: rgba(255, 255, 255, 16);"
        " border: 1px solid rgba(255, 255, 255, 30); border-radius: 10px;"
        " padding: 10px; color: #f0f0f2; font-family: monospace; font-size: 12px; }");
    layout->addWidget(m_keyBox);

    m_keyStatus = makeStatus();
    m_copyButton = makeButton("Copy public key");
    auto *copyRow = new QHBoxLayout;
    copyRow->addWidget(m_keyStatus, 1);
    copyRow->addWidget(m_copyButton);
    layout->addLayout(copyRow);
    layout->addSpacing(16);

    showKey();
    connect(m_keyName, &QLineEdit::textChanged, this, &GitPanel::showKey);
    connect(generateButton, &QPushButton::clicked, this, &GitPanel::generate);
    connect(m_copyButton, &QPushButton::clicked, this, [this]() {
        QGuiApplication::clipboard()->setText(m_keyBox->toPlainText());
        m_keyStatus->setText("Public key copied to clipboard.");
    });
}

void GitPanel::buildRepo(QVBoxLayout *layout)
{
    addSection(layout, "nixlyOS repository");

    auto *pathLabel = new QLabel(QString("Local path: %1").arg(repoPath()));
    pathLabel->setStyleSheet("color: #f0f0f2; font-size: 13px;");
    layout->addWidget(pathLabel);

    m_url = makeField("git@github.com:user/.nixlyos.git", repoRemote());
    m_createButton = makeButton("Create git nixlyOS-repo");

    auto *row = new QHBoxLayout;
    row->setSpacing(10);
    row->addWidget(makeFieldLabel("Repository URL"));
    row->addWidget(m_url, 1);
    row->addWidget(m_createButton);
    layout->addLayout(row);

    m_repoStatus = makeStatus();
    layout->addWidget(m_repoStatus);
    layout->addSpacing(16);

    connect(m_createButton, &QPushButton::clicked, this, &GitPanel::createRepository);
}

void GitPanel::buildSystem(QVBoxLayout *layout)
{
    addSection(layout, "System configuration");

    const QString host = moduleHost();
    m_host = makeField("github.com", host.isEmpty() ? "github.com" : host);
    m_applyButton = makeButton("Apply & rebuild");

    auto *row = new QHBoxLayout;
    row->setSpacing(10);
    row->addWidget(makeFieldLabel("SSH host"));
    row->addWidget(m_host, 1);
    row->addWidget(m_applyButton);
    layout->addLayout(row);

    m_systemStatus = makeStatus();
    m_systemStatus->setText(QString("Writes %1 and switches the system so ~/.ssh/config "
                                    "and the git identity take effect immediately.")
                                .arg(modulePath()));
    layout->addWidget(m_systemStatus);
    layout->addSpacing(16);

    connect(m_applyButton, &QPushButton::clicked, this, &GitPanel::applyAndRebuild);
}

void GitPanel::buildAutomation(QVBoxLayout *layout)
{
    addSection(layout, "Automation");

    auto *autoCommit = new QCheckBox("Auto commit changes");
    autoCommit->setChecked(autoCommitEnabled());
    autoCommit->setStyleSheet("QCheckBox { color: #f0f0f2; font-size: 14px; }"
                              "QCheckBox::indicator { width: 16px; height: 16px; }");
    layout->addWidget(autoCommit);

    m_autoStatus = makeStatus();
    m_autoStatus->setText("Commits and pushes ~/.nixlyos after every successful update.");
    layout->addWidget(m_autoStatus);

    connect(autoCommit, &QCheckBox::toggled, this, [this](bool on) {
        const QString error = setAutoCommit(on);
        if (!error.isEmpty())
            m_autoStatus->setText(error);
        else
            m_autoStatus->setText(on ? "Enabled — every successful update is committed and pushed."
                                     : "Disabled — updates leave ~/.nixlyos uncommitted.");
    });
}

void GitPanel::showKey()
{
    const QString name = m_keyName->text().trimmed();
    const QString pub = readPublicKey(name);
    m_keyBox->setPlainText(pub);
    m_copyButton->setEnabled(!pub.isEmpty());
    m_keyStatus->setText(pub.isEmpty()
                             ? QString("No key at ~/.ssh/%1").arg(name)
                             : QString("Paste this into GitHub → Settings → SSH and GPG keys."));
}

void GitPanel::generate()
{
    const QString name = m_keyName->text().trimmed();
    if (name.isEmpty()) {
        m_keyStatus->setText("Enter a key name first.");
        return;
    }
    if (keyExists(name)
        && QMessageBox::question(this, "Replace key",
                                 QString("~/.ssh/%1 already exists. Replace it?").arg(name))
            != QMessageBox::Yes)
        return;

    const QString error = generateKey(name, m_email->text().trimmed());
    showKey();
    if (!error.isEmpty())
        m_keyStatus->setText(error);
    else
        m_keyStatus->setText("Key created. Copy it to GitHub, then press Apply & rebuild.");
}

void GitPanel::createRepository()
{
    const QString url = m_url->text().trimmed();
    if (url.isEmpty()) {
        m_repoStatus->setText("Enter the repository URL first.");
        return;
    }

    m_createButton->setEnabled(false);
    m_repoStatus->setText("Creating and pushing…");

    const QString name = m_name->text().trimmed();
    const QString email = m_email->text().trimmed();
    QPointer<GitPanel> self(this);
    runAsync([url, name, email]() { return createRepo(url, name, email); },
             [self, url](const QString &error) {
                 if (!self)
                     return;
                 self->m_createButton->setEnabled(true);
                 self->m_repoStatus->setText(
                     error.isEmpty() ? QString("%1 pushed to %2").arg(repoPath(), url) : error);
             });
}

/* Writes the home-manager module and switches the system right away, so the
 * new ~/.ssh/config and git identity are live without a manual rebuild. */
void GitPanel::applyAndRebuild()
{
    const QString name = m_name->text().trimmed();
    const QString email = m_email->text().trimmed();
    const QString host = m_host->text().trimmed();
    const QString key = m_keyName->text().trimmed();
    if (name.isEmpty() || email.isEmpty() || host.isEmpty() || key.isEmpty()) {
        m_systemStatus->setText("Fill in name, email, SSH host and key name first.");
        return;
    }

    const QString error = writeModule(name, email, host, key);
    if (!error.isEmpty()) {
        m_systemStatus->setText(error);
        return;
    }

    m_applyButton->setEnabled(false);
    m_systemStatus->setText("Rebuilding…");

    QPointer<GitPanel> self(this);
    const QByteArray password = m_password;
    runAsync(
        [password, self]() {
            return rebuildSwitch(password, [self](const QString &line) {
                postToGui([self, line]() {
                    if (self)
                        self->m_systemStatus->setText(line);
                });
            });
        },
        [self](const QString &error) {
            if (!self)
                return;
            self->m_applyButton->setEnabled(true);
            self->m_systemStatus->setText(error.isEmpty()
                                              ? "Applied — ~/.ssh/config and git identity are live."
                                              : error);
        });
}
