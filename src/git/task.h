#pragma once

#include <QString>
#include <functional>

/* Runs work() on a worker thread and delivers its result to done() on the GUI
 * thread. Keeps PAM's failure delay, git's network calls and the rebuild off
 * the event loop.
 *
 * done() may fire after the widgets it touches are gone — the Git panel is
 * destroyed when the page locks — so guard them with a QPointer. */
void runAsync(std::function<QString()> work, std::function<void(const QString &)> done);

/* Hands a callback back to the GUI thread — for progress reported from inside
 * a runAsync worker. Same lifetime caveat. */
void postToGui(std::function<void()> fn);
