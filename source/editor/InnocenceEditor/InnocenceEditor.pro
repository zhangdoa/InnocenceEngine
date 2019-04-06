#-------------------------------------------------
#
# Project created by QtCreator 2019-03-21T15:42:37
#
#-------------------------------------------------

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = InnocenceEditor
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++17

SOURCES += \
        main.cpp \
        mainwindow.cpp \
        DarkStyle.cpp \
    innoviewport.cpp \
    innoconsole.cpp \
    innodirectoryexplorer.cpp \
    innofileexplorer.cpp \
    innoworldexplorer.cpp \
    innorenderconfigurator.cpp


HEADERS += \
        mainwindow.h \
        DarkStyle.h \
    innoviewport.h \
    innoconsole.h \
    innodirectoryexplorer.h \
    innofileexplorer.h \
    innoworldexplorer.h \
    innorenderconfigurator.h

FORMS += \
        mainwindow.ui

RESOURCES += darkstyle.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

INCLUDEPATH += $$PWD/../../../source/external/include

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../build/lib_archive/release/ -lInnoSystem
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../build/lib_archive/debug/ -lInnoSystem
else:unix: LIBS += -L$$PWD/../../../build/lib_archive/ -lInnoSystem

INCLUDEPATH += $$PWD/../../../build/lib_archive/Debug
DEPENDPATH += $$PWD/../../../build/lib_archive/Debug

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../build/lib_archive/release/ -lInnoGame
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../build/lib_archive/debug/ -lInnoGame
else:unix: LIBS += -L$$PWD/../../../build/lib_archive/ -lInnoGame

INCLUDEPATH += $$PWD/../../../build/lib_archive/Debug
DEPENDPATH += $$PWD/../../../build/lib_archive/Debug
