#pragma once

#include <QString>

class QLabel;
class QPushButton;

/* Shared look for the VOIP bind editors. */
QLabel *makeHeading(const QString &text);
QLabel *makeHint(const QString &text);
QLabel *makeBindChip();
QPushButton *makeRoundButton(const QString &glyph, const QString &hover);
