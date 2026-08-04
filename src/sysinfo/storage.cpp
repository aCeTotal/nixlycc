#include "sysinfo.h"
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStorageInfo>

static bool isPhysicalDisk(const QString &name)
{
    static const char *skip[] = {"loop", "ram", "zram", "dm-", "md", "fd", "sr"};
    for (const char *p : skip)
        if (name.startsWith(p))
            return false;
    return true;
}

static QString diskType(const QString &name, const QString &devPath, bool rotational)
{
    const bool usb = QFileInfo(devPath).symLinkTarget().contains("/usb");
    if (name.startsWith("nvme"))
        return "NVMe SSD";
    if (name.startsWith("mmcblk"))
        return "eMMC / SD";
    if (usb)
        return rotational ? "USB HDD" : "USB SSD";
    return rotational ? "SATA HDD" : "SATA SSD";
}

SysInfoSection storageSection()
{
    SysInfoSection s{"Storage", {}};
    QDir block("/sys/block");
    QStringList disks = block.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    disks.sort();
    for (const QString &name : disks) {
        if (!isPhysicalDisk(name))
            continue;
        const QString base = block.filePath(name);
        const double gb = sysReadTrimmed(base + "/size").toDouble() * 512.0 / 1e9;
        if (gb <= 0)
            continue;
        QString model =
            (sysReadTrimmed(base + "/device/vendor") + " " + sysReadTrimmed(base + "/device/model"))
                .trimmed();
        model.remove(QRegularExpression("^ATA\\s+"));
        const bool rotational = sysReadTrimmed(base + "/queue/rotational") == "1";
        QString desc = QString("%1 — %2 GB, %3")
                           .arg(model.isEmpty() ? "Unknown model" : model)
                           .arg(gb, 0, 'f', gb < 100 ? 1 : 0)
                           .arg(diskType(name, base, rotational));
        const QString fw = sysReadTrimmed(base + "/device/firmware_rev");
        if (!fw.isEmpty())
            desc += ", FW " + fw;
        const QString serial = sysReadTrimmed(base + "/device/serial");
        if (!serial.isEmpty())
            desc += ", S/N " + serial;
        s.rows.append(SysInfoRow{name, desc});
    }
    const QStorageInfo root = QStorageInfo::root();
    if (root.isValid())
        s.rows.append(SysInfoRow{
            "Root filesystem",
            QString("%1 GB free of %2 GB (%3)")
                .arg(root.bytesAvailable() / 1e9, 0, 'f', 1)
                .arg(root.bytesTotal() / 1e9, 0, 'f', 1)
                .arg(QString::fromUtf8(root.fileSystemType()))});
    return s;
}
