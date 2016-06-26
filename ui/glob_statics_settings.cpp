//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com

#include "globalsettings.h"

#define DECL_SETT(CLASS,KEY,DEF,args...) {KEY, widgetted_pt(new CLASS(KEY,DEF,args))}
//-----------------------------------------------------------------------------------------------------------------------
//declare all global settings below, so it can be automatically added to setting's
//-----------------------------------------------------------------------------------------------------------------------

StaticSettingsMap &StaticSettingsMap::getGlobalSetts()
{
    QString defRel;

    if (defRel.isEmpty())
      defRel = QDir::homePath();

    //do not change keys once used, because 1: key-string is  directly used in other code (must be same), 2: users will lose stored value on next run
    //visual order depends on string sort of the keys
    static StaticSettingsMap list({

                                   });
    return list;
}
