#pragma once

#include "conf.h"

#include <QObject>
#include <functional>

class QKeyEvent;
class QMouseEvent;
class QWidget;

/* Grabs one key combo or side/middle mouse button; Esc cancels. */
class BindCapture : public QObject {
public:
    explicit BindCapture(QWidget *owner);

    void start();
    bool active() const { return m_active; }

    /* Empty value means cancelled. */
    std::function<void(const Bind &)> onDone;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void finish(const Bind &bind);
    void handleKey(QKeyEvent *event);
    void handleMouse(QMouseEvent *event);

    QWidget *m_owner;
    bool m_active = false;
};
