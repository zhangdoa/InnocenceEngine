#-------------------------------------------------
#
# Project created by QtCreator 2019-03-21T15:42:37
#
#-------------------------------------------------

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Editor
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
    cameracomponentpropertyeditor.cpp \
    directorylistviewer.cpp \
    directorytreeviewer.cpp \
    directoryviewer.cpp \
    menubar.cpp \
    lightcomponentpropertyeditor.cpp \
        main.cpp \
        mainwindow.cpp \
    viewport.cpp \
    console.cpp \
    worldexplorer.cpp \
    renderconfigurator.cpp \
    propertyeditor.cpp \
    materialcomponentpropertyeditor.cpp \
    transformcomponentpropertyeditor.cpp \
    adjustlabel.cpp \
    combolabeltext.cpp \
    modelcomponentpropertyeditor.cpp


HEADERS += \
    cameracomponentpropertyeditor.h \
    directorylistviewer.h \
    directorytreeviewer.h \
    directoryviewer.h \
    menubar.h \
    lightcomponentpropertyeditor.h \
        mainwindow.h \
    viewport.h \
    console.h \
    worldexplorer.h \
    renderconfigurator.h \
    propertyeditor.h \
    icomponentpropertyeditor.h \
    materialcomponentpropertyeditor.h \
    transformcomponentpropertyeditor.h \
    adjustlabel.h \
    combolabeltext.h \
    modelcomponentpropertyeditor.h

FORMS += \
        mainwindow.ui

RESOURCES += qdarkstyle/style.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

INCLUDEPATH += $$PWD/../../Source/External/Include

win32:CONFIG(release, debug|release):LIBS += -L$$PWD/../../Build/LibArchive/Release/ -lApplicationEntry
else:win32:CONFIG(debug, debug|release):LIBS += -L$$PWD/../../Build/LibArchive/Debug/ -lApplicationEntry
else:unix: LIBS += -L$$PWD/../../Build/LibArchive/ -lApplicationEntry

win32:CONFIG(release, debug|release):LIBS += -L$$PWD/../../Build/LibArchive/Release/ -lCore
else:win32:CONFIG(debug, debug|release):LIBS += -L$$PWD/../../Build/LibArchive/Debug/ -lCore
else:unix: LIBS += -L$$PWD/../../Build/LibArchive/ -lCore

win32:CONFIG(release, debug|release):LIBS += -L$$PWD/../../Build/LibArchive/Release/ -lEngine
else:win32:CONFIG(debug, debug|release):LIBS += -L$$PWD/../../Build/LibArchive/Debug/ -lEngine
else:unix: LIBS += -L$$PWD/../../Build/LibArchive/ -lEngine

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../Build/LibArchive/Release/ -lDefaultRenderingClient
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../Build/LibArchive/Debug/ -lDefaultRenderingClient
else:unix: LIBS += -L$$PWD/../../Build/LibArchive/ -lDefaultRenderingClient

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../Build/LibArchive/Release/ -lDefaultLogicClient
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../Build/LibArchive/Debug/ -lDefaultLogicClient
else:unix: LIBS += -L$$PWD/../../Build/LibArchive/ -lDefaultLogicClient
