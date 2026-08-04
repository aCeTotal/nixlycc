#pragma once

#include <QList>
#include <QString>

struct SysInfoRow {
    QString label;
    QString value;
};

struct SysInfoSection {
    QString title;
    QList<SysInfoRow> rows;
};

QList<SysInfoSection> collectSysInfo();
QString pciDeviceName(const QString &vendorId, const QString &deviceId);
QString sysReadTrimmed(const QString &path);
SysInfoSection storageSection();
SysInfoSection networkSection();
SysInfoSection batterySection();
