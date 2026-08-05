#pragma once

#include <QByteArray>

class QWidget;

/* Asks for the user password sudo needs to rebuild. Returns an empty array
 * when the dialog is cancelled. The password is not checked here — sudo
 * rejects a wrong one and the rebuild reports it. */
QByteArray askPassword(QWidget *parent);
