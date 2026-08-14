#pragma once

#include <QByteArray>
#include <QList>
#include <QWidget>

class QLabel;
class QProgressBar;
class QPushButton;
class QVBoxLayout;
struct UpdateEntry;

/* The unlocked half of the Upgrade System page: the list of packages with an
 * update available, and the button that runs nix flake update plus
 * nixos-rebuild switch with the password LockPage collected. */
class UpgradePanel : public QWidget {
public:
    explicit UpgradePanel(const QByteArray &password);

    /* True while the upgrade runs, so LockPage does not tear the panel down
     * mid-rebuild. */
    bool busy() const { return m_busy; }

private:
    void rescan();
    void clearRows();
    void addRow(const UpdateEntry &entry);
    void upgrade();

    QByteArray m_password;
    QLabel *m_status;
    QVBoxLayout *m_list;
    QList<QWidget *> m_rows;
    QLabel *m_progressText;
    QProgressBar *m_progress;
    QPushButton *m_upgradeButton;
    bool m_busy = false;
};
