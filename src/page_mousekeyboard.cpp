#include "pages.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>

QWidget *createMouseKeyboardPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    auto *title = new QLabel("Mouse & Keyboard");
    title->setStyleSheet("color: white; font-size: 24px; font-weight: bold; margin-bottom: 20px;");
    layout->addWidget(title);
    auto *body = new QLabel("Mouse and keyboard configuration will appear here.");
    body->setStyleSheet("color: #cccccc; font-size: 16px;");
    body->setWordWrap(true);
    layout->addWidget(body);
    layout->addStretch();
    return page;
}
