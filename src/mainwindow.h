#pragma once
#include <QMainWindow>

class QListWidget;
class QStackedWidget;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(const QString &page = QString(), QWidget *parent = nullptr);
};
