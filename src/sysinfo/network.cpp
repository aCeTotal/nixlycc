#include "sysinfo.h"
#include <QDir>
#include <QFileInfo>

SysInfoSection networkSection()
{
    SysInfoSection s{"Network", {}};
    QDir net("/sys/class/net");
    QStringList ifaces = net.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    ifaces.sort();
    for (const QString &iface : ifaces) {
        const QString base = net.filePath(iface);
        /* Skip loopback and virtual interfaces (bridges, veth, VPN tunnels). */
        if (iface == "lo" || QFileInfo(base).symLinkTarget().contains("/virtual/"))
            continue;
        QString model;
        QString vendor = sysReadTrimmed(base + "/device/vendor");
        QString device = sysReadTrimmed(base + "/device/device");
        if (vendor.startsWith("0x") && device.startsWith("0x")) {
            vendor.remove("0x");
            device.remove("0x");
            model = pciDeviceName(vendor, device);
        }
        if (model.isEmpty()) {
            const QStringList uevent = sysReadTrimmed(base + "/device/uevent").split('\n');
            for (const QString &line : uevent)
                if (line.startsWith("DRIVER="))
                    model = line.mid(7) + " device";
        }
        const bool wifi = QDir(base + "/phy80211").exists() || QDir(base + "/wireless").exists();
        QString desc = QString("%1 (%2)").arg(model.isEmpty() ? "Unknown" : model,
                                              wifi ? "Wi-Fi" : "Ethernet");
        const QString state = sysReadTrimmed(base + "/operstate");
        if (!state.isEmpty())
            desc += ", " + state;
        const QString speed = sysReadTrimmed(base + "/speed");
        if (!speed.isEmpty() && speed.toInt() > 0)
            desc += ", " + speed + " Mb/s";
        const QString mac = sysReadTrimmed(base + "/address");
        if (!mac.isEmpty())
            desc += ", MAC " + mac;
        s.rows.append(SysInfoRow{iface, desc});
    }
    return s;
}
