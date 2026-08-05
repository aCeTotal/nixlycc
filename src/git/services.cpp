#include "services.h"

const QVector<GitService> &gitServices()
{
    static const QVector<GitService> services = {
        { "GitHub", "github", "github.com", "GitHub → Settings → SSH and GPG keys",
          "git@github.com:user/.nixlyos.git" },
        { "GitLab", "gitlab", "gitlab.com", "GitLab → Preferences → SSH Keys",
          "git@gitlab.com:user/.nixlyos.git" },
        { "Bitbucket", "bitbucket", "bitbucket.org", "Bitbucket → Personal settings → SSH keys",
          "git@bitbucket.org:user/.nixlyos.git" },
        { "Codeberg", "codeberg", "codeberg.org", "Codeberg → Settings → SSH / GPG Keys",
          "git@codeberg.org:user/.nixlyos.git" },
        { "SourceHut", "sourcehut", "git.sr.ht", "SourceHut → meta.sr.ht → Keys",
          "git@git.sr.ht:~user/.nixlyos" },
        { "Gitea", "gitea", "gitea.com", "Gitea → Settings → SSH / GPG Keys",
          "git@gitea.com:user/.nixlyos.git" },
        { "Azure DevOps", "azure", "ssh.dev.azure.com",
          "Azure DevOps → User settings → SSH public keys",
          "git@ssh.dev.azure.com:v3/org/project/.nixlyos" },
    };
    return services;
}

int serviceIndexForHost(const QString &host)
{
    const QVector<GitService> &services = gitServices();
    for (int i = 0; i < services.size(); ++i) {
        if (services[i].host.compare(host, Qt::CaseInsensitive) == 0)
            return i;
    }
    return 0;
}
