#-------------------------------------------------
#
# Project created by QtCreator 2016-06-19T01:35:57
#
#-------------------------------------------------

QT       += core gui

QMAKE_CXXFLAGS += -std=c++11 -Wall -mtune=native -mfpmath=sse

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG(debug)
{
   DEFINES += _DEBUG
}

include(./libusb/libusb.pri)
include(./devicing/devicing.pri)
include (./ui/ui.pri)

TARGET = qtcampp
TEMPLATE = app


SOURCES += main.cpp

RESOURCES += \
    resources.qrc
