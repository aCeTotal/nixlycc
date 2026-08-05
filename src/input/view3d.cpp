#include "view3d.h"

#include <QMouseEvent>
#include <QPainter>
#include <cmath>

DeviceView::DeviceView(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(280);
    setCursor(Qt::OpenHandCursor);

    m_timer.setInterval(16);
    connect(&m_timer, &QTimer::timeout, this, [this]() { step(); });
}

void DeviceView::setMesh(const Mesh &mesh)
{
    m_mesh = mesh;
    update();
}

void DeviceView::setSelected(int part)
{
    m_selected = part;
    update();
}

void DeviceView::turnTo(float yaw, float pitch)
{
    m_targetYaw = yaw;
    m_targetPitch = pitch;
    if (!m_timer.isActive())
        m_timer.start();
}

/* Eases towards the target angles and stops once it is there, so an idle page
 * costs nothing. */
void DeviceView::step()
{
    const float dy = m_targetYaw - m_yaw;
    const float dp = m_targetPitch - m_pitch;
    if (std::abs(dy) < 0.002f && std::abs(dp) < 0.002f) {
        m_yaw = m_targetYaw;
        m_pitch = m_targetPitch;
        m_timer.stop();
        update();
        return;
    }
    m_yaw += dy * 0.18f;
    m_pitch += dp * 0.18f;
    update();
}

void DeviceView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(255, 255, 255, 10));
    if (m_mesh.faces.isEmpty()) {
        painter.setPen(QColor(139, 143, 154));
        painter.drawText(rect(), Qt::AlignCenter, "No device detected.");
        return;
    }
    renderMesh(painter, m_mesh, rect().adjusted(20, 20, -20, -20), m_yaw, m_pitch, m_selected);
}

void DeviceView::mousePressEvent(QMouseEvent *event)
{
    m_dragging = true;
    m_dragFrom = event->position();
    setCursor(Qt::ClosedHandCursor);
}

void DeviceView::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging)
        return;
    const QPointF delta = event->position() - m_dragFrom;
    m_dragFrom = event->position();
    m_targetYaw = m_yaw + float(delta.x()) * 0.012f;
    m_targetPitch = qBound(-1.45f, m_pitch + float(delta.y()) * 0.012f, 0.5f);
    m_yaw = m_targetYaw;
    m_pitch = m_targetPitch;
    update();
}

/* A press that did not turn into a drag is a click on a button. */
void DeviceView::mouseReleaseEvent(QMouseEvent *event)
{
    m_dragging = false;
    setCursor(Qt::OpenHandCursor);

    const int part =
        pickPart(m_mesh, rect().adjusted(20, 20, -20, -20), m_yaw, m_pitch, event->position());
    if (part >= 0 && onPick)
        onPick(part);
}
