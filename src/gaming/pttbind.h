#pragma once

#include <QWidget>

class QLabel;
class QPushButton;

/* Push-to-talk bind editor.  Captures a key combo (Ctrl/Alt/Shift/Super +
 * key) or a mouse button (side/extra/middle) and writes it to
 * ~/.local/nixlyos/gaming.conf, which nixlytile hot-reloads. */
class PttBindWidget : public QWidget {
public:
    PttBindWidget(QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    void startCapture();
    void stopCapture();
    void clearBind();
    void saveBind(const QString &bind, const QString &label);
    void loadBind();
    void refreshDisplay();

    QLabel *m_bindLabel;
    QPushButton *m_changeButton;
    QPushButton *m_clearButton;
    QLabel *m_status;

    QString m_bind;   /* conf value, e.g. "ctrl+key:0x6d" or "mouse:275" */
    QString m_label;  /* display text, e.g. "Ctrl+M" */
    bool m_capturing = false;
};
