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
                                            3, 30),

                                   });
    return list;
}
