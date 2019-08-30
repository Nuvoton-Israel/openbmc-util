###################################################################
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, see http://gnu.org/licenses/
#
#
#  Copyright (C) 2009, Justin Davis <tuxdavis@gmail.com>
#  Copyright (C) 2009-2017 ImageWriter developers
#                 https://sourceforge.net/projects/win32diskimager/
###################################################################
TEMPLATE = app
TARGET = ../../NBDServerWSWindows
DEPENDPATH += .
INCLUDEPATH += .


CONFIG += debug
CONFIG += static
DEFINES -= UNICODE
LIBS += -lwsock32
LIBS += -lIPHLPAPI
LIBS += -L../QtWebsocket/debug/ -lQtWebsocket

QT += widgets
QT += core
QT += network

INCLUDEPATH += ../QtWebsocket
DEPENDPATH += ../QtWebsocket

VERSION = 2.0
VERSTR = '\\"$${VERSION}\\"'
DEFINES += VER=\"$${VERSTR}\"
DEFINES += WINVER=0x0601
DEFINES += _WIN32_WINNT=0x0601
QMAKE_TARGET_PRODUCT = "NBD Server Websocket Windows"
QMAKE_TARGET_DESCRIPTION = "NBD Server for Windows to export USB Device"
QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2017-2019 Nuvoton CS20"
QT_INSTALL_BINS = c:\Qt\5.7\mingw53_32\bin
# Input
HEADERS +=\
    mainwindow.h\
    droppablelineedit.h \
    disk.h \
    elapsedtimer.h \
    globaldebug.h \
    NBDProtocol.h

FORMS += mainwindow.ui

SOURCES +=\
    main.cpp\
    mainwindow.cpp\
    droppablelineedit.cpp \
    disk.cpp \
    elapsedtimer.cpp \
    nbd-server-ws.cpp \
    websocket-client.cpp

RESOURCES += gui_icons.qrc translations.qrc

RC_FILE = NBDServerWSWindows.rc

LANGUAGES  = zh_TW


defineReplace(prependAll) {
 for(a,$$1):result += $$2$${a}$$3
 return($$result)
}

TRANSLATIONS = $$prependAll(LANGUAGES, $$PWD/lang/remotemediaclient_, .ts)

TRANSLATIONS_FILES =

qtPrepareTool(LRELEASE, lrelease)
for(tsfile, TRANSLATIONS) {
    qmfile = $$tsfile
    qmfile ~= s,.ts$,.qm,
    command = $$LRELEASE $$tsfile -qm $$qmfile
    system($$command)|error("Failed to run: $$command")
    TRANSLATIONS_FILES += $$qmfile
}


