#-------------------------------------------------
#
# Project created by QtCreator 2018-12-04T10:41:59
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = NBDServerWSLinux
TEMPLATE = app


#NUvoton CS20--Start
CONFIG += debug
PRE_TARGETDEPS += ../QtWebsocket/libQtWebsocket.a
LIBS += -L../QtWebsocket/ -lQtWebsocket


INCLUDEPATH += ../QtWebsocket
DEPENDPATH += ../QtWebsocket
QT += widgets
QT += network

RESOURCES += gui_icons.qrc
#NUvoton CS20--End

SOURCES += main.cpp\
        mainwindow.cpp \
        nbd-server-ws.cpp \
        websocket-client.cpp

HEADERS  += mainwindow.h \
    nbdprotocol.h

FORMS    += mainwindow.ui
