#pragma once

#include "conf.h"

#include <QWidget>

class BindCapture;
class QLabel;
class QPushButton;
class QVBoxLayout;

/* Push-to-mute (Discord) binds, any number of them. */
class PtmListWidget : public QWidget {
public:
    PtmListWidget(QWidget *parent = nullptr);

private:
    void captured(const Bind &bind);
    void removeBind(int index);
    void store(const QList<Bind> &binds);
    void rebuildRows();

    QVBoxLayout *m_rows;
    QPushButton *m_addButton;
    QLabel *m_status;
    BindCapture *m_capture;
    QList<Bind> m_binds;
};
