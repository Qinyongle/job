QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    audio.cpp \
    main.cpp \
    mainwindow.cpp \
    table.cpp

HEADERS += \
    audio.h \
    common.h \
    mainwindow.h \
    table.h

FORMS += \
    mainwindow.ui \
    table.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target



win32: {
    FFMPEG_HOME=D:\ffmpeg
    #���� ffmpeg ��ͷ�ļ�
    INCLUDEPATH += $$FFMPEG_HOME/include

    #���õ�����Ŀ¼һ�߳�������ҵ������
    # -L ��ָ��������Ŀ¼
    # -l ��ָ��Ҫ����� ������
    LIBS +=  -L$$FFMPEG_HOME/lib \
             -lavcodec \
             -lavdevice \
             -lavfilter \
            -lavformat \
            -lavutil \
            -lpostproc \
            -lswresample \
            -lswscale
}
