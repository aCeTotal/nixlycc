#include "icons.h"

#include "iconfarm.h"

#include <QFileInfo>
#include <QLinearGradient>
#include <QPainter>
#include <QPixmap>

namespace {

QIcon load(const QString &key)
{
    if (key.isEmpty())
        return QIcon();
    const QString path = IconFarm::fileFor(key);
    if (!QFileInfo::exists(path))
        return QIcon();
    QIcon icon(path);
    return icon.availableSizes().isEmpty() ? QIcon() : icon;
}

/* Rounded slate tile — only reached before the first farm finishes, when even
 * application-x-executable has not been baked yet. */
QIcon makePlaceholder()
{
    const int size = 40;
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    QLinearGradient g(0, 0, 0, size);
    g.setColorAt(0, QColor(110, 118, 140, 140));
    g.setColorAt(1, QColor(122, 136, 168, 140));
    p.setBrush(g);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(QRectF(0.5, 0.5, size - 1, size - 1), 9, 9);
    return QIcon(pm);
}

} // namespace

void IconIndex::reset()
{
    m_cache.clear();
}

QIcon IconIndex::icon(const QString &attr, const QString &pname) const
{
    const auto cached = m_cache.constFind(attr);
    if (cached != m_cache.constEnd())
        return *cached;

    QIcon result = load(attr.toLower());
    if (result.isNull())
        result = load(pname.toLower());
    if (result.isNull())
        result = load(attr.section('.', -1).toLower());
    if (result.isNull())
        result = load("application-x-executable");
    if (result.isNull())
        result = makePlaceholder();

    m_cache.insert(attr, result);
    return result;
}
