#pragma once

#include <QString>
#include <QVector>

/* A hosted git service, as far as the panel cares: where SSH connects, where
 * the public key is pasted, and what a repository URL looks like there. */
struct GitService {
    QString name;
    QString slug;
    QString host;
    QString keyPage;
    QString urlHint;
};

/* GitHub first — it is the default selection. */
const QVector<GitService> &gitServices();

/* Index of the service using this SSH host, or 0 when none does. */
int serviceIndexForHost(const QString &host);
