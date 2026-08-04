#include "sysinfo.h"
#include <QFile>

static const char *kPciIdPaths[] = {
#ifdef HWDATA_DIR
    HWDATA_DIR "/pci.ids",
#endif
    "/run/current-system/sw/share/hwdata/pci.ids",
    "/usr/share/hwdata/pci.ids",
    "/usr/share/misc/pci.ids",
};

static QString fallbackVendor(const QString &vendorId)
{
    if (vendorId == "8086") return "Intel";
    if (vendorId == "1002") return "AMD";
    if (vendorId == "10de") return "NVIDIA";
    return "Vendor 0x" + vendorId;
}

QString pciDeviceName(const QString &vendorId, const QString &deviceId)
{
    const QByteArray vid = vendorId.toLower().toUtf8();
    const QByteArray did = deviceId.toLower().toUtf8();
    QString vendorName, deviceName;

    for (const char *path : kPciIdPaths) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        bool inVendor = false;
        while (!f.atEnd()) {
            const QByteArray line = f.readLine();
            if (line.startsWith('#') || line.trimmed().isEmpty())
                continue;
            if (!line.startsWith('\t')) {
                if (inVendor)
                    break;
                if (line.left(4).toLower() == vid) {
                    inVendor = true;
                    vendorName = QString::fromUtf8(line.mid(4).trimmed());
                }
            } else if (inVendor && !line.startsWith("\t\t")) {
                if (line.mid(1, 4).toLower() == did) {
                    deviceName = QString::fromUtf8(line.mid(5).trimmed());
                    break;
                }
            }
        }
        if (!vendorName.isEmpty())
            break;
    }

    if (!deviceName.isEmpty())
        return vendorName + " " + deviceName;
    if (!vendorName.isEmpty())
        return vendorName + " (device 0x" + deviceId + ")";
    return fallbackVendor(vendorId) + " (device 0x" + deviceId + ")";
}
