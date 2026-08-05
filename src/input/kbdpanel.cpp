#include "kbdpanel.h"
#include "../git/style.h"
#include "view3d.h"

#include <QColorDialog>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
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

/* The layouts xkb ships that are worth offering without a search box. */
const QStringList &layouts()
{
    static const QStringList list = { "no", "us", "gb", "se", "dk", "fi", "de", "fr",
                                      "es", "it", "nl", "pl", "pt", "cz", "ru" };
    return list;
}

} // namespace

KeyboardPanel::KeyboardPanel()
{
    m_settings = readInputSettings().keyboard;

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    for (const InputDevice &device : enumerateInputDevices()) {
        if (device.keyboard)
            m_devices << device;
    }

    buildDevice(layout);
    buildKey(layout);
    buildLayout(layout);
    buildLight(layout);
    layout->addStretch();

    selectDevice(0);
}

void KeyboardPanel::buildDevice(QVBoxLayout *layout)
{
    QStringList names;
    for (const InputDevice &device : m_devices)
        names << QString("%1  (%2 keys)").arg(device.name).arg(device.keys);
    if (names.isEmpty())
        names << "No keyboard detected";

    m_device = makeCombo(names);
    auto *row = new QHBoxLayout;
    row->setSpacing(10);
    row->addWidget(makeFieldLabel("Device"));
    row->addWidget(m_device, 1);
    layout->addLayout(row);

    m_view = new DeviceView;
    m_view->setMinimumHeight(320);
    m_view->onPick = [this](int part) { selectKey(part); };
    layout->addWidget(m_view);

    connect(m_device, &QComboBox::currentIndexChanged, this,
            [this](int index) { selectDevice(index); });
}

void KeyboardPanel::buildKey(QVBoxLayout *layout)
{
    addSection(layout, "Key");

    m_keyLabel = makeStatus();
    m_keyLabel->setText("Click a key on the model to remap it.");
    layout->addWidget(m_keyLabel);

    QStringList names;
    for (const auto &action : inputActions())
        names << action.first;
    m_keyAction = makeCombo(names);
    m_keyAction->setEnabled(false);

    auto *row = new QHBoxLayout;
    row->setSpacing(10);
    row->addWidget(makeFieldLabel("Sends"));
    row->addWidget(m_keyAction, 1);
    layout->addLayout(row);
    layout->addSpacing(10);

    connect(m_keyAction, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (m_key < 0)
            return;
        m_settings.remaps.insert(m_key, inputActions().value(index).second);
        changed();
    });
}

void KeyboardPanel::buildLayout(QVBoxLayout *layout)
{
    addSection(layout, "Layout and repeat");

    m_layout = makeCombo(layouts());
    m_layout->setCurrentText(m_settings.layout);
    m_variant = makeField("nodeadkeys, colemak, …", m_settings.variant);

    m_delay = new QSpinBox;
    m_delay->setRange(100, 1000);
    m_delay->setSingleStep(10);
    m_delay->setValue(m_settings.repeatDelay);
    m_delay->setStyleSheet(kSpinStyle);

    m_rate = new QSpinBox;
    m_rate->setRange(10, 200);
    m_rate->setValue(m_settings.repeatRate);
    m_rate->setStyleSheet(kSpinStyle);

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(18);
    grid->setVerticalSpacing(8);
    grid->setColumnMinimumWidth(0, 150);
    grid->setColumnStretch(1, 1);
    grid->addWidget(makeFieldLabel("Layout"), 0, 0);
    grid->addWidget(m_layout, 0, 1);
    grid->addWidget(makeFieldLabel("Variant"), 1, 0);
    grid->addWidget(m_variant, 1, 1);
    grid->addWidget(makeFieldLabel("Repeat delay (ms)"), 2, 0);
    grid->addWidget(m_delay, 2, 1);
    grid->addWidget(makeFieldLabel("Repeat rate (Hz)"), 3, 0);
    grid->addWidget(m_rate, 3, 1);
    layout->addLayout(grid);
    layout->addSpacing(10);

    connect(m_layout, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        m_settings.layout = text;
        changed();
    });
    connect(m_variant, &QLineEdit::textChanged, this, [this](const QString &text) {
        m_settings.variant = text.trimmed();
        changed();
    });
    connect(m_delay, &QSpinBox::valueChanged, this, [this](int value) {
        m_settings.repeatDelay = value;
        changed();
    });
    connect(m_rate, &QSpinBox::valueChanged, this, [this](int value) {
        m_settings.repeatRate = value;
        changed();
    });
}

void KeyboardPanel::buildLight(QVBoxLayout *layout)
{
    addSection(layout, "Lighting");

    auto *hint = makeStatus();
    hint->setText("Driven through OpenRGB — it only reaches keyboards OpenRGB has a driver for.");
    layout->addWidget(hint);

    m_rgbMode = makeCombo({ "off", "static", "breathing", "rainbow" });
    m_rgbMode->setCurrentText(m_settings.rgbMode);

    m_rgbColor = makeButton("Colour");
    m_rgbColor->setStyleSheet(QString("QPushButton { background-color: %1; border: 1px solid"
                                      " rgba(255,255,255,60); border-radius: 10px;"
                                      " padding: 9px 18px; color: #10121a; font-size: 13px; }")
                                  .arg(m_settings.rgbColor));

    m_brightness = new QSlider(Qt::Horizontal);
    m_brightness->setRange(0, 100);
    m_brightness->setValue(m_settings.brightness);
    m_brightness->setStyleSheet(kSliderStyle);

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(18);
    grid->setVerticalSpacing(8);
    grid->setColumnMinimumWidth(0, 150);
    grid->setColumnStretch(1, 1);
    grid->addWidget(makeFieldLabel("Mode"), 0, 0);
    grid->addWidget(m_rgbMode, 0, 1);
    grid->addWidget(makeFieldLabel("Colour"), 1, 0);
    grid->addWidget(m_rgbColor, 1, 1);
    grid->addWidget(makeFieldLabel("Brightness"), 2, 0);
    grid->addWidget(m_brightness, 2, 1);
    layout->addLayout(grid);

    connect(m_rgbMode, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        m_settings.rgbMode = text;
        changed();
    });
    connect(m_brightness, &QSlider::valueChanged, this, [this](int value) {
        m_settings.brightness = value;
        changed();
    });
    connect(m_rgbColor, &QPushButton::clicked, this, [this]() {
        const QColor picked = QColorDialog::getColor(QColor(m_settings.rgbColor), this, "Lighting");
        if (!picked.isValid())
            return;
        m_settings.rgbColor = picked.name();
        m_rgbColor->setStyleSheet(
            QString("QPushButton { background-color: %1; border: 1px solid rgba(255,255,255,60);"
                    " border-radius: 10px; padding: 9px 18px; color: #10121a; font-size: 13px; }")
                .arg(m_settings.rgbColor));
        changed();
    });
}

void KeyboardPanel::selectDevice(int index)
{
    if (index < 0 || index >= m_devices.size()) {
        m_view->setMesh(Mesh());
        return;
    }

    const InputDevice &device = m_devices[index];
    m_settings.device = device.name;
    m_settings.id = deviceId(device);
    m_model = buildKeyboardModel(device);
    m_view->setMesh(m_model.mesh);
    m_view->turnTo(0.0f, -1.0f);
    m_keyLabel->setText(QString("%1 keys detected%2. Click one on the model to remap it.")
                            .arg(device.keys)
                            .arg(device.numpad ? ", full size" : ""));
    changed();
}

void KeyboardPanel::selectKey(int code)
{
    m_key = code;
    m_view->setSelected(code);
    float yaw = 0, pitch = 0;
    keyboardViewFor(m_model, code, &yaw, &pitch);
    m_view->turnTo(yaw, pitch);

    const QString name = keydKeyName(code);
    m_keyAction->setEnabled(!name.isEmpty());
    m_keyLabel->setText(name.isEmpty()
                            ? "keyd cannot bind that key."
                            : QString("Editing %1.").arg(name.toUpper()));

    const QString current = m_settings.remaps.value(code);
    for (int i = 0; i < inputActions().size(); ++i) {
        if (inputActions()[i].second == current) {
            m_keyAction->setCurrentIndex(i);
            break;
        }
    }
}

void KeyboardPanel::changed()
{
    if (onChanged)
        onChanged();
}
