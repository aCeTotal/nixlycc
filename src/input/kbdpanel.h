#pragma once

#include "devices.h"
#include "kbdmesh.h"
#include "settings.h"

#include <QWidget>
#include <functional>

class DeviceView;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSlider;
class QSpinBox;
class QVBoxLayout;

/* Keyboard half of the page: the detected keyboard as a model you can click,
 * per-key remapping, layout, repeat, and the RGB settings OpenRGB can drive. */
class KeyboardPanel : public QWidget {
public:
    KeyboardPanel();

    KeyboardSettings settings() const { return m_settings; }
    std::function<void()> onChanged;

private:
    void buildDevice(QVBoxLayout *layout);
    void buildKey(QVBoxLayout *layout);
    void buildLayout(QVBoxLayout *layout);
    void buildLight(QVBoxLayout *layout);
    void selectDevice(int index);
    void selectKey(int code);
    void changed();

    QList<InputDevice> m_devices;
    KeyboardModel m_model;
    KeyboardSettings m_settings;
    int m_key = -1;

    QComboBox *m_device;
    DeviceView *m_view;
    QLabel *m_keyLabel;
    QComboBox *m_keyAction;
    QComboBox *m_layout;
    QLineEdit *m_variant;
    QSpinBox *m_delay;
    QSpinBox *m_rate;
    QComboBox *m_rgbMode;
    QPushButton *m_rgbColor;
    QSlider *m_brightness;
};
