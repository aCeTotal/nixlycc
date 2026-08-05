#include "mousemesh.h"
#include "devices.h"

#include <linux/input.h>

namespace {

const QColor kShell(58, 62, 74);
const QColor kButton(78, 84, 100);
const QColor kAccent(96, 104, 124);

/* The model is authored in millimetres around the origin: x right, y down,
 * z towards the front of the mouse. */
const float kHalfWidth = 31.0f;
const float kFront = 60.0f;
const float kBack = -62.0f;

void addSideColumn(Mesh &mesh, const QList<int> &buttons, float xInner, float xOuter)
{
    float z = 34.0f;
    for (int code : buttons) {
        mesh.addBox(QVector3D(std::min(xInner, xOuter), -4.0f, z - 24.0f),
                    QVector3D(std::max(xInner, xOuter), 7.0f, z), kButton, code);
        z -= 28.0f;
    }
}

} // namespace

Mesh buildMouseMesh(const InputDevice &device)
{
    Mesh mesh;

    /* Chassis: a lower slab and a shoulder, so the silhouette reads as a
     * mouse rather than a brick. */
    mesh.addBox(QVector3D(-kHalfWidth, -6.0f, kBack), QVector3D(kHalfWidth, 20.0f, kFront), kShell);
    mesh.addBox(QVector3D(-kHalfWidth + 3.0f, -18.0f, kBack + 6.0f),
                QVector3D(kHalfWidth - 3.0f, -5.0f, 4.0f), kShell.lighter(112));

    QList<int> side, right;
    bool hasLeft = false, hasRight = false, hasMiddle = false, hasTask = false;
    for (int code : device.buttons) {
        switch (code) {
        case BTN_LEFT:
            hasLeft = true;
            break;
        case BTN_RIGHT:
            hasRight = true;
            break;
        case BTN_MIDDLE:
            hasMiddle = true;
            break;
        case BTN_SIDE:
        case BTN_EXTRA:
            side << code;
            break;
        case BTN_FORWARD:
        case BTN_BACK:
            right << code;
            break;
        case BTN_TASK:
            hasTask = true;
            break;
        default:
            break;
        }
    }

    if (hasLeft)
        mesh.addBox(QVector3D(-kHalfWidth, -14.0f, 6.0f), QVector3D(-5.0f, -5.0f, kFront), kButton,
                    BTN_LEFT);
    if (hasRight)
        mesh.addBox(QVector3D(5.0f, -14.0f, 6.0f), QVector3D(kHalfWidth, -5.0f, kFront), kButton,
                    BTN_RIGHT);
    if (hasMiddle || device.wheel)
        mesh.addWheel(QVector3D(0.0f, -13.0f, 40.0f), 9.0f, 4.0f, 14, kAccent, BTN_MIDDLE);
    if (hasTask)
        mesh.addBox(QVector3D(-9.0f, -20.0f, -26.0f), QVector3D(9.0f, -16.0f, -6.0f), kButton,
                    BTN_TASK);

    addSideColumn(mesh, side, -kHalfWidth, -kHalfWidth - 3.5f);
    addSideColumn(mesh, right, kHalfWidth, kHalfWidth + 3.5f);
    return mesh;
}

void mouseViewFor(int part, float *yaw, float *pitch)
{
    switch (part) {
    case BTN_SIDE:
    case BTN_EXTRA:
        *yaw = 1.15f;
        *pitch = -0.32f;
        return;
    case BTN_FORWARD:
    case BTN_BACK:
        *yaw = -1.15f;
        *pitch = -0.32f;
        return;
    case BTN_TASK:
        *yaw = 0.25f;
        *pitch = -0.95f;
        return;
    case BTN_LEFT:
    case BTN_RIGHT:
    case BTN_MIDDLE:
        *yaw = 0.3f;
        *pitch = -0.8f;
        return;
    default:
        *yaw = 0.5f;
        *pitch = -0.6f;
        return;
    }
}
