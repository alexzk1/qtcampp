HEADERS += \
    $$PWD/auto_closable.h \
    $$PWD/guardeds.h \
    $$PWD/video_camera.h \
    $$PWD/frame_listener_base.h \
    $$PWD/runners.h \
    $$PWD/union_cast.h \
    $$PWD/no_const.h

SOURCES += \
    $$PWD/video_camera.cpp \
    $$PWD/frame_listener_base.cpp

qtcampp{
    DEFINES += QTCAMPP_PROJ #enables integration to other parts of projects, do not define for standalone usage of the device files
}

v4ldevices{
HEADERS += \
    $$PWD/frame_listener_v4l.h \
    $$PWD/v4l2device.h

SOURCES += \
   $$PWD/frame_listener_v4l.cpp \
   $$PWD/v4l2device.cpp

    LIBS    += -lv4l2 -lv4lconvert
    DEFINES += V4LDEVICES
}

ptpdevices{
    DEFINES += PTPDEVICES
}
