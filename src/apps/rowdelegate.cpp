#include "rowdelegate.h"
#include "icons.h"

#include <QPainter>

PkgRowDelegate::PkgRowDelegate(const IconIndex *icons, QObject *parent)
    : QStyledItemDelegate(parent), m_icons(icons)
{
}

QSize PkgRowDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
    return QSize(0, 58);
}

void PkgRowDelegate::paint(QPainter *p, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const
{
    const QRect r = option.rect;
    p->save();
    p->setRenderHint(QPainter::Antialiasing);

    if (option.state & QStyle::State_Selected)
        p->fillRect(r, QColor(102, 179, 255, 55));
    else if (option.state & QStyle::State_MouseOver)
        p->fillRect(r, QColor(255, 255, 255, 16));

    const QString attr = index.data(Qt::DisplayRole).toString();
    const QString pname = index.data(PnameRole).toString();
    const QString version = index.data(VersionRole).toString();
    const QString desc = index.data(DescRole).toString();
    const bool nixly = index.data(NixlyRole).toBool();

    const QRect iconRect(r.left() + 6, r.top() + 9, 40, 40);
    m_icons->icon(attr, pname).paint(p, iconRect);

    const int textLeft = iconRect.right() + 14;
    int textRight = r.right() - 10;

    /* Tick box on the far right — filled once the package is in the list. */
    const bool installed = index.data(InstalledRole).toBool();
    const QRect tick(textRight - 22, r.top() + 18, 22, 22);
    p->setPen(QPen(QColor(installed ? 122 : 255, installed ? 162 : 255,
                          installed ? 247 : 255, installed ? 200 : 60), 1.5));
    p->setBrush(installed ? QColor(122, 162, 247, 90) : QColor(255, 255, 255, 10));
    p->drawRoundedRect(tick, 6, 6);
    if (installed) {
        p->setPen(QPen(QColor(240, 240, 242), 2));
        p->drawLine(tick.left() + 6, tick.center().y(), tick.center().x(), tick.bottom() - 6);
        p->drawLine(tick.center().x(), tick.bottom() - 6, tick.right() - 5, tick.top() + 6);
    }
    p->setBrush(Qt::NoBrush);
    textRight = tick.left() - 12;

    /* Version sits flush right on the first line. */
    QFont small = option.font;
    small.setPointSize(9);
    if (!version.isEmpty()) {
        p->setFont(small);
        const int w = QFontMetrics(small).horizontalAdvance(version);
        p->setPen(QColor(139, 143, 154));
        p->drawText(QRect(textRight - w, r.top() + 11, w, 18), Qt::AlignRight | Qt::AlignVCenter,
                    version);
        textRight -= w + 12;
    }

    QFont title = option.font;
    title.setPointSize(11);
    title.setWeight(QFont::DemiBold);
    p->setFont(title);

    int titleLeft = textLeft;
    if (nixly) {
        const QString badge = "nixly";
        QFont badgeFont = option.font;
        badgeFont.setPointSize(8);
        badgeFont.setWeight(QFont::DemiBold);
        const int bw = QFontMetrics(badgeFont).horizontalAdvance(badge) + 12;
        const QRect badgeRect(titleLeft, r.top() + 12, bw, 17);
        p->setBrush(QColor(122, 162, 247, 60));
        p->setPen(Qt::NoPen);
        p->drawRoundedRect(badgeRect, 8, 8);
        p->setFont(badgeFont);
        p->setPen(QColor(150, 185, 255));
        p->drawText(badgeRect, Qt::AlignCenter, badge);
        titleLeft += bw + 8;
        p->setFont(title);
    }

    p->setPen(QColor(240, 240, 242));
    const QRect titleRect(titleLeft, r.top() + 10, textRight - titleLeft, 20);
    p->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter,
                QFontMetrics(title).elidedText(attr, Qt::ElideRight, titleRect.width()));

    if (!desc.isEmpty()) {
        p->setFont(small);
        p->setPen(QColor(139, 143, 154));
        const QRect descRect(textLeft, r.top() + 30, tick.left() - 12 - textLeft, 18);
        p->drawText(descRect, Qt::AlignLeft | Qt::AlignVCenter,
                    QFontMetrics(small).elidedText(desc, Qt::ElideRight, descRect.width()));
    }

    p->setPen(QColor(255, 255, 255, 14));
    p->drawLine(r.left() + 6, r.bottom(), r.right() - 6, r.bottom());
    p->restore();
}
