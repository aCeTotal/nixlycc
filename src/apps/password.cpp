#include "password.h"
#include "../git/style.h"

#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

QByteArray askPassword(QWidget *parent)
{
    QDialog dialog(parent);
    dialog.setWindowTitle("Apply");
    dialog.setStyleSheet("QDialog { background-color: #16171d; }");

    auto *layout = new QVBoxLayout(&dialog);
    layout->setSpacing(12);

    auto *hint = new QLabel("Enter your user password to install the changes.");
    hint->setStyleSheet("color: #f0f0f2; font-size: 14px;");
    layout->addWidget(hint);

    QLineEdit *field = makeField("Password");
    field->setEchoMode(QLineEdit::Password);
    field->setMinimumWidth(300);
    layout->addWidget(field);

    QPushButton *cancel = makeButton("Cancel");
    QPushButton *ok = makeButton("Apply");
    auto *row = new QHBoxLayout;
    row->addStretch();
    row->addWidget(cancel);
    row->addWidget(ok);
    layout->addLayout(row);

    QObject::connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(ok, &QPushButton::clicked, &dialog, &QDialog::accept);
    QObject::connect(field, &QLineEdit::returnPressed, &dialog, &QDialog::accept);

    if (dialog.exec() != QDialog::Accepted)
        return QByteArray();
    return field->text().toUtf8();
}
