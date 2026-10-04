#include "ptmlist.h"
#include "capture.h"
#include "style.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

const char *kAddHover = "#2d5bd1";
const char *kRemoveHover = "#c0392b";

} // namespace

PtmListWidget::PtmListWidget(QWidget *parent)
    : QWidget(parent)
    , m_capture(new BindCapture(this))
{
    setFocusPolicy(Qt::StrongFocus);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    layout->addWidget(makeHeading("Push-to-mute (Discord)"));
    layout->addWidget(makeHint(
        "While a bind is held the microphone is open for everything except "
        "Discord — talk in-game without Discord hearing a thing. Discord gets "
        "pure silence, not noise."));

    m_rows = new QVBoxLayout;
    m_rows->setSpacing(6);
    layout->addLayout(m_rows);

    m_addButton = makeRoundButton("+", kAddHover);
    m_addButton->setToolTip("Add bind");
    layout->addWidget(m_addButton);

    m_status = new QLabel;
    m_status->setStyleSheet("color: #8b8f9a; font-size: 12px;");
    layout->addWidget(m_status);

    connect(m_addButton, &QPushButton::clicked, this, [this]() {
        m_capture->start();
        m_status->setText("Press a key or side/middle mouse button. Esc cancels.");
    });
    m_capture->onDone = [this](const Bind &bind) { captured(bind); };

    m_binds = loadGamingConf().voipMute;
    rebuildRows();
}

void PtmListWidget::captured(const Bind &bind)
{
    m_status->clear();
    if (bind.value.isEmpty())
        return;
    for (const Bind &b : m_binds) {
        if (b.value != bind.value)
            continue;
        m_status->setText(bind.label + " is already bound.");
        return;
    }
    QList<Bind> binds = m_binds;
    binds.append(bind);
    store(binds);
}

void PtmListWidget::removeBind(int index)
{
    QList<Bind> binds = m_binds;
    binds.removeAt(index);
    store(binds);
}

/* Reload first so the push-to-talk bind survives. */
void PtmListWidget::store(const QList<Bind> &binds)
{
    GamingConf conf = loadGamingConf();
    conf.voipMute = binds;
    if (!saveGamingConf(conf)) {
        m_status->setText("Could not write " + gamingConfPath());
        return;
    }
    m_binds = binds;
    rebuildRows();
    m_status->setText("Saved. Applied immediately by nixlytile.");
}

void PtmListWidget::rebuildRows()
{
    while (QLayoutItem *item = m_rows->takeAt(0)) {
        item->widget()->hide();
        item->widget()->deleteLater();
        delete item;
    }
    for (int i = 0; i < m_binds.size(); i++) {
        auto *row = new QWidget;
        auto *line = new QHBoxLayout(row);
        line->setContentsMargins(0, 0, 0, 0);
        line->setSpacing(10);

        auto *chip = makeBindChip();
        chip->setText(m_binds[i].label.isEmpty() ? m_binds[i].value : m_binds[i].label);
        line->addWidget(chip);

        auto *remove = makeRoundButton("×", kRemoveHover);
        remove->setToolTip("Remove bind");
        connect(remove, &QPushButton::clicked, this, [this, i]() { removeBind(i); });
        line->addWidget(remove);
        line->addStretch();
        m_rows->addWidget(row);
    }
}
