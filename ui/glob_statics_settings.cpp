//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com

#include "globalsettings.h"
#include <linux/videodev2.h>

#define DECL_SETT(CLASS,KEY,DEF,args...) {KEY, widgetted_pt(new CLASS(KEY,DEF,args))}
//-----------------------------------------------------------------------------------------------------------------------
//declare all global settings below, so it can be automatically added to setting's
//-----------------------------------------------------------------------------------------------------------------------

const StaticSettingsMap &StaticSettingsMap::getGlobalSetts()
{
    //do not change keys once used, because 1: key-string is  directly used in other code (must be same), 2: users will lose stored value on next run
    //visual order depends on string sort of the keys
    const static StaticSettingsMap list({
                                            DECL_SETT(GlobalFileStorable, "WorkingFolder", QDir::homePath(), tr("Working Folder"),
                                            tr("Set a folder where captured images and videos will be stored."),
                                            tr("Select working folder")),
                                            DECL_SETT(GlobalStorableInt, "VideoBuffs", 10,
                                            tr("Amount of the memory buffers."),
                                            tr("Sets amount of memory buffers used. More is better for bigger resolutions but consumes RAM.\n"
                                            "This value is optional for the driver and may be adjusted as needed automatically.\n"
                                            "You must restart capture (or device) to take effect."),
                                            3, 240),
                                            DECL_SETT(GlobalStorableBool, "PereodicDeviceTestPresence", true, tr("Check Device Pereodically"),
                                            tr("If enabled will test last used device presense pereodically and restart capture if device reconnected.")),

                                            DECL_SETT(GlobalStorableInt, "PereodicDeviceTestSecs", 5,
                                            tr("Device presence test period (seconds)."),
                                            tr("It is used if \"Check Device Pereodically\" is enabled.\nLower (faster) values may lower FPS."),
                                            1, 60),

                                            DECL_SETT(GlobalStorableBool, "CustomYUYV", false, tr("Custom YUYV conversion."),
                                            tr("If enabled and camera uses YUVY format it will use custom conversion instead of v4lutils lib,\n"
                                            "which shoud avoid applying auto-gain etc.\nMay be helpfull to picture bright objects on dark back like planets over telescope.")),

                                            //such a tricky "Wp_" key will place it visually after "WorkingFolder"
                                            DECL_SETT(GlobalComboBoxStorable, "Wp0SingleShotFormat", 0, tr("Single Short Saving Format"),
                                            tr("Sets format used to output single captured frame."),[](QStringList& s, QVariantList& v)
                                            {
                                                Q_UNUSED(v);
                                                s = QStringList{
                                                    "PNG compressed",
                                                    "Portatable Pixmap (PPM)",
                                                    "JPEG Compressed",
                                                }; //related is MainWindow::saveSnapshoot
                                            }),
                                            DECL_SETT(GlobalStorableInt, "Wp0SeriesLen", 30, tr("Amount of shoots in series."),
                                            tr("Defines length of sequentally stored frames on single button press.\n"
                                            "On fastest CPUs/SSDs it is limited by FPS (i.e. 30FPS will give 30 different shoots at most).\n"
                                            "On slower CPUs/HDDs you may need to increase buffers' amount."), 2, 3000),
                                        });
    return list;
}
