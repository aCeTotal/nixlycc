#include "pages.h"
#include "ui/lockpage.h"
#include "users/panel.h"

QWidget *createUserSettingsPage()
{
    return new LockPage(
        "User Settings",
        "Enter your user password to rename accounts, change passwords and add users. "
        "Three wrong passwords lock the page for five minutes.",
        [](const QByteArray &password) -> QWidget * { return new UserPanel(password); },
        [](QWidget *panel) { return static_cast<UserPanel *>(panel)->busy(); });
}
