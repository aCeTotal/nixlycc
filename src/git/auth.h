#pragma once

class QString;

/* Verifies the password of the user running nixlycc against PAM.
 * Blocking: pam_unix delays a failed attempt by a couple of seconds, so call
 * this from a worker thread. */
bool verifyPassword(const QString &password);
