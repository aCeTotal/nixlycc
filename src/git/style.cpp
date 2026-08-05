#include "style.h"

#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

const char *kFieldStyle =
    "QLineEdit { background-color: rgba(255, 255, 255, 16);"
    " border: 1px solid rgba(255, 255, 255, 30); border-radius: 10px;"
    " padding: 0 14px; color: #f0f0f2; font-size: 14px; }"
    "QLineEdit:focus { border: 1px solid #7aa2f7; }";

const char *kButtonStyle =
    "QPushButton { background-color: rgba(122, 162, 247, 45);"
    " border: 1px solid rgba(122, 162, 247, 120); border-radius: 10px;"
    " padding: 9px 18px; color: #f0f0f2; font-size: 14px; }"
    "QPushButton:hover { background-color: rgba(122, 162, 247, 80); }"
    "QPushButton:disabled { color: #8b8f9a; border-color: rgba(255, 255, 255, 30);"
    " background-color: rgba(255, 255, 255, 12); }";

} // namespace

QLineEdit *makeField(const QString &placeholder, const QString &value)
{
    auto *edit = new QLineEdit(value);
    edit->setPlaceholderText(placeholder);
    edit->setMinimumHeight(40);
    edit->setStyleSheet(kFieldStyle);
    return edit;
}

QComboBox *makeCombo(const QStringList &items)
{
    auto *combo = new QComboBox;
    combo->addItems(items);
    combo->setCursor(Qt::PointingHandCursor);
    combo->setMinimumHeight(40);
    combo->setStyleSheet(
        "QComboBox { background-color: rgba(255, 255, 255, 16);"
        " border: 1px solid rgba(255, 255, 255, 30); border-radius: 10px;"
        " padding: 0 14px; color: #f0f0f2; font-size: 14px; }"
        "QComboBox:focus { border: 1px solid #7aa2f7; }"
        "QComboBox::drop-down { border: none; width: 24px; }"
        "QComboBox QAbstractItemView { background-color: #1c1e26; color: #f0f0f2;"
        " selection-background-color: rgba(122, 162, 247, 80); border: 1px solid"
        " rgba(255, 255, 255, 30); outline: none; }");
    return combo;
}

QPushButton *makeButton(const QString &text)
{
    auto *button = new QPushButton(text);
    button->setCursor(Qt::PointingHandCursor);
    button->setStyleSheet(kButtonStyle);
    return button;
}

QPushButton *makeTab(const QString &text)
{
    auto *tab = new QPushButton(text);
    tab->setCheckable(true);
    tab->setCursor(Qt::PointingHandCursor);
    tab->setStyleSheet(
        "QPushButton { background-color: rgba(255, 255, 255, 12);"
        " border: 1px solid rgba(255, 255, 255, 30); border-radius: 10px;"
        " padding: 9px 26px; color: #8b8f9a; font-size: 14px; }"
        "QPushButton:hover { background-color: rgba(255, 255, 255, 24); }"
        "QPushButton:checked { background-color: rgba(122, 162, 247, 60);"
        " border-color: rgba(122, 162, 247, 140); color: #f0f0f2; }");
    return tab;
}

QLabel *makeStatus()
{
    auto *label = new QLabel;
    label->setStyleSheet("color: #8b8f9a; font-size: 12px;");
    label->setWordWrap(true);
    return label;
}

QLabel *makeFieldLabel(const QString &text)
{
    auto *label = new QLabel(text);
    label->setStyleSheet("color: #8b8f9a; font-size: 13px;");
    return label;
}

void addSection(QVBoxLayout *layout, const QString &title)
{
    auto *label = new QLabel(title.toUpper());
    label->setStyleSheet("color: #7aa2f7; font-size: 11px; font-weight: bold; letter-spacing: 1.5px;");
    layout->addWidget(label);

    auto *rule = new QFrame;
    rule->setFixedHeight(1);
    rule->setStyleSheet("background-color: rgba(255, 255, 255, 30);");
    layout->addWidget(rule);
}
