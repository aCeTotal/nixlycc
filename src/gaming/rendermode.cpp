#include "rendermode.h"
#include "checkcard.h"
#include "conf.h"
#include "style.h"

#include <QLabel>
#include <QVBoxLayout>

RenderModeWidget::RenderModeWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    layout->addWidget(makeHeading("Rendering"));
    layout->addWidget(makeHint(
        "How nixlytile keeps fullscreen games smooth and responsive."));

    m_dynamic = new CheckCard("Dynamic rendering",
        "Runs games at the highest frame rate that matches your display "
        "exactly, aiming for 100 fps or more. If a game can't hold that, it "
        "settles on the next rate that still divides your refresh evenly. "
        "Shading gradually turns coarser from the screen edges inward as "
        "needed, while the center, the mouse pointer and all text and HUD "
        "stay sharp. Quality eases back as soon as there is headroom.");
    m_dynamic->setChecked(loadGamingConf().dynamicRender);
    layout->addWidget(m_dynamic);

    auto *foveated = new CheckCard("Foveated rendering",
        "Keeps full detail wherever you look, using an eye tracker. "
        "Not available yet.");
    foveated->setEnabled(false);
    layout->addWidget(foveated);

    m_status = new QLabel;
    m_status->setStyleSheet("color: #8b8f9a; font-size: 12px;");
    layout->addWidget(m_status);

    connect(m_dynamic, &QAbstractButton::toggled, this, &RenderModeWidget::store);
}

/* Reload first so the mic binds survive. */
void RenderModeWidget::store(bool dynamic)
{
    GamingConf conf = loadGamingConf();
    conf.dynamicRender = dynamic;
    if (!saveGamingConf(conf)) {
        m_status->setText("Could not write " + gamingConfPath());
        return;
    }
    m_status->setText("Saved. Applied immediately by nixlytile.");
}
