#include "accounts.h"

#include <QDir>
#include <QFile>
#include <QRegularExpression>

namespace {

QString readAll(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(file.readAll());
}

/* The body of users.users.<name> = { … }, so the fields can be read out. */
QString blockFor(const QString &text, const QString &name)
{
    const QRegularExpression head(
        QString(R"(users\.users\.%1\s*=\s*\{)").arg(QRegularExpression::escape(name)));
    const QRegularExpressionMatch match = head.match(text);
    if (!match.hasMatch())
        return QString();

    int depth = 0;
    const int start = match.capturedEnd() - 1;
    for (int i = start; i < text.size(); ++i) {
        if (text[i] == '{')
            ++depth;
        else if (text[i] == '}' && --depth == 0)
            return text.mid(start + 1, i - start - 1);
    }
    return QString();
}

} // namespace

QString usersModulePath()
{
    return QDir::homePath() + "/.nixlyos/modules/core/users.nix";
}

QList<Account> declaredAccounts()
{
    const QString text = readAll(usersModulePath());
    QList<Account> accounts;

    static const QRegularExpression decl(R"(users\.users\.([A-Za-z0-9_-]+)\s*=\s*\{)");
    QRegularExpressionMatchIterator it = decl.globalMatch(text);
    while (it.hasNext()) {
        Account account;
        account.name = it.next().captured(1);

        const QString body = blockFor(text, account.name);
        static const QRegularExpression desc(R"rx(description\s*=\s*"([^"]*)")rx");
        const QRegularExpressionMatch descMatch = desc.match(body);
        if (descMatch.hasMatch())
            account.description = descMatch.captured(1);
        account.admin = body.contains("\"wheel\"");

        accounts.append(account);
    }

    /* /etc/passwd says whether the account actually exists yet. */
    for (const QString &line : readAll("/etc/passwd").split('\n', Qt::SkipEmptyParts)) {
        const QStringList fields = line.split(':');
        if (fields.size() < 6)
            continue;
        for (Account &account : accounts) {
            if (account.name == fields[0]) {
                account.uid = fields[2].toInt();
                account.home = fields[5];
            }
        }
    }
    return accounts;
}
