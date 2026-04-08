#include "pages.h"
#include "monitorwidget.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>

QWidget *createMonitorsPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    auto *title = new QLabel("Monitors");
    title->setStyleSheet("color: white; font-size: 24px; font-weight: bold; margin-bottom: 20px;");
    layout->addWidget(title);
    layout->addWidget(new MonitorSetupWidget, 1);
    return page;
}
