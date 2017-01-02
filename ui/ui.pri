HEADERS += \
    $$PWD/globalsettings.h \
    $$PWD/singleton.h \
    $$PWD/variant_convert.h \
    $$PWD/mainwindow.h \
    $$PWD/saveable_widget.h \
    $$PWD/selectdevicedialog.h \
    $$PWD/settingsdialog.h \
    $$PWD/ppm_p6_buffer.h \
    $$PWD/utils.h \
    $$PWD/dndwidget.h \
    $$PWD/deviceproppanebase.h

SOURCES += \
    $$PWD/globalsettings.cpp \
    $$PWD/mainwindow.cpp \
    $$PWD/glob_statics_settings.cpp \
    $$PWD/selectdevicedialog.cpp \
    $$PWD/settingsdialog.cpp \
    $$PWD/ppm_p6_buffer.cpp \
    $$PWD/dndwidget.cpp \
    $$PWD/deviceproppanebase.cpp

FORMS += \
    $$PWD/mainwindow.ui \
    $$PWD/selectdevicedialog.ui \
    $$PWD/settingsdialog.ui

v4ldevices{
  HEADERS += \
    $$PWD/v4l_deviceproperties.h

  SOURCES += \
    $$PWD/v4l_deviceproperties.cpp
}
