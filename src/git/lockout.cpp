#include "lockout.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSettings>

namespace {

const int kMaxFailures = 3;
const int kLockSeconds = 300;

QString statePath()
{
    const QString path = QDir::homePath() + "/.local/state/nixlycc/gitauth.conf";
    QDir().mkpath(QFileInfo(path).absolutePath());
    return path;
}

} // namespace

int lockoutRemaining()
{
    QSettings settings(statePath(), QSettings::IniFormat);
    const qint64 left = settings.value("until", 0).toLongLong() - QDateTime::currentSecsSinceEpoch();
    return left > 0 ? int(left) : 0;
}

void registerFailure()
{
    QSettings settings(statePath(), QSettings::IniFormat);
    const int failures = settings.value("failures", 0).toInt() + 1;
    if (failures < kMaxFailures) {
        settings.setValue("failures", failures);
        return;
    }
    settings.setValue("failures", 0);
    settings.setValue("until", QDateTime::currentSecsSinceEpoch() + kLockSeconds);
}

void clearFailures()
{
    QSettings settings(statePath(), QSettings::IniFormat);
    settings.setValue("failures", 0);
    settings.setValue("until", 0);
}
