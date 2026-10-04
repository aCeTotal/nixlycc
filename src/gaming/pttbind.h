#pragma once

#include "conf.h"

#include <QWidget>

class BindCapture;
class QLabel;
class QPushButton;

/* Single push-to-talk bind editor. */
class PttBindWidget : public QWidget {
public:
    PttBindWidget(QWidget *parent = nullptr);

private:
    void startCapture();
    void saveBind(const Bind &bind);
    void refreshDisplay();

    QLabel *m_bindLabel;
    QPushButton *m_changeButton;
    QPushButton *m_clearButton;
    QLabel *m_status;
    BindCapture *m_capture;
    Bind m_bind;
};
