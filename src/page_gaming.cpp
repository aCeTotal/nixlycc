#include "pages.h"
#include "gaming/pttbind.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>

QWidget *createGamingPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    auto *title = new QLabel("Gaming");
    title->setStyleSheet("color: white; font-size: 24px; font-weight: bold; margin-bottom: 20px;");
    layout->addWidget(title);
    layout->addWidget(new PttBindWidget);
    layout->addStretch();
    return page;
}
