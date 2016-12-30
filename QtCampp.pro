#-------------------------------------------------
#
# Project created by QtCreator 2016-06-19T01:35:57
#
#-------------------------------------------------

QT       += core gui

CONFIG += qtcampp    #enabling gui integration of subprojects
CONFIG += v4ldevices #comment out to disable usage of v4l


QMAKE_CXXFLAGS += -std=c++11 -Wall -Werror=strict-aliasing -fstrict-aliasing -Wctor-dtor-privacy -Werror=delete-non-virtual-dtor
QMAKE_CXXFLAGS += -fexceptions -Werror=return-type -Werror=overloaded-virtual

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG(debug)
{
   DEFINES += _DEBUG
}

INCLUDEPATH += $$PWD

include(./libusb/libusb.pri)
include(./devicing/devicing.pri)
include (./ui/ui.pri)
include (./video/video.pri)

#comment include below to disable opencv filter (all "tools" actually) usage
include (tools/tools.pri)

TARGET = qtcampp
TEMPLATE = app


SOURCES += main.cpp

RESOURCES += \
    resources.qrc

HEADERS += \
    utils.h
