#include "checkcard.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QVariantAnimation>
#include <QVBoxLayout>

namespace {

const int kBox = 20;
const int kBoxLeft = 18;
const int kGap = 14;
const int kPadding = 16;
const qreal kCardRadius = 10.0;
const qreal kBoxRadius = 6.0;
const int kTickMs = 160;
const qreal kDisabledOpacity = 0.45;
const QColor kAccent(45, 91, 209);
const QColor kAccentEdge(122, 162, 247, 150);
const QColor kEdge(255, 255, 255, 30);
const QColor kWell(35, 37, 46);
const QColor kWellEdge(58, 61, 73);

QColor mix(const QColor &a, const QColor &b, qreal t)
{
    return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t,
                            a.greenF() + (b.greenF() - a.greenF()) * t,
                            a.blueF() + (b.blueF() - a.blueF()) * t,
                            a.alphaF() + (b.alphaF() - a.alphaF()) * t);
}

/* Stroke drawn up to t of its length. */
void drawTick(QPainter &p, const QRectF &box, qreal t)
{
    const QPointF a = box.topLeft() + QPointF(0.26 * box.width(), 0.52 * box.height());
    const QPointF b = box.topLeft() + QPointF(0.44 * box.width(), 0.70 * box.height());
    const QPointF c = box.topLeft() + QPointF(0.76 * box.width(), 0.32 * box.height());
    const qreal first = QLineF(a, b).length();
    const qreal second = QLineF(b, c).length();
    const qreal reach = t * (first + second);
    QPainterPath path(a);

    path.lineTo(QLineF(a, b).pointAt(qMin(1.0, reach / first)));
    if (reach > first)
        path.lineTo(QLineF(b, c).pointAt((reach - first) / second));
    p.drawPath(path);
}

} // namespace

CheckCard::CheckCard(const QString &title, const QString &hint, QWidget *parent)
    : QAbstractButton(parent)
    , m_title(new QLabel(title))
    , m_hint(new QLabel(hint))
    , m_tick(new QVariantAnimation(this))
{
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    setCheckable(true);
    setAttribute(Qt::WA_Hover);

    m_hint->setWordWrap(true);
    m_title->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_hint->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto *text = new QVBoxLayout;
    text->setSpacing(4);
    text->addWidget(m_title);
    text->addWidget(m_hint);
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(kBoxLeft + kBox + kGap, kPadding, kPadding, kPadding);
    layout->addLayout(text);

    m_tick->setDuration(kTickMs);
    m_tick->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_tick, &QVariantAnimation::valueChanged, this, [this]() { update(); });
    connect(this, &QAbstractButton::toggled, this, [this](bool on) {
        m_tick->stop();
        m_tick->setStartValue(m_tick->currentValue().isValid() ? m_tick->currentValue() : 0.0);
        m_tick->setEndValue(on ? 1.0 : 0.0);
        m_tick->start();
    });
    restyle();
}

void CheckCard::paintEvent(QPaintEvent *)
{
    const qreal t = m_tick->currentValue().toReal();
    const QRectF card = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const QRectF box(kBoxLeft, kPadding + 1, kBox, kBox);
    QPainter p(this);

    p.setRenderHint(QPainter::Antialiasing);
    p.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    p.setPen(QPen(mix(kEdge, kAccentEdge, t), hasFocus() ? 2.0 : 1.0));
    p.setBrush(QColor(255, 255, 255, underMouse() && isEnabled() ? 24 : 12));
    p.drawRoundedRect(card, kCardRadius, kCardRadius);

    p.setPen(QPen(mix(kWellEdge, kAccent, t), 1.5));
    p.setBrush(mix(kWell, kAccent, t));
    p.drawRoundedRect(box, kBoxRadius, kBoxRadius);

    if (t <= 0.0)
        return;
    p.setPen(QPen(Qt::white, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    drawTick(p, box, t);
}

void CheckCard::changeEvent(QEvent *event)
{
    QAbstractButton::changeEvent(event);
    if (event->type() == QEvent::EnabledChange)
        restyle();
}

void CheckCard::restyle()
{
    const bool on = isEnabled();

    setCursor(on ? Qt::PointingHandCursor : Qt::ArrowCursor);
    m_title->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: bold;")
                               .arg(on ? "#f0f0f2" : "#6b6f7a"));
    m_hint->setStyleSheet(QString("color: %1; font-size: 13px;")
                              .arg(on ? "#8b8f9a" : "#5b5f6a"));
}
