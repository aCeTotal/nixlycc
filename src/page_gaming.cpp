#include "pages.h"
#include "gaming/pttbind.h"
#include "gaming/ptmlist.h"
#include "gaming/rendermode.h"
#include "git/style.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <iterator>

namespace {

QWidget *createVoipPanel()
{
    auto *panel = new QWidget;
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 10, 0, 0);
    layout->setSpacing(28);
    layout->addWidget(new PttBindWidget);
    layout->addWidget(new PtmListWidget);
    layout->addStretch();
    return panel;
}

QWidget *createPerformancePanel()
{
    auto *panel = new QWidget;
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 10, 0, 0);
    layout->addWidget(new RenderModeWidget);
    layout->addStretch();
    return panel;
}

} // namespace

QWidget *createGamingPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setSpacing(10);

    auto *title = new QLabel("Gaming");
    title->setStyleSheet("color: white; font-size: 24px; font-weight: bold; margin-bottom: 10px;");
    layout->addWidget(title);

    auto *stack = new QStackedWidget;
    stack->addWidget(createVoipPanel());
    stack->addWidget(createPerformancePanel());

    auto *group = new QButtonGroup(page);
    auto *tabs = new QHBoxLayout;
    tabs->setSpacing(10);
    const QString names[] = {"VOIP", "Performance"};
    for (int i = 0; i < int(std::size(names)); i++) {
        QPushButton *tab = makeTab(names[i]);
        group->addButton(tab, i);
        tabs->addWidget(tab);
    }
    tabs->addStretch();
    group->button(0)->setChecked(true);
    QObject::connect(group, &QButtonGroup::idClicked, stack, &QStackedWidget::setCurrentIndex);

    layout->addLayout(tabs);
    layout->addWidget(stack, 1);
    return page;
}
