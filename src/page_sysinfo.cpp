#include "pages.h"
#include "sysinfo/sysinfo.h"
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
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

/* The whole page as plain text, for the clipboard and the saved file. */
QString sectionsAsText(const QList<SysInfoSection> &sections)
{
    QString text;
    for (const SysInfoSection &section : sections) {
        text += section.title.toUpper() + "\n";
        for (const SysInfoRow &row : section.rows)
            text += QString("  %1: %2\n").arg(row.label, row.value);
        text += "\n";
    }
    return text;
}

QPushButton *makeIconButton(const QString &glyph, const QString &tip)
{
    auto *button = new QPushButton(glyph);
    button->setToolTip(tip);
    button->setCursor(Qt::PointingHandCursor);
    button->setFixedSize(34, 34);
    button->setStyleSheet(
        "QPushButton { background-color: rgba(255, 255, 255, 14);"
        " border: 1px solid rgba(255, 255, 255, 30); border-radius: 9px;"
        " color: #f0f0f2; font-size: 15px; }"
        "QPushButton:hover { background-color: rgba(122, 162, 247, 70); }");
    return button;
}

QWidget *createSysInfoPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *title = new QLabel("System Information");
    title->setStyleSheet("color: white; font-size: 24px; font-weight: bold;");

    auto *copyButton = makeIconButton("⧉", "Copy everything to the clipboard");
    auto *saveButton = makeIconButton("⭳", "Save everything to a .txt file");
    auto *header = new QHBoxLayout;
    header->addWidget(title);
    header->addStretch();
    header->addWidget(copyButton);
    header->addWidget(saveButton);
    layout->addLayout(header);

    auto *status = new QLabel;
    status->setStyleSheet("color: #8b8f9a; font-size: 12px; margin-bottom: 8px;");
    layout->addWidget(status);

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

    const QString text = sectionsAsText(sections);
    QObject::connect(copyButton, &QPushButton::clicked, page, [text, status]() {
        QGuiApplication::clipboard()->setText(text);
        status->setText("Copied to the clipboard.");
    });
    QObject::connect(saveButton, &QPushButton::clicked, page, [page, text, status]() {
        const QString suggestion = QDir::homePath() + "/sysinfo.txt";
        const QString path =
            QFileDialog::getSaveFileName(page, "Save system information", suggestion,
                                         "Text files (*.txt)");
        if (path.isEmpty())
            return;
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            status->setText(QString("Could not write %1").arg(path));
            return;
        }
        file.write(text.toUtf8());
        status->setText(QString("Saved to %1").arg(path));
    });
    return page;
}
