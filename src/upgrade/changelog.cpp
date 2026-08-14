#include "changelog.h"

#include <QDesktopServices>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QTextBrowser>
#include <QUrl>
#include <QVBoxLayout>

namespace {

QPushButton *makeDialogButton(const QString &text)
{
    auto *button = new QPushButton(text);
    button->setCursor(Qt::PointingHandCursor);
    button->setStyleSheet(
        "QPushButton { background-color: rgba(255, 255, 255, 14);"
        " border: 1px solid rgba(255, 255, 255, 30); border-radius: 8px;"
        " padding: 7px 16px; color: #f0f0f2; font-size: 13px; }"
        "QPushButton:hover { background-color: rgba(122, 162, 247, 70); }"
        "QPushButton:disabled { color: #8b8f9a; }");
    return button;
}

} // namespace

void showChangelog(QWidget *parent, const QString &attr, const QString &url)
{
    auto *dialog = new QDialog(parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(attr + " — changelog");
    dialog->resize(700, 560);
    dialog->setStyleSheet("QDialog { background-color: #16171d; }");

    auto *layout = new QVBoxLayout(dialog);
    layout->setSpacing(10);

    auto *title = new QLabel(attr);
    title->setStyleSheet("color: #f0f0f2; font-size: 17px; font-weight: bold;");
    layout->addWidget(title);

    auto *link = new QLabel(url);
    link->setStyleSheet("color: #8b8f9a; font-size: 12px;");
    link->setTextInteractionFlags(Qt::TextSelectableByMouse);
    link->setWordWrap(true);
    layout->addWidget(link);

    auto *view = new QTextBrowser;
    view->setOpenExternalLinks(true);
    view->setStyleSheet(
        "QTextBrowser { background-color: rgba(255, 255, 255, 10);"
        " border: 1px solid rgba(255, 255, 255, 30); border-radius: 10px;"
        " color: #f0f0f2; font-size: 13px; padding: 8px; }");
    layout->addWidget(view, 1);

    auto *openButton = makeDialogButton("Open in browser");
    openButton->setEnabled(!url.isEmpty());
    auto *closeButton = makeDialogButton("Close");
    auto *row = new QHBoxLayout;
    row->addStretch();
    row->addWidget(openButton);
    row->addWidget(closeButton);
    layout->addLayout(row);

    QObject::connect(openButton, &QPushButton::clicked, dialog,
                     [url]() { QDesktopServices::openUrl(QUrl(url)); });
    QObject::connect(closeButton, &QPushButton::clicked, dialog, &QDialog::close);

    if (url.isEmpty()) {
        view->setPlainText("This package does not publish a changelog link.");
    } else {
        view->setPlainText("Loading changelog…");
        auto *net = new QNetworkAccessManager(dialog);
        QNetworkReply *reply = net->get(QNetworkRequest(QUrl(url)));
        QObject::connect(reply, &QNetworkReply::finished, dialog, [view, reply]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                view->setPlainText("Could not load the changelog: " + reply->errorString()
                                   + "\n\nUse \"Open in browser\" instead.");
                return;
            }
            const QString type = reply->header(QNetworkRequest::ContentTypeHeader).toString();
            const QByteArray body = reply->readAll();
            if (type.contains("html", Qt::CaseInsensitive))
                view->setHtml(QString::fromUtf8(body));
            else
                view->setPlainText(QString::fromUtf8(body));
        });
    }

    dialog->show();
}
