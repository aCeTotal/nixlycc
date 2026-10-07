#pragma once

#include <QAbstractButton>

class QLabel;
class QVariantAnimation;

/* Checkable card: box, title, hint. */
class CheckCard : public QAbstractButton {
public:
    CheckCard(const QString &title, const QString &hint, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    void restyle();

    QLabel *m_title;
    QLabel *m_hint;
    QVariantAnimation *m_tick;
};
