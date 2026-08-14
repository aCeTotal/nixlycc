#pragma once

#include <QList>
#include <QString>
#include <functional>

/* One package with a newer version available. */
struct UpdateEntry {
    QString attr;
    QString oldVersion;
    QString newVersion;
    QString changelog; /* URL from meta.changelog, may be empty */
    QString source;    /* URL of the package's source code, may be empty */
};

/* Finds every installed package — nixpkgs, nixpkgs-unstable and nixlypkgs
 * alike — whose version differs between the flake inputs the system is built
 * from and the newest revision of those inputs. The changelog and source
 * links are scraped in the same evaluation.
 *
 * Async. onStatus reports progress text; done gets the entries sorted by
 * attribute, or an error text. */
void scanForUpdates(std::function<void(const QString &)> onStatus,
                    std::function<void(const QList<UpdateEntry> &, const QString &)> done);
