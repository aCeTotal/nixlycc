#include "panel.h"
#include "changelog.h"
#include "run.h"
#include "scan.h"
#include "../git/style.h"
#include "../git/task.h"

#include <QDesktopServices>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QUrl>
#include <QVBoxLayout>

namespace {

QPushButton *makeRowButton(const QString &text)
{
    auto *button = new QPushButton(text);
    button->setCursor(Qt::PointingHandCursor);
    button->setStyleSheet(
        "QPushButton { background-color: rgba(255, 255, 255, 14);"
        " border: 1px solid rgba(255, 255, 255, 30); border-radius: 8px;"
        " padding: 5px 12px; color: #f0f0f2; font-size: 12px; }"
        "QPushButton:hover { background-color: rgba(122, 162, 247, 70); }"
        "QPushButton:disabled { color: #8b8f9a; }");
    return button;
}

} // namespace

UpgradePanel::UpgradePanel(const QByteArray &password) : m_password(password)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    m_status = makeStatus();
    layout->addWidget(m_status);

    auto *container = new QWidget;
    container->setAutoFillBackground(false);
    m_list = new QVBoxLayout(container);
    m_list->setContentsMargins(0, 0, 12, 0);
    m_list->setSpacing(8);
    m_list->addStretch();

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->viewport()->setAutoFillBackground(false);
    scroll->setWidget(container);
    layout->addWidget(scroll, 1);

    m_progressText = new QLabel;
    m_progressText->setStyleSheet("color: #8b8f9a; font-size: 13px;");
    m_progressText->setVisible(false);
    layout->addWidget(m_progressText);

    m_progress = new QProgressBar;
    m_progress->setRange(0, 100);
    m_progress->setTextVisible(false);
    m_progress->setFixedHeight(10);
    m_progress->setStyleSheet(
        "QProgressBar { background-color: rgba(255, 255, 255, 14); border: none;"
        " border-radius: 5px; }"
        "QProgressBar::chunk { background-color: #7aa2f7; border-radius: 5px; }");
    m_progress->setVisible(false);
    layout->addWidget(m_progress);

    m_upgradeButton = new QPushButton("Upgrade system now");
    m_upgradeButton->setCursor(Qt::PointingHandCursor);
    m_upgradeButton->setMinimumHeight(52);
    m_upgradeButton->setStyleSheet(
        "QPushButton { background-color: rgba(122, 162, 247, 45);"
        " border: 1px solid rgba(122, 162, 247, 120); border-radius: 12px;"
        " color: #f0f0f2; font-size: 16px; font-weight: bold; }"
        "QPushButton:hover { background-color: rgba(122, 162, 247, 80); }"
        "QPushButton:disabled { color: #8b8f9a; border-color: rgba(255, 255, 255, 30);"
        " background-color: rgba(255, 255, 255, 12); }");
    m_upgradeButton->setEnabled(false);
    connect(m_upgradeButton, &QPushButton::clicked, this, [this]() { upgrade(); });
    layout->addWidget(m_upgradeButton);

    rescan();
}

void UpgradePanel::rescan()
{
    clearRows();
    m_upgradeButton->setEnabled(false);

    QPointer<UpgradePanel> self(this);
    scanForUpdates(
        [self](const QString &text) {
            if (self)
                self->m_status->setText(text);
        },
        [self](const QList<UpdateEntry> &updates, const QString &error) {
            if (!self)
                return;
            if (!error.isEmpty()) {
                self->m_status->setText(error);
                return;
            }
            for (const UpdateEntry &entry : updates)
                self->addRow(entry);
            self->m_status->setText(
                updates.isEmpty() ? "The system is up to date."
                                  : QString("%1 update%2 available.")
                                        .arg(updates.size())
                                        .arg(updates.size() == 1 ? "" : "s"));
            self->m_upgradeButton->setEnabled(!updates.isEmpty());
        });
}

void UpgradePanel::clearRows()
{
    for (QWidget *row : m_rows)
        delete row;
    m_rows.clear();
}

void UpgradePanel::addRow(const UpdateEntry &entry)
{
    auto *row = new QWidget;
    row->setAttribute(Qt::WA_StyledBackground);
    row->setStyleSheet("background-color: rgba(255, 255, 255, 10); border-radius: 10px;");
    auto *h = new QHBoxLayout(row);
    h->setContentsMargins(14, 8, 14, 8);
    h->setSpacing(10);

    auto *name = new QLabel(QString("%1 (%2)  →  %1 (%3)")
                                .arg(entry.attr, entry.oldVersion, entry.newVersion));
    name->setStyleSheet("color: #f0f0f2; font-size: 13px; background: transparent;");
    name->setTextInteractionFlags(Qt::TextSelectableByMouse);
    h->addWidget(name, 1);

    auto *changelogButton = makeRowButton("Changelog");
    auto *sourceButton = makeRowButton("Source");
    sourceButton->setEnabled(!entry.source.isEmpty());
    sourceButton->setToolTip(entry.source);
    h->addWidget(changelogButton);
    h->addWidget(sourceButton);

    const QString attr = entry.attr;
    const QString changelog = entry.changelog;
    const QString source = entry.source;
    connect(changelogButton, &QPushButton::clicked, this,
            [this, attr, changelog]() { showChangelog(this, attr, changelog); });
    connect(sourceButton, &QPushButton::clicked, this,
            [source]() { QDesktopServices::openUrl(QUrl(source)); });

    m_list->insertWidget(m_list->count() - 1, row);
    m_rows.append(row);
}

void UpgradePanel::upgrade()
{
    m_busy = true;
    m_upgradeButton->setEnabled(false);
    m_progress->setValue(0);
    m_progress->setVisible(true);
    m_progressText->setText("Starting…");
    m_progressText->setVisible(true);

    QPointer<UpgradePanel> self(this);
    const QByteArray password = m_password;
    runAsync(
        [password, self]() {
            return upgradeSystem(password, [self](int percent, const QString &text) {
                postToGui([self, percent, text]() {
                    if (!self)
                        return;
                    self->m_progress->setValue(percent);
                    self->m_progressText->setText(text);
                });
            });
        },
        [self](const QString &error) {
            if (!self)
                return;
            self->m_busy = false;
            if (!error.isEmpty()) {
                self->m_progressText->setText(error);
                self->m_upgradeButton->setEnabled(true);
                return;
            }
            self->m_progress->setValue(100);
            self->m_progressText->setText("Upgrade complete.");
            self->rescan();
        });
}
