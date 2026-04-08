#include "pages.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>

QWidget *createApplicationsPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    auto *title = new QLabel("Applications");
    title->setStyleSheet("color: white; font-size: 24px; font-weight: bold; margin-bottom: 20px;");
    layout->addWidget(title);
    auto *body = new QLabel("Application management and settings will appear here.");
    body->setStyleSheet("color: #cccccc; font-size: 16px;");
    body->setWordWrap(true);
    layout->addWidget(body);
    layout->addStretch();
    return page;
}
