#include "style.h"

#include <QLabel>
#include <QPushButton>

namespace {

const int kRoundSize = 34;

} // namespace

QLabel *makeHeading(const QString &text)
{
    auto *label = new QLabel(text);
    label->setStyleSheet("color: #f0f0f2; font-size: 17px; font-weight: bold;");
    return label;
}

QLabel *makeHint(const QString &text)
{
    auto *label = new QLabel(text);
    label->setStyleSheet("color: #8b8f9a; font-size: 13px;");
    label->setWordWrap(true);
    return label;
}

QLabel *makeBindChip()
{
    auto *label = new QLabel;
    label->setStyleSheet(
        "color: #f0f0f2; font-size: 14px; font-weight: bold;"
        "background: #23252e; border: 1px solid #3a3d49; border-radius: 6px;"
        "padding: 8px 16px;");
    return label;
}

QPushButton *makeRoundButton(const QString &glyph, const QString &hover)
{
    auto *button = new QPushButton(glyph);
    button->setFixedSize(kRoundSize, kRoundSize);
    button->setCursor(Qt::PointingHandCursor);
    button->setStyleSheet(QString(
        "QPushButton { color: #f0f0f2; background: #2a2c35; border: 1px solid #3a3d49;"
        " border-radius: %1px; font-size: 18px; font-weight: bold; padding: 0; }"
        "QPushButton:hover { background: %2; border-color: %2; }"
        "QPushButton:pressed { padding-top: 2px; }")
        .arg(kRoundSize / 2).arg(hover));
    return button;
}
