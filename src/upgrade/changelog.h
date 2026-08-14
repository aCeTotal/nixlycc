#pragma once

#include <QString>

class QWidget;

/* Popup showing a package's changelog, fetched from its meta.changelog URL.
 * When the fetch fails — or the package publishes no changelog — the popup
 * says so and offers the browser instead. */
void showChangelog(QWidget *parent, const QString &attr, const QString &url);
