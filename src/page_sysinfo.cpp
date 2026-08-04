#include "pages.h"
#include "sysinfo/sysinfo.h"
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

static QWidget *makeCard(const SysInfoSection &section)
{
    auto *card = new QFrame;
    card->setStyleSheet("QFrame { background-color: rgba(255, 255, 255, 12); border-radius: 10px; }");
    auto *v = new QVBoxLayout(card);
    v->setContentsMargins(16, 14, 16, 14);
    v->setSpacing(10);

    auto *title = new QLabel(section.title);
    title->setStyleSheet("color: #7aa2f7; font-size: 13px; font-weight: bold; letter-spacing: 0.5px;");
    v->addWidget(title);

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(24);
    grid->setVerticalSpacing(6);
    grid->setColumnMinimumWidth(0, 130);
    grid->setColumnStretch(1, 1);
    int row = 0;
    for (const SysInfoRow &r : section.rows) {
        auto *key = new QLabel(r.label);
        key->setStyleSheet("color: #999999; font-size: 13px;");
        auto *val = new QLabel(r.value);
        val->setStyleSheet("color: white; font-size: 13px;");
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
    list->setSpacing(12);
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
