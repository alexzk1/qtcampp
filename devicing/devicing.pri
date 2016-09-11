HEADERS += \
    $$PWD/v4l2device.h \
    $$PWD/auto_closable.h \
    $$PWD/frame_listener.h \
    $$PWD/guardeds.h

SOURCES += \
    $$PWD/v4l2device.cpp \
    $$PWD/frame_listener.cpp

LIBS += -lv4l2 -lv4lconvert

#enables integration to other parts of projects, do not define for standalone usage of the device files
DEFINES += QTCAMPP_PROJ
