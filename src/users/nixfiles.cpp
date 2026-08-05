#include "nixfiles.h"
#include "../git/repo.h"
#include "accounts.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QRegularExpression>
#include <QSaveFile>

namespace {

QString repoDir()
{
    return QDir::homePath() + "/.nixlyos";
}

QString readAll(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(file.readAll());
}

QString writeAll(const QString &path, const QString &text)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return QString("Could not write %1").arg(path);
    file.write(text.toUtf8());
    if (!file.commit())
        return QString("Could not write %1").arg(path);
    return QString();
}

QRegularExpression wholeWord(const QString &name)
{
    return QRegularExpression(QString(R"(\b%1\b)").arg(QRegularExpression::escape(name)));
}

QStringList nixFiles()
{
    QStringList files;
    QDirIterator it(repoDir(), { "*.nix" }, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        if (!path.contains("/.git/"))
            files << path;
    }
    files.sort();
    return files;
}

} // namespace

QStringList filesMentioning(const QString &user)
{
    QStringList hits;
    if (user.isEmpty())
        return hits;

    const QRegularExpression word = wholeWord(user);
    for (const QString &path : nixFiles()) {
        if (readAll(path).contains(word))
            hits << path;
    }
    return hits;
}

QString renameUserInFiles(const QString &oldName, const QString &newName)
{
    if (oldName.isEmpty() || newName.isEmpty())
        return "Both the old and the new user name are needed.";

    const QRegularExpression word = wholeWord(oldName);
    for (const QString &path : filesMentioning(oldName)) {
        QString text = readAll(path);
        text.replace(word, newName);
        const QString error = writeAll(path, text);
        if (!error.isEmpty())
            return error;
    }
    return QString();
}

/* The block mirrors the one users.nix already ships, minus the groups an
 * ordinary account has no business in. */
QString addUserToFiles(const QString &name, const QString &description, bool admin)
{
    static const QRegularExpression valid(R"(^[a-z_][a-z0-9_-]*$)");
    if (!valid.match(name).hasMatch())
        return "A user name must start with a letter and use lowercase letters, digits, - or _.";

    for (const Account &account : declaredAccounts()) {
        if (account.name == name)
            return QString("%1 is already declared.").arg(name);
    }

    const QString usersPath = usersModulePath();
    QString users = readAll(usersPath);
    if (users.isEmpty())
        return QString("Could not read %1").arg(usersPath);

    QString groups = "      \"networkmanager\"\n      \"video\"\n      \"audio\"\n"
                     "      \"render\"\n      \"input\"\n";
    if (admin)
        groups.prepend("      \"wheel\"\n");

    const QString block = QString("\n  users.users.%1 = {\n"
                                  "    isNormalUser = true;\n"
                                  "    description = \"%2\";\n"
                                  "    home = \"/home/%1\";\n"
                                  "    shell = pkgs.bashInteractive;\n"
                                  "    extraGroups = [\n%3    ];\n"
                                  "  };\n")
                             .arg(name, description, groups);

    /* Insert before the closing brace of the module. */
    const int close = users.lastIndexOf('}');
    if (close < 0)
        return QString("Unexpected shape of %1").arg(usersPath);
    users.insert(close, block);

    QString error = writeAll(usersPath, users);
    if (!error.isEmpty())
        return error;

    /* home-manager: give the new account the same home.nix as the others. */
    const QString flakePath = repoDir() + "/flake.nix";
    QString flake = readAll(flakePath);
    if (flake.isEmpty())
        return QString("Could not read %1").arg(flakePath);

    static const QRegularExpression entry(R"(( *)users\.([A-Za-z0-9_-]+) = import \./home\.nix;)");
    QRegularExpressionMatch last;
    QRegularExpressionMatchIterator it = entry.globalMatch(flake);
    while (it.hasNext())
        last = it.next();
    if (!last.hasMatch())
        return QString("No home-manager users entry in %1").arg(flakePath);
    if (flake.contains(QString("users.%1 = ").arg(name)))
        return QString();

    /* home.nix pins home.username and home.homeDirectory to the first user, so
     * a second one has to override them — otherwise the two definitions
     * collide and nothing evaluates. */
    const QString indent = last.captured(1);
    flake.insert(last.capturedEnd(),
                 QString("\n%1users.%2 = { lib, ... }: {\n"
                         "%1  imports = [ ./home.nix ];\n"
                         "%1  home.username = lib.mkForce \"%2\";\n"
                         "%1  home.homeDirectory = lib.mkForce \"/home/%2\";\n"
                         "%1};")
                     .arg(indent, name));

    error = writeAll(flakePath, flake);
    if (!error.isEmpty())
        return error;
    return stageFile(usersPath);
}
