#-------------------------------------------------
#
# Project created by QtCreator 2015-01-19T09:26:40
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = vchar64
target.path = $${PREFIX}/bin
INSTALLS += target
win32 {
    DESTDIR = ..
} else {
    DESTDIR = ../bin
}
TEMPLATE = app
VERSION = 0.0.13
GIT_VERSION = $$system(git describe --abbrev=4 --dirty --always --tags)
DEFINES += GIT_VERSION=\\\"$$GIT_VERSION\\\"

CONFIG += c++11
CONFIG += debug_and_release


SOURCES += main.cpp\
        mainwindow.cpp \
    state.cpp \
    aboutdialog.cpp \
    stateimport.cpp \
    stateexport.cpp \
    exportdialog.cpp \
    tilepropertiesdialog.cpp \
    commands.cpp \
    palette.cpp \
    bigcharwidget.cpp \
    charsetwidget.cpp \
    colorrectwidget.cpp \
    palettewidget.cpp \
    xlinkpreview.cpp \
    tilesetwidget.cpp \
    vchar64application.cpp \
    importvicedialog.cpp \
    fileutils.cpp \
    serverpreview.cpp \
    serverconnectdialog.cpp \
    importkoaladialog.cpp \
    selectcolordialog.cpp \
    mapwidget.cpp \
    utils.cpp \
    mappropertiesdialog.cpp \
    importkoalacharsetwidget.cpp \
    importkoalabitmapwidget.cpp \
    importvicecharsetwidget.cpp

HEADERS  += mainwindow.h \
    state.h \
    aboutdialog.h \
    stateimport.h \
    stateexport.h \
    exportdialog.h \
    tilepropertiesdialog.h \
    commands.h \
    palette.h \
    bigcharwidget.h \
    charsetwidget.h \
    colorrectwidget.h \
    palettewidget.h \
    xlinkpreview.h \
    tilesetwidget.h \
    vchar64application.h \
    importvicedialog.h \
    fileutils.h \
    serverpreview.h \
    serverconnectdialog.h \
    serverprotocol.h \
    importkoaladialog.h \
    selectcolordialog.h \
    mapwidget.h \
    utils.h \
    mappropertiesdialog.h \
    importkoalacharsetwidget.h \
    importkoalabitmapwidget.h \
    importvicecharsetwidget.h

FORMS    += mainwindow.ui \
    aboutdialog.ui \
    exportdialog.ui \
    tilepropertiesdialog.ui \
    importvicedialog.ui \
    serverconnectdialog.ui \
    importkoaladialog.ui \
    selectcolordialog.ui \
    mappropertiesdialog.ui

INCLUDEPATH += src

DISTFILES += \
    res/vchar64-icon-mac.icns

RESOURCES += \
    resources.qrc

!win32 {
    QMAKE_CXXFLAGS += -Werror
}

win32 {
    RC_FILE = res/vchar64.rc
}
macx {
    TARGET = VChar64
    ICON = res/vchar64-icon-mac.icns
    QMAKE_INFO_PLIST = res/Info.plist
}
