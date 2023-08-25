#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <common.h>
#include <table.h>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();




    void delay(int );

    QThread *tableThread=nullptr;
    table *myTable=nullptr;

private:
    Ui::MainWindow *ui;

private slots:
    void on_CheckBtn_clicked();
};
#endif // MAINWINDOW_H
