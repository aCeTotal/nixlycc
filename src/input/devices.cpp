#include "devices.h"

#include <QDir>
#include <algorithm>

#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace {

const int kMouseButtons[] = { BTN_LEFT,    BTN_RIGHT, BTN_MIDDLE, BTN_SIDE,
                              BTN_EXTRA,   BTN_FORWARD, BTN_BACK, BTN_TASK };

bool bitSet(const unsigned long *bits, int bit)
{
    const int perLong = 8 * sizeof(unsigned long);
    return (bits[bit / perLong] >> (bit % perLong)) & 1;
}

} // namespace

QString buttonLabel(int code)
{
    switch (code) {
    case BTN_LEFT:
        return "Left";
    case BTN_RIGHT:
        return "Right";
    case BTN_MIDDLE:
        return "Middle";
    case BTN_SIDE:
        return "Side 1";
    case BTN_EXTRA:
        return "Side 2";
    case BTN_FORWARD:
        return "Forward";
    case BTN_BACK:
        return "Back";
    case BTN_TASK:
        return "Task";
    default:
        return QString("Button %1").arg(code);
    }
}

QString deviceId(const InputDevice &device)
{
    return QString("%1:%2")
        .arg(device.vendor, 4, 16, QChar('0'))
        .arg(device.product, 4, 16, QChar('0'));
}

QList<InputDevice> enumerateInputDevices()
{
    QList<InputDevice> devices;
    const QStringList nodes =
        QDir("/dev/input").entryList({ "event*" }, QDir::System, QDir::Name);

    for (const QString &node : nodes) {
        const QString path = "/dev/input/" + node;
        const int fd = open(path.toUtf8().constData(), O_RDONLY | O_CLOEXEC);
        if (fd < 0)
            continue;

        InputDevice dev;
        dev.node = path;

        char name[256] = { 0 };
        if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), name) >= 0)
            dev.name = QString::fromUtf8(name).trimmed();

        input_id id = {};
        if (ioctl(fd, EVIOCGID, &id) >= 0) {
            dev.vendor = id.vendor;
            dev.product = id.product;
        }

        unsigned long evBits[(EV_MAX + 8 * sizeof(unsigned long)) / (8 * sizeof(unsigned long))] = { 0 };
        unsigned long keyBits[(KEY_MAX + 8 * sizeof(unsigned long)) / (8 * sizeof(unsigned long))] = { 0 };
        unsigned long relBits[(REL_MAX + 8 * sizeof(unsigned long)) / (8 * sizeof(unsigned long))] = { 0 };
        unsigned long absBits[(ABS_MAX + 8 * sizeof(unsigned long)) / (8 * sizeof(unsigned long))] = { 0 };
        ioctl(fd, EVIOCGBIT(0, sizeof(evBits)), evBits);
        ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBits)), keyBits);
        ioctl(fd, EVIOCGBIT(EV_REL, sizeof(relBits)), relBits);
        ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absBits)), absBits);
        close(fd);

        for (int code : kMouseButtons) {
            if (bitSet(keyBits, code))
                dev.buttons << code;
        }
        dev.wheel = bitSet(relBits, REL_WHEEL);
        dev.hwheel = bitSet(relBits, REL_HWHEEL);

        for (int code = KEY_ESC; code <= KEY_MAX; ++code) {
            if (bitSet(keyBits, code))
                ++dev.keys;
        }
        dev.numpad = bitSet(keyBits, KEY_KP0) && bitSet(keyBits, KEY_KPENTER);
        dev.fnRow = bitSet(keyBits, KEY_F1) && bitSet(keyBits, KEY_F12);
        dev.extraKeys = bitSet(keyBits, KEY_F13);

        /* A touchpad reports absolute finger positions; a mouse reports
         * relative motion with at least a left button. */
        dev.touchpad = bitSet(evBits, EV_ABS) && bitSet(keyBits, BTN_TOOL_FINGER);
        dev.mouse = !dev.touchpad && bitSet(relBits, REL_X) && bitSet(relBits, REL_Y)
            && bitSet(keyBits, BTN_LEFT);
        dev.keyboard = bitSet(keyBits, KEY_A) && bitSet(keyBits, KEY_Z)
            && bitSet(keyBits, KEY_ENTER) && bitSet(keyBits, KEY_SPACE);

        if (dev.mouse || dev.keyboard || dev.touchpad)
            devices.append(dev);
    }

    std::sort(devices.begin(), devices.end(), [](const InputDevice &a, const InputDevice &b) {
        if (a.mouse != b.mouse)
            return a.mouse;
        return a.keys > b.keys;
    });
    return devices;
}
