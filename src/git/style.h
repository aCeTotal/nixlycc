#pragma once

#include <QString>
#include <QStringList>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QVBoxLayout;

/* Shared look for the Git page widgets, matching the rest of the panel. */
QLineEdit *makeField(const QString &placeholder, const QString &value = QString());
QComboBox *makeCombo(const QStringList &items);
QPushButton *makeButton(const QString &text);

/* Checkable pill used to switch between the halves of a page. */
QPushButton *makeTab(const QString &text);
QLabel *makeStatus();
QLabel *makeFieldLabel(const QString &text);

/* Small blue caps over a hairline rule, as on the System Information page. */
void addSection(QVBoxLayout *layout, const QString &title);
