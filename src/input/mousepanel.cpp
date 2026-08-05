#include "mousepanel.h"
#include "../git/style.h"
#include "mousemesh.h"
#include "view3d.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

#include <linux/input.h>

namespace {

const char *kSliderStyle =
    "QSlider::groove:horizontal { height: 4px; background: rgba(255,255,255,40);"
    " border-radius: 2px; }"
    "QSlider::handle:horizontal { width: 16px; margin: -7px 0; border-radius: 8px;"
    " background: #7aa2f7; }";

const char *kSpinStyle =
    "QSpinBox { background-color: rgba(255, 255, 255, 16);"
    " border: 1px solid rgba(255, 255, 255, 30); border-radius: 10px;"
    " padding: 0 10px; color: #f0f0f2; font-size: 14px; min-height: 34px; }";

QComboBox *actionCombo(const QString &current)
{
    QStringList names;
    for (const auto &action : inputActions())
        names << action.first;

    QComboBox *combo = makeCombo(names);
    combo->setMinimumHeight(0);
    combo->setFixedHeight(34);
    for (int i = 0; i < inputActions().size(); ++i) {
        if (inputActions()[i].second == current)
            combo->setCurrentIndex(i);
    }
    return combo;
}

QCheckBox *makeCheck(const QString &text, bool checked)
{
    auto *box = new QCheckBox(text);
    box->setChecked(checked);
    box->setStyleSheet("QCheckBox { color: #f0f0f2; font-size: 13px; }"
                       "QCheckBox::indicator { width: 16px; height: 16px; }");
    return box;
}

} // namespace

MousePanel::MousePanel()
{
    m_settings = readInputSettings().mouse;

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    for (const InputDevice &device : enumerateInputDevices()) {
        if (device.mouse || device.touchpad)
            m_devices << device;
    }

    buildDevice(layout);
    buildButtons(layout);
    buildBehaviour(layout);
    layout->addStretch();

    selectDevice(0);
}

void MousePanel::buildDevice(QVBoxLayout *layout)
{
    QStringList names;
    for (const InputDevice &device : m_devices)
        names << QString("%1  (%2)").arg(device.name, deviceId(device));
    if (names.isEmpty())
        names << "No mouse detected";

    m_device = makeCombo(names);
    auto *row = new QHBoxLayout;
    row->setSpacing(10);
    row->addWidget(makeFieldLabel("Device"));
    row->addWidget(m_device, 1);
    layout->addLayout(row);

    m_view = new DeviceView;
    m_view->onPick = [this](int part) { selectButton(part); };
    layout->addWidget(m_view);

    connect(m_device, &QComboBox::currentIndexChanged, this,
            [this](int index) { selectDevice(index); });
}

void MousePanel::buildButtons(QVBoxLayout *layout)
{
    addSection(layout, "Buttons");

    m_buttonHint = makeStatus();
    m_buttonHint->setText("Click a button on the model to spin it into view.");
    layout->addWidget(m_buttonHint);

    m_buttonBox = new QWidget;
    new QGridLayout(m_buttonBox);
    layout->addWidget(m_buttonBox);
    layout->addSpacing(10);
}

void MousePanel::buildBehaviour(QVBoxLayout *layout)
{
    addSection(layout, "Pointer");

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(18);
    grid->setVerticalSpacing(8);
    grid->setColumnMinimumWidth(0, 150);
    grid->setColumnStretch(1, 1);

    m_speed = new QSlider(Qt::Horizontal);
    m_speed->setRange(-100, 100);
    m_speed->setValue(int(m_settings.accelSpeed * 100));
    m_speed->setStyleSheet(kSliderStyle);
    m_speedValue = makeStatus();
    m_speedValue->setText(QString::number(m_settings.accelSpeed, 'f', 2));
    auto *speedRow = new QHBoxLayout;
    speedRow->addWidget(m_speed, 1);
    speedRow->addWidget(m_speedValue);

    m_profile = makeCombo({ "adaptive", "flat" });
    m_profile->setCurrentText(m_settings.accelProfile);
    m_scrollMethod = makeCombo({ "wheel", "button", "edge", "no-scroll" });
    m_scrollMethod->setCurrentText(m_settings.scrollMethod);
    m_buttonMap = makeCombo({ "lrm", "lmr" });
    m_buttonMap->setCurrentText(m_settings.buttonMap);

    grid->addWidget(makeFieldLabel("Sensitivity"), 0, 0);
    grid->addLayout(speedRow, 0, 1);
    grid->addWidget(makeFieldLabel("Acceleration"), 1, 0);
    grid->addWidget(m_profile, 1, 1);
    grid->addWidget(makeFieldLabel("Scroll method"), 2, 0);
    grid->addWidget(m_scrollMethod, 2, 1);
    grid->addWidget(makeFieldLabel("Button order"), 3, 0);
    grid->addWidget(m_buttonMap, 3, 1);
    layout->addLayout(grid);

    auto *natural = makeCheck("Natural scrolling", m_settings.naturalScroll);
    auto *left = makeCheck("Left-handed", m_settings.leftHanded);
    auto *middle = makeCheck("Middle-click emulation", m_settings.middleEmulation);
    auto *checks = new QHBoxLayout;
    checks->addWidget(natural);
    checks->addWidget(left);
    checks->addWidget(middle);
    checks->addStretch();
    layout->addLayout(checks);
    layout->addSpacing(10);

    addSection(layout, "Hardware");
    auto *hint = makeStatus();
    hint->setText("DPI and polling rate are written for ratbagd — they only take on mice "
                  "libratbag supports. 0 leaves the device as it is.");
    layout->addWidget(hint);

    m_dpi = new QSpinBox;
    m_dpi->setRange(0, 32000);
    m_dpi->setSingleStep(100);
    m_dpi->setValue(m_settings.dpi);
    m_dpi->setStyleSheet(kSpinStyle);

    m_rate = makeCombo({ "Leave alone", "125 Hz", "250 Hz", "500 Hz", "1000 Hz" });
    const QList<int> rates = { 0, 125, 250, 500, 1000 };
    m_rate->setCurrentIndex(std::max(0, int(rates.indexOf(m_settings.pollingRate))));

    auto *hw = new QGridLayout;
    hw->setHorizontalSpacing(18);
    hw->setColumnMinimumWidth(0, 150);
    hw->setColumnStretch(1, 1);
    hw->addWidget(makeFieldLabel("DPI"), 0, 0);
    hw->addWidget(m_dpi, 0, 1);
    hw->addWidget(makeFieldLabel("Polling rate"), 1, 0);
    hw->addWidget(m_rate, 1, 1);
    layout->addLayout(hw);

    connect(m_speed, &QSlider::valueChanged, this, [this](int value) {
        m_settings.accelSpeed = value / 100.0;
        m_speedValue->setText(QString::number(m_settings.accelSpeed, 'f', 2));
        changed();
    });
    connect(m_profile, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        m_settings.accelProfile = text;
        changed();
    });
    connect(m_scrollMethod, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        m_settings.scrollMethod = text;
        changed();
    });
    connect(m_buttonMap, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        m_settings.buttonMap = text;
        changed();
    });
    connect(natural, &QCheckBox::toggled, this, [this](bool on) {
        m_settings.naturalScroll = on;
        changed();
    });
    connect(left, &QCheckBox::toggled, this, [this](bool on) {
        m_settings.leftHanded = on;
        changed();
    });
    connect(middle, &QCheckBox::toggled, this, [this](bool on) {
        m_settings.middleEmulation = on;
        changed();
    });
    connect(m_dpi, &QSpinBox::valueChanged, this, [this](int value) {
        m_settings.dpi = value;
        changed();
    });
    connect(m_rate, &QComboBox::currentIndexChanged, this, [this, rates](int index) {
        m_settings.pollingRate = rates.value(index);
        changed();
    });
}

/* Rebuilds the model and the per-button rows for the newly picked device. */
void MousePanel::selectDevice(int index)
{
    if (index < 0 || index >= m_devices.size()) {
        m_view->setMesh(Mesh());
        return;
    }

    const InputDevice &device = m_devices[index];
    m_settings.device = device.name;
    m_settings.id = deviceId(device);
    m_view->setMesh(buildMouseMesh(device));
    m_view->turnTo(0.5f, -0.6f);

    auto *grid = qobject_cast<QGridLayout *>(m_buttonBox->layout());
    while (QLayoutItem *item = grid->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    m_actions.clear();

    int row = 0;
    for (int code : device.buttons) {
        auto *label = makeFieldLabel(buttonLabel(code));
        QComboBox *combo = actionCombo(m_settings.actions.value(code));
        const bool bindable = !keydButtonName(code).isEmpty();
        combo->setEnabled(bindable);
        if (!bindable)
            combo->setToolTip("keyd cannot bind this button.");

        grid->addWidget(label, row, 0);
        grid->addWidget(combo, row, 1);
        grid->setColumnMinimumWidth(0, 150);
        grid->setColumnStretch(1, 1);
        m_actions.insert(code, combo);
        ++row;

        connect(combo, &QComboBox::currentIndexChanged, this, [this, code](int action) {
            m_settings.actions.insert(code, inputActions().value(action).second);
            changed();
        });
        connect(combo, &QComboBox::highlighted, this, [this, code]() { selectButton(code); });
    }

    m_buttonHint->setText(device.buttons.isEmpty()
                              ? "This device reports no buttons."
                              : QString("%1 buttons detected. Click one on the model to edit it.")
                                    .arg(device.buttons.size()));
    changed();
}

void MousePanel::selectButton(int code)
{
    m_view->setSelected(code);
    float yaw = 0, pitch = 0;
    mouseViewFor(code, &yaw, &pitch);
    m_view->turnTo(yaw, pitch);

    if (QComboBox *combo = m_actions.value(code))
        combo->setFocus();
    m_buttonHint->setText(QString("Editing %1.").arg(buttonLabel(code)));
}

void MousePanel::changed()
{
    if (onChanged)
        onChanged();
}
