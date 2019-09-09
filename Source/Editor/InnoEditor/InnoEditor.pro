#-------------------------------------------------
#
# Project created by QtCreator 2019-03-21T15:42:37
#
#-------------------------------------------------

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = InnoEditor
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
    lightcomponentpropertyeditor.cpp \
        main.cpp \
        mainwindow.cpp \
    innoviewport.cpp \
    innoconsole.cpp \
    innodirectoryexplorer.cpp \
    innofileexplorer.cpp \
    innoworldexplorer.cpp \
    innorenderconfigurator.cpp \
    innopropertyeditor.cpp \
    materialdatacomponentpropertyeditor.cpp \
    transformcomponentpropertyeditor.cpp \
    adjustlabel.cpp \
    combolabeltext.cpp \
    visiblecomponentpropertyeditor.cpp


HEADERS += \
    lightcomponentpropertyeditor.h \
        mainwindow.h \
    innoviewport.h \
    innoconsole.h \
    innodirectoryexplorer.h \
    innofileexplorer.h \
    innoworldexplorer.h \
    innorenderconfigurator.h \
    innopropertyeditor.h \
    icomponentpropertyeditor.h \
    materialdatacomponentpropertyeditor.h \
    transformcomponentpropertyeditor.h \
    adjustlabel.h \
    combolabeltext.h \
    visiblecomponentpropertyeditor.h

FORMS += \
        mainwindow.ui

RESOURCES += qdarkstyle/style.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

INCLUDEPATH += $$PWD/../../../Source/External/Include

win32:CONFIG(release, debug|release):LIBS += -L$$PWD/../../../Build/Release/LibArchive/Release/ -lInnoApplicationEntry
else:win32:CONFIG(debug, debug|release):LIBS += -L$$PWD/../../../Build/Debug/LibArchive/Debug/ -lInnoApplicationEntry
else:unix: LIBS += -L$$PWD/../../../Build/LibArchive/ -lInnoApplicationEntry

win32:CONFIG(release, debug|release):LIBS += -L$$PWD/../../../Build/Release/LibArchive/Release/ -lInnoEngine
else:win32:CONFIG(debug, debug|release):LIBS += -L$$PWD/../../../Build/Debug/LibArchive/Debug/ -lInnoEngine
else:unix: LIBS += -L$$PWD/../../../Build/LibArchive/ -lInnoEngine

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../Build/Release/LibArchive/Release/ -lInnoDefaultRenderingClient
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../Build/Debug/LibArchive/Debug/ -lInnoDefaultRenderingClient
else:unix: LIBS += -L$$PWD/../../../Build/LibArchive/ -lInnoDefaultRenderingClient

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../Build/Release/LibArchive/Release/ -lInnoDefaultLogicClient
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../Build/Debug/LibArchive/Debug/ -lInnoDefaultLogicClient
else:unix: LIBS += -L$$PWD/../../../Build/LibArchive/ -lInnoDefaultLogicClient
