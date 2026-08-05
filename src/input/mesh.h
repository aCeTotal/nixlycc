#pragma once

#include <QColor>
#include <QList>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector3D>

class QPainter;

/* A flat-shaded polygon. part is the evdev code of the button this face
 * belongs to, or -1 for the chassis; label is drawn on the face when it is
 * facing the camera. */
struct Face {
    QList<int> idx;
    QColor color;
    int part = -1;
    QString label;
};

/* Just enough geometry for a mouse and a keyboard: vertices, quads, and a
 * painter's-algorithm renderer. No GPU, no new dependencies. */
struct Mesh {
    QList<QVector3D> verts;
    QList<Face> faces;

    /* Box from corner to corner, six faces, all carrying part and label. */
    void addBox(const QVector3D &min, const QVector3D &max, const QColor &color, int part = -1,
                const QString &label = QString());

    /* Upright cylinder along X (a scroll wheel), segments around. */
    void addWheel(const QVector3D &center, float radius, float halfWidth, int segments,
                  const QColor &color, int part);
};

/* Draws the mesh into area, rotated yaw around Y and pitch around X. The
 * highlighted part is lit up; every other face is shaded by its angle to a
 * fixed light. */
void renderMesh(QPainter &painter, const Mesh &mesh, const QRectF &area, float yaw, float pitch,
                int highlight);

/* The part under pos, or -1. Uses the same projection as renderMesh. */
int pickPart(const Mesh &mesh, const QRectF &area, float yaw, float pitch, const QPointF &pos);
