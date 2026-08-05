#include "task.h"

#include <QCoreApplication>
#include <QMetaObject>
#include <QThread>

/* Results are posted to the application object rather than to the caller: the
 * Git panel is destroyed when the page locks, and a job may still be running. */

void runAsync(std::function<QString()> work, std::function<void(const QString &)> done)
{
    QThread *thread = QThread::create([work = std::move(work), done = std::move(done)]() {
        const QString result = work();
        QMetaObject::invokeMethod(
            qApp, [done, result]() { done(result); }, Qt::QueuedConnection);
    });
    QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

void postToGui(std::function<void()> fn)
{
    QMetaObject::invokeMethod(qApp, std::move(fn), Qt::QueuedConnection);
}
