#-------------------------------------------------
#
# Project created by QtCreator 2011-05-19T11:03:54
#
#-------------------------------------------------

QT       += core gui

TARGET = AlogTool
TEMPLATE = app


SOURCES += src/main.cc \
           src/mainwindow.cc \
           src/fileutils.cc

HEADERS  += include/mainwindow.h \
            include/fileutils.h

INCLUDEPATH += include

FORMS    += ui/mainwindow.ui

OTHER_FILES += \
    config/alog-tool.rc \
    resources/alog-tool.ico \
    resources/alog-tool.icns

RC_FILE = config/alog-tool.rc


ICON = resources/alog-tool.icns

Release:DESTDIR = release
Release:OBJECTS_DIR = release/.obj
Release:MOC_DIR = release/.moc
Release:RCC_DIR = release/.rcc
Release:UI_DIR = release/.ui

Debug:DESTDIR = debug
Debug:OBJECTS_DIR = debug/.obj
Debug:MOC_DIR = debug/.moc
Debug:RCC_DIR = debug/.rcc
Debug:UI_DIR = debug/.ui
