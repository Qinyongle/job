#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QThread>

#include <QSettings>



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

}

MainWindow::~MainWindow()
{
    delete ui;
}




void MainWindow::delay(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms,&loop,SLOT(quit()));
    loop.exec();
}




void MainWindow::on_CheckBtn_clicked()
{
    //this->hide();
      myTable=new table();
      myTable->show();

}


void MainWindow::on_AddBtn_clicked()
{

}

