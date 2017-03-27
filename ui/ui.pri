HEADERS += \
    $$PWD/globalsettings.h \
    $$PWD/singleton.h \
    $$PWD/variant_convert.h \
    $$PWD/mainwindow.h \
    $$PWD/saveable_widget.h \
    $$PWD/settingsdialog.h \
    $$PWD/ppm_p6_buffer.h \
    $$PWD/utils.h \
    $$PWD/dndwidget.h \
    $$PWD/deviceproppanebase.h \
    $$PWD/v4l_selectdevicedialog.h

SOURCES += \
    $$PWD/globalsettings.cpp \
    $$PWD/mainwindow.cpp \
    $$PWD/glob_statics_settings.cpp \
    $$PWD/settingsdialog.cpp \
    $$PWD/ppm_p6_buffer.cpp \
    $$PWD/dndwidget.cpp \
    $$PWD/deviceproppanebase.cpp \
    $$PWD/v4l_selectdevicedialog.cpp

FORMS += \
    $$PWD/mainwindow.ui \
    $$PWD/settingsdialog.ui \
    $$PWD/v4l_selectdevicedialog.ui

v4ldevices{
  HEADERS += \
    $$PWD/v4l_deviceproperties.h

  SOURCES += \
    $$PWD/v4l_deviceproperties.cpp
}

ptpdevices{
}
