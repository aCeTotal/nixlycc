#pragma once

#include "mesh.h"

#include <QTimer>
#include <QWidget>
#include <functional>

/* Shows a mesh and turns it. Selecting a button swings the model round to the
 * side that button sits on; dragging spins it by hand. Clicking a button
 * reports it through onPick. */
class DeviceView : public QWidget {
public:
    explicit DeviceView(QWidget *parent = nullptr);

    void setMesh(const Mesh &mesh);
    void setSelected(int part);
    void turnTo(float yaw, float pitch);
    int selected() const { return m_selected; }

    std::function<void(int)> onPick;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void step();

    Mesh m_mesh;
    QTimer m_timer;
    float m_yaw = 0.5f;
    float m_pitch = -0.6f;
    float m_targetYaw = 0.5f;
    float m_targetPitch = -0.6f;
    int m_selected = -1;
    bool m_dragging = false;
    QPointF m_dragFrom;
};
