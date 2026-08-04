#include "pages.h"
#include "apps/iconfarm.h"
#include "apps/icons.h"
#include "apps/pkgindex.h"
#include "apps/rowdelegate.h"

#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QShowEvent>
#include <QVBoxLayout>

namespace {

const int kResultLimit = 300;

/* The page owns the two indexes, so their async callbacks stay valid: the
 * stack keeps every page alive for the lifetime of the window. */
class ApplicationsPage : public QWidget {
public:
    ApplicationsPage();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void runSearch();
    void startIconFarm();

    PkgIndex m_index;
    IconIndex m_icons;
    QLineEdit *m_search;
    QLabel *m_status;
    QListWidget *m_list;
    bool m_started = false;
};

ApplicationsPage::ApplicationsPage()
{
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(10);

    auto *title = new QLabel("Applications");
    title->setStyleSheet("color: white; font-size: 24px; font-weight: bold; margin-bottom: 10px;");
    layout->addWidget(title);

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

    connect(m_search, &QLineEdit::textChanged, m_list, [this]() { runSearch(); });
}

void ApplicationsPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (m_started)
        return;
    m_started = true;

    m_status->setText("Preparing package index…");
    m_index.load([this](const QString &text) { m_status->setText(text); },
                 [this]() {
                     runSearch();
                     startIconFarm();
                 });
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

void ApplicationsPage::runSearch()
{
    const QString query = m_search->text();
    m_list->clear();

    if (!m_index.ready() || query.trimmed().isEmpty())
        return;

    const std::vector<const Pkg *> hits = m_index.search(query, kResultLimit);
    for (const Pkg *p : hits) {
        auto *item = new QListWidgetItem(p->attr, m_list);
        item->setData(PkgRowDelegate::DescRole, p->desc);
        item->setData(PkgRowDelegate::VersionRole, p->version);
        item->setData(PkgRowDelegate::PnameRole, p->pname);
        item->setData(PkgRowDelegate::NixlyRole, p->nixly);
    }

    if (hits.empty())
        m_status->setText(QString("No packages match \"%1\"").arg(query));
    else
        m_status->setText(QString("%1%2 of %3 packages")
                              .arg(hits.size())
                              .arg(hits.size() == kResultLimit ? "+" : "")
                              .arg(m_index.count()));
}

} // namespace

QWidget *createApplicationsPage()
{
    return new ApplicationsPage;
}
