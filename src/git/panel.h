#pragma once

#include <QByteArray>
#include <QWidget>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QVBoxLayout;

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
    void buildSystem(QVBoxLayout *layout);
    void buildAutomation(QVBoxLayout *layout);

    void showKey();
    void generate();
    void createRepository();
    void applyAndRebuild();

    QByteArray m_password;

    QLineEdit *m_name;
    QLineEdit *m_email;
    QLineEdit *m_keyName;
    QLineEdit *m_host;
    QLineEdit *m_url;
    QPlainTextEdit *m_keyBox;
    QPushButton *m_copyButton;
    QPushButton *m_createButton;
    QPushButton *m_applyButton;
    QLabel *m_keyStatus;
    QLabel *m_repoStatus;
    QLabel *m_systemStatus;
    QLabel *m_autoStatus;
};
