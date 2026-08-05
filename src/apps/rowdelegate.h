#pragma once

#include <QStyledItemDelegate>

class IconIndex;

/* Two-line search result: icon, attribute path with version, description, and
 * a tick box on the right showing whether the package is in packages.nix. */
class PkgRowDelegate : public QStyledItemDelegate {
public:
    enum Role {
        DescRole = Qt::UserRole + 1,
        VersionRole,
        PnameRole,
        NixlyRole,
        InstalledRole,
    };

    explicit PkgRowDelegate(const IconIndex *icons, QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

private:
    const IconIndex *m_icons;
};
