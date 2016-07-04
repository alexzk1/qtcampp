#include "globalsettings.h"
#include <QSettings>

namespace nmsp_gs
{
    const QString SaveableToStorage::defaultGroup = "GlobalValues";
    //more changes I do - less sence has this "GlobalStorage"


    void SaveableToStorage::setGroup(const QString &value)
    {
        group = value;
        //fixme: not sure if we should call reload from here..but what if group changed before save...
    }

    void SaveableToStorage::openGroup(QSettings &s) const
    {
        s.beginGroup(group);
        s.beginGroup(QString("sub_%1").arg(currSubgroup));

    }

    void SaveableToStorage::closeGroup(QSettings &s) const
    {
        s.endGroup();
        s.endGroup();
    }

    void SaveableToStorage::switchSubgroup(int id)
    {
        //subgroups work as follows:
        //when it switched it tries to read value, if absent - uses current state as default which if absent defaults to default
        currSubgroup = id;
    }

    const QString &SaveableToStorage::key() const
    {
        return key_m;
    }

    QVariant SaveableToStorage::getDefault()
    {
        return default_m;
    }

    SaveableToStorage::SaveableToStorage(const QString &key, const QVariant &def, const QString &group):
        key_m(key),
        default_m(def),
        currSubgroup(0),
        group(group)
    {

    }

    void SaveableToStorage::load()
    {
        QSettings s; //local copy so it will be thread safe
        openGroup(s);
        setVariantValue(s.value(key(), getDefault()));
        closeGroup(s);
    }

    void SaveableToStorage::flush()
    {
        QSettings s; //local copy so it will be thread safe
        openGroup(s);
        s.setValue(key(), static_cast<QVariant>(*this));
        closeGroup(s);
        s.sync();
    }

    SaveableToStorage::~SaveableToStorage()
    {
    }

    nmsp_gs::SaveableToStorage::operator QVariant() const
    {
        return valueAsVariant();
    }
}
//-----------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------

StaticSettingsMap::StaticSettingsMap(const StaticSettingsMap::list_t &init):
    sets(init)
{

}
//-----------------------------------------------------------------------------------------------------------------------

QList<QWidget *> StaticSettingsMap::createWidgets() const
{
    QList<QWidget*> r;
    for (const auto& p : sets)
    {
        r.push_back(p.second->createWidget());
    }

    return r;
}

UserHintHolderForSettings::~UserHintHolderForSettings()
{

}

ISaveableWidget::~ISaveableWidget()
{

}

//allowing copy-paste from header (using same name for macro), needed to have 1 virtual table for all objects, otherwise each inst has own = bigger code
#define DECL_DESTRUCTOR(NAME) NAME::~NAME(){}

DECL_DESTRUCTOR(GlobalStorableBool)
DECL_DESTRUCTOR(GlobalFileStorable)
DECL_DESTRUCTOR(GlobalStorableInt);
DECL_DESTRUCTOR(GlobalComboBoxStorable);

