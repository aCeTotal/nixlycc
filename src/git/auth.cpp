#include "auth.h"

#include <QByteArray>
#include <QFile>
#include <QString>

#include <security/pam_appl.h>
#include <cstdlib>
#include <cstring>

namespace {

/* PAM asks for the secret through this callback; there is no terminal, so the
 * password collected by the UI is handed straight back. */
int conversation(int count, const struct pam_message **msg,
                 struct pam_response **resp, void *data)
{
    const auto *password = static_cast<const QByteArray *>(data);
    auto *replies = (struct pam_response *)calloc(count, sizeof(struct pam_response));
    if (!replies)
        return PAM_BUF_ERR;

    for (int i = 0; i < count; ++i) {
        if (msg[i]->msg_style == PAM_PROMPT_ECHO_OFF)
            replies[i].resp = strdup(password->constData());
    }
    *resp = replies;
    return PAM_SUCCESS;
}

const char *serviceName()
{
    /* nixlyOS ships a minimal pam_unix-only service for its lock screen —
     * the same shape this needs. "login" is the portable fallback. */
    if (QFile::exists("/etc/pam.d/nixly-lockscreen"))
        return "nixly-lockscreen";
    return "login";
}

} // namespace

bool verifyPassword(const QString &password)
{
    QByteArray secret = password.toUtf8();
    struct pam_conv conv = { conversation, &secret };

    QByteArray user = qgetenv("USER");
    if (user.isEmpty())
        user = qgetenv("LOGNAME");
    if (user.isEmpty())
        return false;

    pam_handle_t *handle = nullptr;
    if (pam_start(serviceName(), user.constData(), &conv, &handle) != PAM_SUCCESS)
        return false;

    const int rc = pam_authenticate(handle, PAM_DISALLOW_NULL_AUTHTOK);
    pam_end(handle, rc);

    secret.fill('\0');
    return rc == PAM_SUCCESS;
}
