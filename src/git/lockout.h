#pragma once

/* Brute-force protection for the Git page password prompt: three wrong
 * passwords lock it for five minutes. The counter lives in
 * ~/.local/state/nixlycc/gitauth.conf, so it survives closing the panel and
 * restarting nixlycc. */

/* Seconds left of the lockout, or 0 when the prompt is usable. */
int lockoutRemaining();

/* Counts one rejected password, starting the lockout on the third. */
void registerFailure();

/* Called after PAM accepts — drops the counter. */
void clearFailures();
