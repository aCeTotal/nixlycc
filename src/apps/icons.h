#pragma once

#include <QHash>
#include <QIcon>
#include <QString>

/* Icon lookup for package names.
 *
 * Reads the PNGs IconFarm baked into ~/.cache/nixlycc/icons, so a lookup is a
 * single file open — no scanning at startup. Names with no icon get
 * application-x-executable, the same fallback nixly_launcher uses. */
class IconIndex {
public:
    /* Tries attr, then pname, then the last component of a reverse-DNS attr. */
    QIcon icon(const QString &attr, const QString &pname) const;

    /* Drops memoised icons so a finished farm shows up. */
    void reset();

private:
    mutable QHash<QString, QIcon> m_cache;
};
