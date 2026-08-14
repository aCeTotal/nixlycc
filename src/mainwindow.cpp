#include "mainwindow.h"
#include "pages.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QFont>

MainWindow::MainWindow(const QString &page, QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("NixlyCC");
    resize(800, 600);
    setAttribute(Qt::WA_TranslucentBackground);

    auto *central = new QWidget(this);
    central->setAutoFillBackground(true);
    auto centralPal = central->palette();
    centralPal.setColor(QPalette::Window, QColor(18, 18, 18, 235));
    central->setPalette(centralPal);
    auto *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    /* Sidebar */
    auto *sidebar = new QWidget;
    sidebar->setFixedWidth(200);
    auto *sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(0, 0, 0, 0);

    auto *menu = new QListWidget;
    QFont menuFont = menu->font();
    menuFont.setPointSize(10);
    menu->setFont(menuFont);
    menu->setStyleSheet(
        "QListWidget { background-color: transparent; color: white; border: none; }"
        "QListWidget::item { padding: 10px; }"
        "QListWidget::item:selected { background-color: rgba(102, 179, 255, 90); }"
        "QListWidget::item:hover { background-color: rgba(255, 255, 255, 45); }");
    menu->addItem("System Information");
    menu->addItem("Upgrade System");
    menu->addItem("User Settings");
    menu->addItem("Monitors");
    menu->addItem("Applications");
    menu->addItem("Mouse & Keyboard");
    menu->addItem("Shortcuts");
    menu->addItem("Statusbar & Tiling");
    menu->addItem("Neovim IDE");
    menu->addItem("Storage & Sharing");
    menu->addItem("Firewall & Security");
    menu->addItem("Git");
    menu->addItem("Backup");
    menu->addItem("Gaming");
    menu->addItem("WinVM Settings");
    menu->addItem("System Cleanup");
    sideLayout->addWidget(menu);

    /* Content area */
    auto *content = new QWidget;

    auto *stack = new QStackedWidget;
    stack->addWidget(createSysInfoPage());
    stack->addWidget(createUpgradePage());
    stack->addWidget(createUserSettingsPage());
    stack->addWidget(createMonitorsPage());
    stack->addWidget(createApplicationsPage());
    stack->addWidget(createMouseKeyboardPage());
    stack->addWidget(createShortcutsPage());
    stack->addWidget(createStatusbarPage());
    stack->addWidget(createNeovimPage());
    stack->addWidget(createStoragePage());
    stack->addWidget(createFirewallPage());
    stack->addWidget(createGitPage());
    stack->addWidget(createBackupPage());
    stack->addWidget(createGamingPage());
    stack->addWidget(createWinVMPage());
    stack->addWidget(createCleanupPage());

    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(20, 20, 20, 20);
    contentLayout->addWidget(stack);

    connect(menu, &QListWidget::currentRowChanged, stack, &QStackedWidget::setCurrentIndex);
    menu->setCurrentRow(0);

    if (!page.isEmpty()) {
        for (int i = 0; i < menu->count(); ++i) {
            if (menu->item(i)->text().contains(page, Qt::CaseInsensitive)) {
                menu->setCurrentRow(i);
                break;
            }
        }
    }

    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(content, 1);
    setCentralWidget(central);
}
