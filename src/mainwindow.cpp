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
        "QListWidget { background-color: rgba(18, 18, 18, 248); color: white; border: none; }"
        "QListWidget::item { padding: 10px; }"
        "QListWidget::item:selected { background-color: #2A2A2A; }"
        "QListWidget::item:hover { background-color: rgba(30, 30, 30, 248); }");
    menu->addItem("System Information");
    menu->addItem("User Settings");
    menu->addItem("Monitors");
    menu->addItem("Applications");
    menu->addItem("Mouse & Keyboard");
    menu->addItem("Shortcuts");
    menu->addItem("Statusbar & Tiling");
    menu->addItem("Neovim IDE");
    menu->addItem("Storage & Sharing");
    menu->addItem("Firewall & Security");
    menu->addItem("Backup");
    menu->addItem("Gaming");
    menu->addItem("WinVM Settings");
    menu->addItem("System Cleanup");
    sideLayout->addWidget(menu);

    /* Content area */
    auto *content = new QWidget;
    content->setAutoFillBackground(true);
    auto pal = content->palette();
    pal.setColor(QPalette::Window, QColor(26, 26, 26, 235));
    content->setPalette(pal);

    auto *stack = new QStackedWidget;
    stack->addWidget(createSysInfoPage());
    stack->addWidget(createUserSettingsPage());
    stack->addWidget(createMonitorsPage());
    stack->addWidget(createApplicationsPage());
    stack->addWidget(createMouseKeyboardPage());
    stack->addWidget(createShortcutsPage());
    stack->addWidget(createStatusbarPage());
    stack->addWidget(createNeovimPage());
    stack->addWidget(createStoragePage());
    stack->addWidget(createFirewallPage());
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
