#include "sysinfo.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QSysInfo>

QString sysReadTrimmed(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(f.readAll()).trimmed();
}
static QString distroName()
{
    const QStringList lines = sysReadTrimmed("/etc/os-release").split('\n');
    for (const QString &line : lines)
        if (line.startsWith("PRETTY_NAME="))
            return QString(line.mid(12)).remove('"');
    return "Unknown";
}

static QString uptimeString()
{
    const double up = sysReadTrimmed("/proc/uptime").section(' ', 0, 0).toDouble();
    const int days = int(up) / 86400;
    const int hours = (int(up) % 86400) / 3600;
    const int mins = (int(up) % 3600) / 60;
    if (days > 0)
        return QString("%1 d %2 h %3 min").arg(days).arg(hours).arg(mins);
    return QString("%1 h %2 min").arg(hours).arg(mins);
}

static SysInfoSection systemSection()
{
    SysInfoSection s{"System", {}};
    s.rows.append(SysInfoRow{"Distribution", distroName()});
    s.rows.append(SysInfoRow{"Kernel", QSysInfo::kernelType() + " " + QSysInfo::kernelVersion()});
    const QString kbuild = sysReadTrimmed("/proc/sys/kernel/version");
    if (!kbuild.isEmpty())
        s.rows.append(SysInfoRow{"Kernel build", kbuild});
    s.rows.append(SysInfoRow{"Architecture", QSysInfo::currentCpuArchitecture()});
    s.rows.append(SysInfoRow{"Hostname", QSysInfo::machineHostName()});
    QDateTime installed = QFileInfo("/").birthTime();
    if (!installed.isValid())
        installed = QFileInfo("/etc/machine-id").birthTime();
    if (!installed.isValid())
        installed = QFileInfo("/etc/machine-id").lastModified();
    if (installed.isValid()) {
        const qint64 days = installed.daysTo(QDateTime::currentDateTime());
        const QString age = days >= 365
            ? QString("%1 y %2 d ago").arg(days / 365).arg(days % 365)
            : QString("%1 d ago").arg(days);
        s.rows.append(SysInfoRow{"Installed",
                                 installed.date().toString(Qt::ISODate) + " (" + age + ")"});
    }
    s.rows.append(SysInfoRow{"Uptime since boot", uptimeString()});
    return s;
}

static SysInfoSection cpuSection()
{
    const QStringList lines = sysReadTrimmed("/proc/cpuinfo").split('\n');
    QString model;
    int threads = 0, coresPerSocket = 0;
    QSet<QString> sockets;
    for (const QString &line : lines) {
        if (line.startsWith("processor"))
            ++threads;
        else if (model.isEmpty() && line.startsWith("model name"))
            model = line.section(':', 1).trimmed();
        else if (coresPerSocket == 0 && line.startsWith("cpu cores"))
            coresPerSocket = line.section(':', 1).trimmed().toInt();
        else if (line.startsWith("physical id"))
            sockets.insert(line.section(':', 1).trimmed());
    }
    SysInfoSection s{"Processor", {}};
    s.rows.append(SysInfoRow{"Model", model.isEmpty() ? "Unknown" : model});
    const int cores = coresPerSocket * qMax(1, int(sockets.size()));
    if (cores > 0)
        s.rows.append(SysInfoRow{"Cores / Threads", QString("%1 cores, %2 threads").arg(cores).arg(threads)});
    else
        s.rows.append(SysInfoRow{"Logical CPUs", QString::number(threads)});
    const double maxGhz =
        sysReadTrimmed("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq").toDouble() / 1e6;
    if (maxGhz > 0)
        s.rows.append(SysInfoRow{"Max frequency", QString::number(maxGhz, 'f', 2) + " GHz"});
    return s;
}

/* DIMM details come from DMI tables, readable only by root. dmidecode
   fails instantly without privileges, so this never stalls the page. */
static void addDimmRows(SysInfoSection &s)
{
    QProcess p;
    p.start("dmidecode", {"-t", "memory"});
    if (!p.waitForFinished(1000) || p.exitCode() != 0) {
        s.rows.append(SysInfoRow{"Modules", "Unavailable (DMI data requires root)"});
        return;
    }
    const QStringList blocks = QString::fromUtf8(p.readAllStandardOutput()).split("Memory Device");
    auto field = [](const QString &block, const QString &name) {
        const QStringList lines = block.split('\n');
        for (const QString &line : lines) {
            const QString t = line.trimmed();
            if (t.startsWith(name + ":"))
                return t.mid(name.size() + 1).trimmed();
        }
        return QString();
    };
    for (int i = 1; i < blocks.size(); ++i) {
        const QString size = field(blocks[i], "Size");
        if (size.isEmpty() || size.startsWith("No Module"))
            continue;
        const QString type = field(blocks[i], "Type");
        const QString speed = field(blocks[i], "Speed");
        const QString vendor = field(blocks[i], "Manufacturer");
        const QString part = field(blocks[i], "Part Number");
        QString desc = size;
        if (!type.isEmpty() && type != "Unknown")
            desc += " " + type;
        if (!speed.isEmpty() && speed != "Unknown")
            desc += " @ " + speed;
        if (!vendor.isEmpty() && vendor != "Unknown")
            desc += " — " + vendor;
        if (!part.isEmpty())
            desc += " " + part;
        const QString slot = field(blocks[i], "Locator");
        s.rows.append(SysInfoRow{slot.isEmpty() ? QString("Module %1").arg(i) : slot, desc});
    }
}

static SysInfoSection memorySection()
{
    SysInfoSection s{"Memory", {}};
    const QStringList lines = sysReadTrimmed("/proc/meminfo").split('\n');
    for (const QString &line : lines) {
        const double gib = line.section(':', 1).trimmed().section(' ', 0, 0).toDouble() / 1048576.0;
        if (line.startsWith("MemTotal:"))
            s.rows.append(SysInfoRow{"Total", QString::number(gib, 'f', 1) + " GiB"});
        else if (line.startsWith("SwapTotal:"))
            s.rows.append(SysInfoRow{"Swap", gib > 0 ? QString::number(gib, 'f', 1) + " GiB" : "None"});
    }
    addDimmRows(s);
    return s;
}

static SysInfoSection boardSection()
{
    SysInfoSection s{"Motherboard", {}};
    const QString dmi = "/sys/class/dmi/id/";
    const QString board =
        (sysReadTrimmed(dmi + "board_vendor") + " " + sysReadTrimmed(dmi + "board_name")).trimmed();
    if (!board.isEmpty())
        s.rows.append(SysInfoRow{"Model", board});
    const QString sys =
        (sysReadTrimmed(dmi + "sys_vendor") + " " + sysReadTrimmed(dmi + "product_name")).trimmed();
    if (!sys.isEmpty() && sys != board)
        s.rows.append(SysInfoRow{"System", sys});
    const QString bios =
        (sysReadTrimmed(dmi + "bios_vendor") + " " + sysReadTrimmed(dmi + "bios_version")).trimmed();
    const QString biosDate = sysReadTrimmed(dmi + "bios_date");
    if (!bios.isEmpty())
        s.rows.append(SysInfoRow{"BIOS", biosDate.isEmpty() ? bios : bios + " (" + biosDate + ")"});
    QString batt = "Unknown";
    const QStringList rtc = sysReadTrimmed("/proc/driver/rtc").split('\n');
    for (const QString &line : rtc)
        if (line.startsWith("batt_status")) {
            batt = line.section(':', 1).trimmed();
            if (batt == "okay")
                batt = "OK";
        }
    s.rows.append(SysInfoRow{"CMOS battery", batt});
    return s;
}

static QList<SysInfoSection> gpuSections()
{
    QList<SysInfoSection> out;
    QDir drm("/sys/class/drm");
    static const QRegularExpression cardRe("^card[0-9]+$");
    QStringList cards = drm.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    cards.sort();
    int idx = 0;
    for (const QString &card : cards) {
        if (!cardRe.match(card).hasMatch())
            continue;
        const QString dev = drm.filePath(card) + "/device";
        QString vendor = sysReadTrimmed(dev + "/vendor");
        QString device = sysReadTrimmed(dev + "/device");
        if (vendor.isEmpty())
            continue;
        vendor.remove("0x");
        device.remove("0x");
        const QString pciAddr = QFileInfo(dev).symLinkTarget().section('/', -1);
        const bool integrated = pciAddr.section(':', 1, 1) == "00";
        QString driver;
        const QStringList uevent = sysReadTrimmed(dev + "/uevent").split('\n');
        for (const QString &line : uevent)
            if (line.startsWith("DRIVER="))
                driver = line.mid(7);
        SysInfoSection s{QString("GPU %1 — %2").arg(idx).arg(integrated ? "Integrated" : "Dedicated"), {}};
        s.rows.append(SysInfoRow{"Model", pciDeviceName(vendor, device)});
        if (!driver.isEmpty()) {
            const QString drvVersion = sysReadTrimmed("/sys/module/" + driver + "/version");
            s.rows.append(SysInfoRow{
                "Driver", drvVersion.isEmpty() ? driver : driver + " " + drvVersion});
        }
        s.rows.append(SysInfoRow{"PCI address", pciAddr});
        const QString vram = sysReadTrimmed(dev + "/mem_info_vram_total");
        if (!vram.isEmpty())
            s.rows.append(SysInfoRow{"VRAM", QString::number(vram.toDouble() / 1073741824.0, 'f', 1) + " GiB"});
        out.append(s);
        ++idx;
    }
    return out;
}

QList<SysInfoSection> collectSysInfo()
{
    QList<SysInfoSection> sections;
    sections.append(systemSection());
    sections.append(cpuSection());
    sections.append(memorySection());
    sections.append(storageSection());
    sections.append(boardSection());
    sections.append(gpuSections());
    sections.append(networkSection());
    const SysInfoSection battery = batterySection();
    if (!battery.rows.isEmpty())
        sections.append(battery);
    return sections;
}
