#pragma once

#include "devices.h"
#include "settings.h"

#include <QHash>
#include <QWidget>
#include <functional>

class DeviceView;
class QComboBox;
class QLabel;
class QSlider;
class QSpinBox;
class QVBoxLayout;

/* Mouse half of the page: the detected device, a model of it with one button
 * per button it actually has, and every libinput and hardware setting that
 * can be written to the module. */
class MousePanel : public QWidget {
public:
    MousePanel();

    MouseSettings settings() const { return m_settings; }
    std::function<void()> onChanged;

private:
    void buildDevice(QVBoxLayout *layout);
    void buildButtons(QVBoxLayout *layout);
    void buildBehaviour(QVBoxLayout *layout);
    void selectDevice(int index);
    void selectButton(int code);
    void changed();

    QList<InputDevice> m_devices;
    MouseSettings m_settings;

    QComboBox *m_device;
    DeviceView *m_view;
    QWidget *m_buttonBox;
    QLabel *m_buttonHint;
    QHash<int, QComboBox *> m_actions;
    QSlider *m_speed;
    QLabel *m_speedValue;
    QComboBox *m_profile;
    QComboBox *m_scrollMethod;
    QComboBox *m_buttonMap;
    QSpinBox *m_dpi;
    QComboBox *m_rate;
};
