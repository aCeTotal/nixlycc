#include "pages.h"
#include "apps/iconfarm.h"
#include "apps/icons.h"
#include "apps/password.h"
#include "apps/pkgindex.h"
#include "apps/pkglist.h"
#include "apps/rowdelegate.h"
#include "apps/unstable.h"
#include "git/rebuild.h"
#include "git/repo.h"
#include "git/style.h"
#include "git/task.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPointer>
#include <QPushButton>
#include <QShowEvent>
#include <QVBoxLayout>

namespace {

const int kResultLimit = 300;

const char *kTabStyle =
    "QPushButton { background-color: rgba(255, 255, 255, 12);"
    " border: 1px solid rgba(255, 255, 255, 30); border-radius: 10px;"
    " padding: 9px 26px; color: #8b8f9a; font-size: 14px; }"
    "QPushButton:hover { background-color: rgba(255, 255, 255, 24); }"
    "QPushButton:checked { background-color: rgba(122, 162, 247, 60);"
    " border-color: rgba(122, 162, 247, 140); color: #f0f0f2; }";

const char *kApplyStyle =
    "QPushButton { background-color: rgba(122, 162, 247, 70);"
    " border: 1px solid rgba(122, 162, 247, 150); border-radius: 10px;"
    " padding: 10px 28px; color: #f0f0f2; font-size: 14px; font-weight: bold; }"
    "QPushButton:hover { background-color: rgba(122, 162, 247, 110); }"
    "QPushButton:disabled { color: #8b8f9a; border-color: rgba(255, 255, 255, 30);"
    " background-color: rgba(255, 255, 255, 12); }";

int indexOfAttr(const QList<PkgEntry> &entries, const QString &attr)
{
    for (int i = 0; i < entries.size(); ++i) {
        if (entries[i].attr == attr)
            return i;
    }
    return -1;
}

/* Order-independent comparison — the file is written sorted, the pending list
 * is not. */
bool sameSet(QList<PkgEntry> a, QList<PkgEntry> b)
{
    auto byAttr = [](const PkgEntry &x, const PkgEntry &y) { return x.attr < y.attr; };
    std::sort(a.begin(), a.end(), byAttr);
    std::sort(b.begin(), b.end(), byAttr);
    if (a.size() != b.size())
        return false;
    for (int i = 0; i < a.size(); ++i) {
        if (a[i].attr != b[i].attr || a[i].unstable != b[i].unstable)
            return false;
    }
    return true;
}

/* The page owns the two indexes, so their async callbacks stay valid: the
 * stack keeps every page alive for the lifetime of the window.
 *
 * Ticking a row only changes the pending list. Apply is what writes
 * packages.nix, rebuilds the system from it, and commits the result. */
class ApplicationsPage : public QWidget {
public:
    ApplicationsPage();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void buildTabs(QVBoxLayout *layout);
    void buildApplyBar(QVBoxLayout *layout);
    void startIconFarm();

    void setMode(bool installedView);
    void refresh();
    void showSearch();
    void showInstalled();
    void addRow(const QString &attr, const Pkg *pkg);
    void addVersionPickers(const QHash<QString, QString> &unstable);
    void toggle(QListWidgetItem *item);
    void setChannel(const QString &attr, bool unstable);
    void apply();
    void updateApplyBar();

    PkgIndex m_index;
    IconIndex m_icons;
    QPushButton *m_searchTab;
    QPushButton *m_installedTab;
    QLineEdit *m_search;
    QLabel *m_status;
    QListWidget *m_list;
    QWidget *m_applyBar;
    QLabel *m_applyStatus;
    QPushButton *m_applyButton;

    QList<PkgEntry> m_current; /* what packages.nix holds */
    QList<PkgEntry> m_pending; /* what the list shows — written on Apply */
    bool m_started = false;
    bool m_applying = false;
};

ApplicationsPage::ApplicationsPage()
{
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(10);

    auto *title = new QLabel("Applications");
    title->setStyleSheet("color: white; font-size: 24px; font-weight: bold; margin-bottom: 10px;");
    layout->addWidget(title);

    buildTabs(layout);

    m_search = new QLineEdit;
    m_search->setPlaceholderText("Search nixpkgs and nixlypkgs…");
    m_search->setClearButtonEnabled(true);
    m_search->setMinimumHeight(54);
    m_search->setStyleSheet(
        "QLineEdit { background-color: rgba(255, 255, 255, 16);"
        " border: 1px solid rgba(255, 255, 255, 30); border-radius: 12px;"
        " padding: 0 16px; color: #f0f0f2; font-size: 17px; }"
        "QLineEdit:focus { border: 1px solid #7aa2f7; }");
    layout->addWidget(m_search);

    m_status = new QLabel;
    m_status->setStyleSheet("color: #8b8f9a; font-size: 12px;");
    layout->addWidget(m_status);

    m_list = new QListWidget;
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setMouseTracking(true);
    m_list->setUniformItemSizes(true);
    m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_list->setItemDelegate(new PkgRowDelegate(&m_icons, m_list));
    m_list->setStyleSheet("QListWidget { background: transparent; border: none; }");
    m_list->viewport()->setAutoFillBackground(false);
    layout->addWidget(m_list, 1);

    buildApplyBar(layout);

    m_current = installedPackages();
    m_pending = m_current;

    connect(m_search, &QLineEdit::textChanged, m_list, [this]() { showSearch(); });
    connect(m_list, &QListWidget::itemClicked, this, &ApplicationsPage::toggle);
}

void ApplicationsPage::buildTabs(QVBoxLayout *layout)
{
    m_searchTab = new QPushButton("Search");
    m_installedTab = new QPushButton("Installed");
    for (QPushButton *tab : { m_searchTab, m_installedTab }) {
        tab->setCheckable(true);
        tab->setCursor(Qt::PointingHandCursor);
        tab->setStyleSheet(kTabStyle);
    }
    m_searchTab->setChecked(true);

    auto *row = new QHBoxLayout;
    row->setSpacing(10);
    row->addWidget(m_searchTab);
    row->addWidget(m_installedTab);
    row->addStretch();
    layout->addLayout(row);

    connect(m_searchTab, &QPushButton::clicked, this, [this]() { setMode(false); });
    connect(m_installedTab, &QPushButton::clicked, this, [this]() { setMode(true); });
}

void ApplicationsPage::buildApplyBar(QVBoxLayout *layout)
{
    m_applyBar = new QWidget;
    auto *row = new QHBoxLayout(m_applyBar);
    row->setContentsMargins(0, 4, 0, 0);
    row->setSpacing(12);

    m_applyStatus = new QLabel;
    m_applyStatus->setStyleSheet("color: #f0f0f2; font-size: 13px;");
    m_applyStatus->setWordWrap(true);

    m_applyButton = new QPushButton("Apply");
    m_applyButton->setCursor(Qt::PointingHandCursor);
    m_applyButton->setStyleSheet(kApplyStyle);

    row->addWidget(m_applyStatus, 1);
    row->addWidget(m_applyButton);
    layout->addWidget(m_applyBar);
    m_applyBar->hide();

    connect(m_applyButton, &QPushButton::clicked, this, &ApplicationsPage::apply);
}

void ApplicationsPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (m_started)
        return;
    m_started = true;

    m_index.load([this](const QString &text) { m_status->setText(text); },
                 [this]() {
                     refresh();
                     startIconFarm();
                 });
}

/* Leaving the page pushes whatever is committed-worthy in ~/.nixlyos, so the
 * repository never trails behind what was applied here. */
void ApplicationsPage::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    if (m_applying)
        return;
    runAsync([]() { return commitAndPush("Changes Applications"); },
             [](const QString &) {});
}

/* The farm needs the package names, so it starts once the index is up. Rows
 * repaint with real icons when it finishes; until then they show the default
 * executable icon. */
void ApplicationsPage::startIconFarm()
{
    IconFarm::run(m_index.iconKeys(), m_index.stamp(),
                  [this](const QString &text) { m_status->setText(text); },
                  [this]() {
                      m_icons.reset();
                      m_list->viewport()->update();
                  });
}

void ApplicationsPage::setMode(bool installedView)
{
    m_searchTab->setChecked(!installedView);
    m_installedTab->setChecked(installedView);
    m_search->setVisible(!installedView);
    refresh();
}

void ApplicationsPage::refresh()
{
    if (m_installedTab->isChecked())
        showInstalled();
    else
        showSearch();
}

void ApplicationsPage::showSearch()
{
    if (m_installedTab->isChecked())
        return;

    const QString query = m_search->text();
    m_list->clear();

    if (!m_index.ready() || query.trimmed().isEmpty())
        return;

    const std::vector<const Pkg *> hits = m_index.search(query, kResultLimit);
    for (const Pkg *p : hits)
        addRow(p->attr, p);

    if (hits.empty())
        m_status->setText(QString("No packages match \"%1\"").arg(query));
    else
        m_status->setText(QString("%1%2 of %3 packages")
                              .arg(hits.size())
                              .arg(hits.size() == kResultLimit ? "+" : "")
                              .arg(m_index.count()));
}

void ApplicationsPage::showInstalled()
{
    m_list->clear();

    QList<PkgEntry> entries = m_pending;
    std::sort(entries.begin(), entries.end(),
              [](const PkgEntry &a, const PkgEntry &b) { return a.attr < b.attr; });

    QStringList attrs;
    for (const PkgEntry &e : entries) {
        addRow(e.attr, m_index.ready() ? m_index.find(e.attr) : nullptr);
        attrs << e.attr;
    }
    m_status->setText(QString("%1 packages in %2").arg(attrs.size()).arg(packagesPath()));

    if (attrs.isEmpty())
        return;

    QPointer<ApplicationsPage> self(this);
    unstableVersions(attrs, [self](const QHash<QString, QString> &versions) {
        if (self && self->m_installedTab->isChecked())
            self->addVersionPickers(versions);
    });
}

/* A package gets a dropdown only when unstable has a different version;
 * otherwise the row just states the stable one. */
void ApplicationsPage::addVersionPickers(const QHash<QString, QString> &unstable)
{
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem *item = m_list->item(i);
        const QString attr = item->text();
        const Pkg *pkg = m_index.ready() ? m_index.find(attr) : nullptr;
        const QString stableVer = pkg ? pkg->version : QString();
        const QString unstableVer = unstable.value(attr);

        if (unstableVer.isEmpty() || unstableVer == stableVer) {
            item->setData(PkgRowDelegate::VersionRole,
                          stableVer.isEmpty() ? QString() : stableVer + " (Stable)");
            continue;
        }

        /* The holder passes clicks through to the row, so ticking still works
         * everywhere except on the dropdown itself. */
        auto *holder = new QWidget;
        holder->setAttribute(Qt::WA_TransparentForMouseEvents);
        auto *row = new QHBoxLayout(holder);
        row->setContentsMargins(0, 0, 46, 0);
        row->addStretch();

        QComboBox *combo = makeCombo({ stableVer + " (Stable)", unstableVer + " (Unstable)" });
        combo->setMinimumHeight(0);
        combo->setFixedHeight(30);
        combo->setFixedWidth(230);
        combo->setStyleSheet(combo->styleSheet() + "QComboBox { font-size: 12px; padding: 0 8px; }");
        const int idx = indexOfAttr(m_pending, attr);
        combo->setCurrentIndex(idx >= 0 && m_pending[idx].unstable ? 1 : 0);
        row->addWidget(combo);

        item->setData(PkgRowDelegate::VersionRole, QString());
        m_list->setItemWidget(item, holder);
        connect(combo, &QComboBox::currentIndexChanged, this,
                [this, attr](int index) { setChannel(attr, index == 1); });
    }
}

void ApplicationsPage::addRow(const QString &attr, const Pkg *pkg)
{
    auto *item = new QListWidgetItem(attr, m_list);
    item->setData(PkgRowDelegate::DescRole, pkg ? pkg->desc : QString());
    item->setData(PkgRowDelegate::VersionRole, pkg ? pkg->version : QString());
    item->setData(PkgRowDelegate::PnameRole, pkg ? pkg->pname : attr);
    item->setData(PkgRowDelegate::NixlyRole, pkg && pkg->nixly);
    item->setData(PkgRowDelegate::InstalledRole, indexOfAttr(m_pending, attr) >= 0);
}

void ApplicationsPage::toggle(QListWidgetItem *item)
{
    const QString attr = item->text();
    const int idx = indexOfAttr(m_pending, attr);
    if (idx >= 0)
        m_pending.removeAt(idx);
    else
        m_pending.append({ attr, false });

    item->setData(PkgRowDelegate::InstalledRole, idx < 0);
    updateApplyBar();
}

void ApplicationsPage::setChannel(const QString &attr, bool unstable)
{
    const int idx = indexOfAttr(m_pending, attr);
    if (idx < 0 || m_pending[idx].unstable == unstable)
        return;
    m_pending[idx].unstable = unstable;
    updateApplyBar();
}

void ApplicationsPage::updateApplyBar()
{
    if (sameSet(m_pending, m_current)) {
        m_applyBar->hide();
        return;
    }

    int added = 0, removed = 0, moved = 0;
    for (const PkgEntry &e : m_pending) {
        const int idx = indexOfAttr(m_current, e.attr);
        if (idx < 0)
            ++added;
        else if (m_current[idx].unstable != e.unstable)
            ++moved;
    }
    for (const PkgEntry &e : m_current)
        removed += indexOfAttr(m_pending, e.attr) < 0 ? 1 : 0;

    m_applyStatus->setText(QString("%1 to install, %2 to remove, %3 to switch channel.")
                               .arg(added)
                               .arg(removed)
                               .arg(moved));
    m_applyBar->show();
}

/* packages.nix is written first, then the system is switched onto it — only
 * the derivations that actually changed get built — and the result is
 * committed and pushed. */
void ApplicationsPage::apply()
{
    QByteArray password = askPassword(this);
    if (password.isEmpty())
        return;

    const QString error = writeInstalledPackages(m_pending);
    if (!error.isEmpty()) {
        m_applyStatus->setText(error);
        return;
    }

    m_applying = true;
    m_applyButton->setEnabled(false);
    m_applyStatus->setText("Rebuilding…");

    QPointer<ApplicationsPage> self(this);
    const QList<PkgEntry> target = m_pending;
    runAsync(
        [password, self]() {
            const QString error = rebuildSwitch(password, [self](const QString &line) {
                postToGui([self, line]() {
                    if (self)
                        self->m_applyStatus->setText(line);
                });
            });
            if (!error.isEmpty())
                return error;
            return commitAndPush("Changes Applications");
        },
        [self, target](const QString &error) {
            if (!self)
                return;
            self->m_applying = false;
            self->m_applyButton->setEnabled(true);
            if (!error.isEmpty()) {
                self->m_applyStatus->setText(error);
                return;
            }
            self->m_current = target;
            self->updateApplyBar();
            self->m_status->setText("Applied — installed, committed and pushed.");
        });
    password.fill('\0');
}

} // namespace

QWidget *createApplicationsPage()
{
    return new ApplicationsPage;
}
