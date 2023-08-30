#include "mainwindow.h"

#include <QApplication>
#include <QSplashScreen>
#include <QTextCodec>
#include <QtDebug>
#include <QDir>
#include <QPixmap>
#include <QWindow>
#include <windows.h>
#include <DbgHelp.h>

QString str;

long  __stdcall CrashInfocallback(_EXCEPTION_POINTERS *pexcp)
{
    QString appName = QCoreApplication::applicationDirPath();
    appName.append("/dmpDir");
    QDir dir(appName);
    if(!dir.exists()){
        dir.mkpath(appName);
    }
    appName.append("/")
            .append(QCoreApplication::applicationName())
            .append(".")
            .append(QString::number(QDateTime::currentMSecsSinceEpoch()))
            .append("_CRASH_DUMP.DMP");
str=appName;
    //create Dump file
    HANDLE hDumpFile = ::CreateFile(
        (LPCWSTR)appName.utf16(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hDumpFile != INVALID_HANDLE_VALUE)
    {
        //Dump info
        MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
        dumpInfo.ExceptionPointers = pexcp;
        dumpInfo.ThreadId = GetCurrentThreadId();
        dumpInfo.ClientPointers = TRUE;
        //write Dump
        ::MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            hDumpFile,
            MiniDumpNormal,
            &dumpInfo,
            NULL,
            NULL
        );
    }
    return 0;
}

int main(int argc, char *argv[])
{
    ::SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)CrashInfocallback);

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
