#include "sysinfo.h"
#include <QDir>

SysInfoSection batterySection()
{
    SysInfoSection s{"Battery", {}};
    QDir ps("/sys/class/power_supply");
    QStringList entries = ps.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    entries.sort();
    for (const QString &name : entries) {
        if (!name.startsWith("BAT"))
            continue;
        const QString base = ps.filePath(name);
        const QString model = sysReadTrimmed(base + "/model_name");
        if (!model.isEmpty())
            s.rows.append(SysInfoRow{name + " model", model});
        const QString capacity = sysReadTrimmed(base + "/capacity");
        const QString status = sysReadTrimmed(base + "/status");
        if (!capacity.isEmpty())
            s.rows.append(SysInfoRow{name + " charge",
                                     capacity + " %" + (status.isEmpty() ? "" : " (" + status + ")")});
        double full = sysReadTrimmed(base + "/energy_full").toDouble();
        double design = sysReadTrimmed(base + "/energy_full_design").toDouble();
        if (full <= 0 || design <= 0) {
            full = sysReadTrimmed(base + "/charge_full").toDouble();
            design = sysReadTrimmed(base + "/charge_full_design").toDouble();
        }
        if (full > 0 && design > 0)
            s.rows.append(SysInfoRow{name + " health",
                                     QString::number(full / design * 100.0, 'f', 0) + " % of design capacity"});
        const QString cycles = sysReadTrimmed(base + "/cycle_count");
        if (!cycles.isEmpty() && cycles != "0")
            s.rows.append(SysInfoRow{name + " cycles", cycles});
    }
    return s;
}
