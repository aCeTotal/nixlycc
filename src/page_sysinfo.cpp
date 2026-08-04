#include "pages.h"
#include "sysinfo/sysinfo.h"
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

static QWidget *makeCard(const SysInfoSection &section)
{
    /* No card background — the rows sit directly on the window's
     * translucent background, separated only by a hairline rule. */
    auto *card = new QWidget;
    auto *v = new QVBoxLayout(card);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(9);

    auto *title = new QLabel(section.title.toUpper());
    title->setStyleSheet("color: #7aa2f7; font-size: 11px; font-weight: bold; letter-spacing: 1.5px;");
    v->addWidget(title);

    auto *rule = new QFrame;
    rule->setFixedHeight(1);
    rule->setStyleSheet("background-color: rgba(255, 255, 255, 30);");
    v->addWidget(rule);

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(28);
    grid->setVerticalSpacing(9);
    grid->setColumnMinimumWidth(0, 150);
    grid->setColumnStretch(1, 1);
    int row = 0;
    for (const SysInfoRow &r : section.rows) {
        auto *key = new QLabel(r.label);
        key->setStyleSheet("color: #8b8f9a; font-size: 13px;");
        auto *val = new QLabel(r.value);
        val->setStyleSheet("color: #f0f0f2; font-size: 13px; font-weight: 500;");
        val->setWordWrap(true);
        val->setTextInteractionFlags(Qt::TextSelectableByMouse);
        grid->addWidget(key, row, 0, Qt::AlignTop);
        grid->addWidget(val, row, 1);
        ++row;
    }
    v->addLayout(grid);
    return card;
}

QWidget *createSysInfoPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    auto *title = new QLabel("System Information");
    title->setStyleSheet("color: white; font-size: 24px; font-weight: bold; margin-bottom: 20px;");
    layout->addWidget(title);

    auto *container = new QWidget;
    container->setAutoFillBackground(false);
    auto *list = new QVBoxLayout(container);
    list->setContentsMargins(0, 0, 12, 0);
    list->setSpacing(26);
    const QList<SysInfoSection> sections = collectSysInfo();
    for (const SysInfoSection &section : sections)
        list->addWidget(makeCard(section));
    list->addStretch();

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->viewport()->setAutoFillBackground(false);
    scroll->setWidget(container);
    layout->addWidget(scroll, 1);
    return page;
}
