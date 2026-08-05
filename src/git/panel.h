#pragma once

#include <QByteArray>
#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QVBoxLayout;

struct GitService;

/* Everything behind the password prompt. Built on unlock and destroyed on
 * lock, so no key material, remote URL or password survives a lock.
 *
 * password is the secret PAM just accepted; it stays in this widget's memory
 * only, is never written to disk, and is wiped in the destructor. */
class GitPanel : public QWidget {
public:
    explicit GitPanel(const QByteArray &password);
    ~GitPanel() override;

    /* True while a repository push or a rebuild is still running — the page
     * keeps the panel alive so its progress stays visible. */
    bool busy() const;

private:
    void buildIdentity(QVBoxLayout *layout);
    void buildKey(QVBoxLayout *layout);
    void buildRepo(QVBoxLayout *layout);
    void buildAutomation(QVBoxLayout *layout);

    const GitService &service() const;
    void serviceChanged();
    void showKey();
    void generate();
    void applyModule();
    void createRepository();

    QByteArray m_password;

    QComboBox *m_service;
    QLineEdit *m_name;
    QLineEdit *m_email;
    QLineEdit *m_keyName;
    QLineEdit *m_url;
    QPlainTextEdit *m_keyBox;
    QPushButton *m_copyButton;
    QPushButton *m_generateButton;
    QPushButton *m_createButton;
    QLabel *m_keyStatus;
    QLabel *m_repoStatus;
    QLabel *m_autoStatus;
};
