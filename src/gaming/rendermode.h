#pragma once

#include <QWidget>

class CheckCard;
class QLabel;

/* Dynamic or foveated rendering for games. */
class RenderModeWidget : public QWidget {
public:
    RenderModeWidget(QWidget *parent = nullptr);

private:
    void store(bool dynamic);

    CheckCard *m_dynamic;
    QLabel *m_status;
};
