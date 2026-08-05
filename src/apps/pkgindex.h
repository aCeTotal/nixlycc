#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <functional>
#include <vector>

struct Pkg {
    QString attr;      /* attribute path, e.g. "firefox" */
    QString pname;     /* name without version */
    QString version;
    QString desc;
    bool nixly = false;

    /* Lowercased copies kept alongside for matching without per-keystroke
     * allocation. */
    QByteArray attrLower;
    QByteArray descLower;
};

/* Searchable index over nixpkgs + nixlypkgs.
 *
 * Enumerating nixpkgs costs ~20 s of evaluation, so the result is cached on
 * disk and rebuilt only when the nixpkgs store path changes. Queries run as a
 * full scan of the in-memory array: ~71k entries is a few milliseconds, fast
 * enough to re-run on every keystroke. */
class PkgIndex {
public:
    /* Loads the cache, or spawns a background rebuild. onStatus reports
     * progress text, onReady fires once entries are searchable. */
    void load(std::function<void(const QString &)> onStatus,
              std::function<void()> onReady);

    bool ready() const { return m_ready; }
    int count() const { return (int)m_pkgs.size(); }
    std::vector<const Pkg *> search(const QString &query, int limit) const;

    /* The entry with this attribute path, or nullptr — used to put a
     * description on an installed package. */
    const Pkg *find(const QString &attr) const;

    /* The nixpkgs store path the entries came from — also IconFarm's refarm
     * key, since new packages arrive with a new nixpkgs. */
    QString stamp() const { return m_stamp; }

    /* Every lowercased name an icon could be filed under: attribute path,
     * pname, and the last component of a reverse-DNS attribute. */
    QStringList iconKeys() const;

private:
    void startBuild();
    void runNixlyEval();
    void finish();
    bool readCache();
    void writeCache() const;

    std::vector<Pkg> m_pkgs;
    QString m_nixpkgs;    /* the flake's nixpkgs input, or a channel path */
    QString m_stamp;      /* nixpkgs store path — the cache validity key */
    bool m_ready = false;
    std::function<void(const QString &)> m_onStatus;
    std::function<void()> m_onReady;
};
