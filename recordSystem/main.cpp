#include "mainwindow.h"

#include <QApplication>
#include <QSplashScreen>
#include <QTextCodec>
#include <QtDebug>
#include <QDir>
#include <QPixmap>
int main(int argc, char *argv[])
{
    //QTextCodec :: setCodecForTr( QTextCodec :: codecForName( "GB18030" ));
    QTextCodec::setCodecForLocale( QTextCodec :: codecForName( "GB18030" ));
    QApplication a(argc, argv);

    QString filename=QDir::cleanPath( "E:/Qyl_Program/recordSystem/images/splash.png");



     QPixmap pixmap(filename);


     QSplashScreen splash(pixmap);
     splash.show();
     a.processEvents();
    MainWindow w;
    w.show();
    splash.finish(&w);
    return a.exec();
}
