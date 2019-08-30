TEMPLATE = subdirs

SUBDIRS += QtWebsocket

win32{
message(WIN32)
SUBDIRS += NBDServerWSWindows
NBDServerWSWindows.depends = QtWebsocket
}

unix {
message(UNIX)
SUBDIRS += NBDServerWSLinux
NBDServerWSLinux.depends = QtWebsocket
}
