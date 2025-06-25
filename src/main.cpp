#include <QApplication>
#include <QMainWindow>
#include <QScreen>
#include <QGuiApplication>
#include <QWindow>
#include <QtCore>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPalette>
#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QFont>

class MainWindow : public QMainWindow
{
public:
    MainWindow(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        // Set window title
        setWindowTitle("NixlyCC");
        
        // Set window flags for tiling window managers
        setWindowFlags(Qt::Window | Qt::WindowTitleHint | 
                      Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint);
        
        // Disable resizing by the user (let the tiling WM handle it)
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        
        // Set window attributes
        setAttribute(Qt::WA_AcceptTouchEvents);
        setAttribute(Qt::WA_OpaquePaintEvent);
        setAttribute(Qt::WA_NoSystemBackground);
        
        // Set initial size - will be managed by tiling WM
        resize(800, 600);
        
        // Create central widget and main layout
        QWidget *centralWidget = new QWidget(this);
        QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);
        
        // Create left menu panel (200px wide with dark background)
        QWidget *leftPanel = new QWidget();
        leftPanel->setFixedWidth(200);
        leftPanel->setAutoFillBackground(true);
        
        // Set left panel background color to #121212 (RGB: 18, 18, 18)
        QPalette leftPalette = leftPanel->palette();
        leftPalette.setColor(QPalette::Window, QColor(18, 18, 18));
        leftPanel->setPalette(leftPalette);
        
        // Create menu list for the left panel
        QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
        leftLayout->setContentsMargins(0, 0, 0, 0);
        
        QListWidget *menuList = new QListWidget();
        
        // Set a larger font for the menu items
        QFont menuFont = menuList->font();
        menuFont.setPointSize(10); // Explicitly set point size (equivalent to ~21-24px)
        menuFont.setBold(false);    // Make the font bold for better visibility
        menuList->setFont(menuFont);
        
        menuList->setStyleSheet(
            "QListWidget { background-color: #121212; color: white; border: none; }"
            "QListWidget::item { padding: 10px; }"
            "QListWidget::item:selected { background-color: #2A2A2A; }"
            "QListWidget::item:hover { background-color: #1E1E1E; }"
        );
        
        // Add menu items
        menuList->addItem("System Information");
        menuList->addItem("User Settings");
        menuList->addItem("Monitors");
        menuList->addItem("Applications");
        menuList->addItem("Mouse & Keyboard");
        menuList->addItem("Shortcuts");
        menuList->addItem("Statusbar & Tiling");
        menuList->addItem("Neovim IDE");
        menuList->addItem("Storage & Sharing");
        menuList->addItem("Firewall & Security");
        menuList->addItem("Backup");
        menuList->addItem("Gaming");
        menuList->addItem("WinVM Settings");
        menuList->addItem("SystemCleanup");
        
        leftLayout->addWidget(menuList);
        
        // Create right content area with slightly lighter background
        QWidget *rightPanel = new QWidget();
        rightPanel->setAutoFillBackground(true);
        
        // Set right panel background color to #1A1A1A (RGB: 26, 26, 26)
        QPalette rightPalette = rightPanel->palette();
        rightPalette.setColor(QPalette::Window, QColor(26, 26, 26));
        rightPanel->setPalette(rightPalette);
        
        // Create stacked widget for content pages
        QStackedWidget *contentStack = new QStackedWidget();
        
        // Function to create a content page with a title
        auto createPage = [](const QString &title) {
            QWidget *page = new QWidget();
            QVBoxLayout *layout = new QVBoxLayout(page);
            
            // Add title
            QLabel *titleLabel = new QLabel(title);
            titleLabel->setStyleSheet("color: white; font-size: 24px; font-weight: bold; margin-bottom: 20px;");
            layout->addWidget(titleLabel);
            
            // Add content placeholder
            QLabel *contentLabel = new QLabel(title + " settings and configuration options will appear here.");
            contentLabel->setStyleSheet("color: #cccccc; font-size: 16px;");
            contentLabel->setWordWrap(true);
            layout->addWidget(contentLabel);
            
            layout->addStretch();
            return page;
        };
        
        // Create pages for each menu item
        QWidget *sysInfoPage = createPage("System Information");
        QWidget *userSettingsPage = createPage("User Settings");
        QWidget *monitorsPage = createPage("Monitors");
        QWidget *applicationsPage = createPage("Applications");
        QWidget *mouseKeyboardPage = createPage("Mouse & Keyboard");
        QWidget *shortcutsPage = createPage("Shortcuts");
        QWidget *statusbarTilingPage = createPage("Statusbar & Tiling");
        QWidget *neovimIDEPage = createPage("Neovim IDE");
        QWidget *storageSharePage = createPage("Storage & Sharing");
        QWidget *firewallSecurityPage = createPage("Firewall & Security");
        QWidget *backupPage = createPage("Backup");
        QWidget *gamingPage = createPage("Gaming");
        QWidget *winVMPage = createPage("WinVM Settings");
        QWidget *systemCleanupPage = createPage("SystemCleanup");
        
        // Add pages to stack
        contentStack->addWidget(sysInfoPage);
        contentStack->addWidget(userSettingsPage);
        contentStack->addWidget(monitorsPage);
        contentStack->addWidget(applicationsPage);
        contentStack->addWidget(mouseKeyboardPage);
        contentStack->addWidget(shortcutsPage);
        contentStack->addWidget(statusbarTilingPage);
        contentStack->addWidget(neovimIDEPage);
        contentStack->addWidget(storageSharePage);
        contentStack->addWidget(firewallSecurityPage);
        contentStack->addWidget(backupPage);
        contentStack->addWidget(gamingPage);
        contentStack->addWidget(winVMPage);
        contentStack->addWidget(systemCleanupPage);
        
        // Layout for right panel
        QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
        rightLayout->setContentsMargins(20, 20, 20, 20);
        rightLayout->addWidget(contentStack);
        
        // Connect menu selection to content stack
        connect(menuList, &QListWidget::currentRowChanged, contentStack, &QStackedWidget::setCurrentIndex);
        menuList->setCurrentRow(0); // Select first item by default
        
        // Add panels to main layout
        mainLayout->addWidget(leftPanel);
        mainLayout->addWidget(rightPanel, 1); // Give right panel expanding space
        
        // Set central widget
        setCentralWidget(centralWidget);
    }
    
    void showEvent(QShowEvent *event) override
    {
        QMainWindow::showEvent(event);
        
        // Set X11/Wayland window properties for tiling window managers
        if (windowHandle()) {
            // Tell the window manager this window should be tiled, not floating
            windowHandle()->setProperty("_NET_WM_WINDOW_TYPE", "_NET_WM_WINDOW_TYPE_NORMAL");
            
            // Disable window decorations if your WM supports it
            windowHandle()->setFlag(Qt::FramelessWindowHint, false);
        }
    }
};

int main(int argc, char *argv[])
{
    // We need to set these environment variables before QApplication is created
    
    // Always use Wayland platform
    qInfo("Setting up Wayland platform exclusively");
    
    // Clear any existing QT_QPA_PLATFORM setting
    qunsetenv("QT_QPA_PLATFORM");
    
    // Find Qt plugins directory
    const char* qtPluginPath = getenv("QT_PLUGIN_PATH");
    if (!qtPluginPath || strlen(qtPluginPath) == 0) {
        // If not set, try to find it in a standard location
        qputenv("QT_PLUGIN_PATH", "/nix/store/l9kvcx3wna1bla7xpy1629hawjqmna4y-qtbase-6.9.0/lib/qt-6/plugins");
        qInfo("Set QT_PLUGIN_PATH to Qt6 plugins directory");
    }
    
    // Set Wayland platform and shell integration paths
    qputenv("QT_QPA_PLATFORM", "wayland");
    qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");
    
    // Set the shell integration path to the one we found
    qputenv("QT_WAYLAND_SHELL_INTEGRATION", "xdg-shell"); // Explicitly request xdg-shell
    qputenv("QT_WAYLAND_SHELL_INTEGRATION_PLUGIN_PATH", "/nix/store/53d22iacdvx0whdbg4qb88bbhlm6lw2f-qtwayland-6.9.0/lib/qt-6/plugins/wayland-shell-integration");
    
    // Also set general plugin path to include qtwayland
    const char* existingPluginPath = getenv("QT_PLUGIN_PATH");
    QString pluginPath;
    if (existingPluginPath) {
        pluginPath = QString(existingPluginPath) + ":/nix/store/53d22iacdvx0whdbg4qb88bbhlm6lw2f-qtwayland-6.9.0/lib/qt-6/plugins";
    } else {
        pluginPath = "/nix/store/53d22iacdvx0whdbg4qb88bbhlm6lw2f-qtwayland-6.9.0/lib/qt-6/plugins";
    }
    qputenv("QT_PLUGIN_PATH", pluginPath.toUtf8());
    qInfo("Set QT_PLUGIN_PATH to include Wayland plugins: %s", qgetenv("QT_PLUGIN_PATH").constData());
    
    // Enable debugging for Qt platform plugins
    qputenv("QT_DEBUG_PLUGINS", "1");
    
    // Set application attributes before creating QApplication
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    
    QApplication app(argc, argv);
    
    // Set application metadata
    app.setApplicationName("nixlycc");
    app.setApplicationDisplayName("NixlyCC");
    app.setApplicationVersion("0.1");
    app.setDesktopFileName("nixlycc");
    
    // Create and show the main window
    MainWindow window;
    window.show();
    
    return app.exec();
}
