#pragma once

#include <QList>
#include <QString>

/* One bind: conf value plus display text. */
struct Bind {
    QString value;
    QString label;
};

/* ~/.local/nixlyos/gaming.conf, hot-reloaded by nixlytile. */
struct GamingConf {
    Bind talk;
    QList<Bind> voipMute;
};

GamingConf loadGamingConf();
bool saveGamingConf(const GamingConf &conf);
QString gamingConfPath();
