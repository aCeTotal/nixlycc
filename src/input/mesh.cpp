#include "mesh.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QPolygonF>
#include <algorithm>
#include <cmath>

namespace {

const QVector3D kLight = QVector3D(-0.4f, -0.75f, 0.55f).normalized();
const float kCameraZ = 620.0f;

struct Projected {
    QList<QPointF> screen;
    QList<QVector3D> view;
};

/* Rotate around Y then X, then perspective-divide. The mesh is authored
 * around the origin in millimetre-ish units. */
Projected project(const Mesh &mesh, const QRectF &area, float yaw, float pitch)
{
    Projected out;
    const float cy = std::cos(yaw), sy = std::sin(yaw);
    const float cx = std::cos(pitch), sx = std::sin(pitch);

    QList<QVector3D> view;
    view.reserve(mesh.verts.size());
    float maxR = 1.0f;
    for (const QVector3D &v : mesh.verts) {
        const float x = v.x() * cy + v.z() * sy;
        const float z = -v.x() * sy + v.z() * cy;
        const float y = v.y() * cx - z * sx;
        const float z2 = v.y() * sx + z * cx;
        view.append(QVector3D(x, y, z2));
        maxR = std::max(maxR, std::max(std::abs(x), std::abs(y)));
    }

    const float scale = float(std::min(area.width(), area.height())) * 0.42f / maxR;
    const QPointF centre = area.center();
    out.view = view;
    out.screen.reserve(view.size());
    for (const QVector3D &v : view) {
        const float depth = kCameraZ / std::max(60.0f, kCameraZ - v.z() * scale * 0.55f);
        out.screen.append(QPointF(centre.x() + v.x() * scale * depth,
                                  centre.y() + v.y() * scale * depth));
    }
    return out;
}

QVector3D faceNormal(const QList<QVector3D> &view, const Face &face)
{
    if (face.idx.size() < 3)
        return QVector3D(0, 0, 1);
    const QVector3D a = view[face.idx[1]] - view[face.idx[0]];
    const QVector3D b = view[face.idx[2]] - view[face.idx[0]];
    return QVector3D::crossProduct(a, b).normalized();
}

float faceDepth(const QList<QVector3D> &view, const Face &face)
{
    float z = 0;
    for (int i : face.idx)
        z += view[i].z();
    return z / face.idx.size();
}

QPolygonF facePolygon(const QList<QPointF> &screen, const Face &face)
{
    QPolygonF poly;
    for (int i : face.idx)
        poly << screen[i];
    return poly;
}

/* Faces are drawn back to front; the ones pointing away from the camera are
 * dropped first, which is enough for closed convex-ish boxes. */
QList<int> visibleOrder(const Projected &p, const Mesh &mesh)
{
    QList<int> order;
    for (int i = 0; i < mesh.faces.size(); ++i) {
        if (faceNormal(p.view, mesh.faces[i]).z() > 0.0f)
            order.append(i);
    }
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        return faceDepth(p.view, mesh.faces[a]) < faceDepth(p.view, mesh.faces[b]);
    });
    return order;
}

} // namespace

void Mesh::addBox(const QVector3D &min, const QVector3D &max, const QColor &color, int part,
                  const QString &label)
{
    const int base = verts.size();
    verts << QVector3D(min.x(), min.y(), min.z()) << QVector3D(max.x(), min.y(), min.z())
          << QVector3D(max.x(), max.y(), min.z()) << QVector3D(min.x(), max.y(), min.z())
          << QVector3D(min.x(), min.y(), max.z()) << QVector3D(max.x(), min.y(), max.z())
          << QVector3D(max.x(), max.y(), max.z()) << QVector3D(min.x(), max.y(), max.z());

    const int quads[6][4] = {
        { 0, 1, 2, 3 }, /* back  */
        { 5, 4, 7, 6 }, /* front */
        { 4, 0, 3, 7 }, /* left  */
        { 1, 5, 6, 2 }, /* right */
        { 4, 5, 1, 0 }, /* top   */
        { 3, 2, 6, 7 }, /* bottom*/
    };
    for (int f = 0; f < 6; ++f) {
        Face face;
        for (int k = 0; k < 4; ++k)
            face.idx << base + quads[f][k];
        face.color = color;
        face.part = part;
        /* Only the top face carries the label — that is the one you read. */
        face.label = (f == 4) ? label : QString();
        faces << face;
    }
}

void Mesh::addWheel(const QVector3D &center, float radius, float halfWidth, int segments,
                    const QColor &color, int part)
{
    const int base = verts.size();
    for (int i = 0; i < segments; ++i) {
        const float a = float(i) / segments * 2.0f * float(M_PI);
        const float y = center.y() + std::sin(a) * radius;
        const float z = center.z() + std::cos(a) * radius;
        verts << QVector3D(center.x() - halfWidth, y, z);
        verts << QVector3D(center.x() + halfWidth, y, z);
    }
    for (int i = 0; i < segments; ++i) {
        const int a = base + i * 2;
        const int b = base + ((i + 1) % segments) * 2;
        Face face;
        face.idx << a << b << b + 1 << a + 1;
        face.color = (i % 2) ? color.darker(125) : color;
        face.part = part;
        faces << face;
    }
}

void renderMesh(QPainter &painter, const Mesh &mesh, const QRectF &area, float yaw, float pitch,
                int highlight)
{
    const Projected p = project(mesh, area, yaw, pitch);
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    for (int i : visibleOrder(p, mesh)) {
        const Face &face = mesh.faces[i];
        const QVector3D normal = faceNormal(p.view, face);
        const float lit = 0.42f + 0.58f * std::max(0.0f, QVector3D::dotProduct(normal, kLight));

        QColor color = face.color;
        if (face.part >= 0 && face.part == highlight)
            color = QColor(122, 162, 247);
        color = QColor::fromHslF(color.hueF() < 0 ? 0.6 : color.hueF(), color.saturationF(),
                                 std::min(0.92f, color.lightnessF() * lit));

        const QPolygonF poly = facePolygon(p.screen, face);
        painter.setBrush(color);
        painter.setPen(QPen(QColor(0, 0, 0, 70), 1));
        painter.drawPolygon(poly);

        if (!face.label.isEmpty() && normal.z() > 0.55f) {
            QFont font = painter.font();
            font.setPointSizeF(std::max(5.0, poly.boundingRect().height() * 0.28));
            painter.setFont(font);
            painter.setPen(QColor(20, 22, 28, 220));
            painter.drawText(poly.boundingRect(), Qt::AlignCenter, face.label);
        }
    }
    painter.restore();
}

int pickPart(const Mesh &mesh, const QRectF &area, float yaw, float pitch, const QPointF &pos)
{
    const Projected p = project(mesh, area, yaw, pitch);
    const QList<int> order = visibleOrder(p, mesh);
    /* Front-most first — the reverse of the draw order. */
    for (int i = order.size() - 1; i >= 0; --i) {
        const Face &face = mesh.faces[order[i]];
        if (face.part >= 0 && facePolygon(p.screen, face).containsPoint(pos, Qt::OddEvenFill))
            return face.part;
    }
    return -1;
}
