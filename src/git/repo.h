#pragma once

#include <QString>

/* The nixlyOS configuration repository — always ~/.nixlyos. */
QString repoPath();

/* Push URL of "origin", or an empty string when there is no remote. */
QString repoRemote();

/* Rewrites an https:// or ssh:// URL into the scp-style git@host:path form, so
 * pushes use the SSH key instead of asking for a password. Anything already in
 * that form, and anything unrecognised, is returned unchanged. */
QString toSshUrl(const QString &url);

/* Global git identity, used to prefill the fields. */
QString gitConfigValue(const QString &key);

/* Initialises ~/.nixlyos as a git repository, points origin at url — converted
 * to SSH first — commits anything uncommitted and pushes. Returns an empty
 * string on success, else the error text. Blocking — the push talks to the
 * network. */
QString createRepo(const QString &url, const QString &name, const QString &email);

/* Adds a file to the ~/.nixlyos index. A flake built from a dirty git tree
 * only sees tracked files, so a module nixlycc just created is invisible to
 * nixos-rebuild until this has run. Returns an empty string on success. */
QString stageFile(const QString &path);

/* Stages everything in ~/.nixlyos, commits it and pushes — so the settings
 * nixlycc just wrote are not left behind locally. Does nothing when
 * ~/.nixlyos is not a repository or when there is nothing to commit. Returns
 * an empty string on success, else the error text. Blocking — the push talks
 * to the network. */
QString commitAndPush(const QString &message);

/* ~/.local/nixlyos/git.conf — read by scripts/update.sh after a rebuild. */
bool autoCommitEnabled();
QString setAutoCommit(bool enabled);
