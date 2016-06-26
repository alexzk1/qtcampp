//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com

#include "globalsettings.h"

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

                                   });
    return list;
}
