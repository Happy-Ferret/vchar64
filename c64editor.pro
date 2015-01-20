#-------------------------------------------------
#
# Project created by QtCreator 2015-01-19T09:26:40
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = c64editor
TEMPLATE = app

CONFIG += c++11

SOURCES += src/main.cpp\
        src/mainwindow.cpp \
    src/bigchar.cpp \
    src/charsetview.cpp \
    src/state.cpp

HEADERS  += src/mainwindow.h \
    src/bigchar.h \
    src/charsetview.h \
    src/state.h

FORMS    += src/mainwindow.ui

DISTFILES +=

RESOURCES += \
    res/resources.qrc
