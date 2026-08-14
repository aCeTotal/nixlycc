#include "pages.h"
#include "ui/lockpage.h"
#include "upgrade/panel.h"

QWidget *createUpgradePage()
{
    return new LockPage(
        "Upgrade System",
        "Enter your user password to see available updates and upgrade the system. "
        "Three wrong passwords lock the page for five minutes.",
        [](const QByteArray &password) -> QWidget * { return new UpgradePanel(password); },
        [](QWidget *panel) { return static_cast<UpgradePanel *>(panel)->busy(); });
}
