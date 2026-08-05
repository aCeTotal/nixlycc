#pragma once

#include <QString>

/* The nixlyOS configuration repository — always ~/.nixlyos. */
QString repoPath();

/* Push URL of "origin", or an empty string when there is no remote. */
QString repoRemote();

/* Global git identity, used to prefill the fields. */
QString gitConfigValue(const QString &key);

/* Initialises ~/.nixlyos as a git repository, points origin at url, commits
 * anything uncommitted and pushes. Returns an empty string on success, else
 * the error text. Blocking — the push talks to the network. */
QString createRepo(const QString &url, const QString &name, const QString &email);

/* ~/.local/nixlyos/git.conf — read by scripts/update.sh after a rebuild. */
bool autoCommitEnabled();
QString setAutoCommit(bool enabled);
