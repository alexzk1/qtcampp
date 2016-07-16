#include "globalsettings.h"
#include <QSettings>

namespace nmsp_gs
{
    const QString SaveableToStorage::defaultGroup = "GlobalValues";

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
        //i.e. implicit copy of the prev state used (prev. subgroup)

        QSettings s;
        openGroup(s);
        bool currentExists = s.contains(key());
        closeGroup(s);

        if (currentExists)
            flush(); //saving current, bcs atomics could be cached!!

        QVariant defValue = (currentExists)?static_cast<QVariant>(*this):getDefault();
        currSubgroup = id;
        load(s, defValue);
        flush();
    }

    void SaveableToStorage::load(QSettings &s, const QVariant &def)
    {
        openGroup(s);
        setVariantValue(s.value(key(), def));
        closeGroup(s);
    }

    void SaveableToStorage::reload()
    {
        QSettings s;
        auto d = getDefault();
        load(s, d);
    }

    void SaveableToStorage::flush()
    {
        QSettings s; //local copy so it will be thread safe
        openGroup(s);
        s.setValue(key(), static_cast<QVariant>(*this));
        closeGroup(s);
        s.sync();
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
    for (auto& p : sets)
    {
        auto ptr = dynamic_cast<UserHintHolderForSettings*>(p.second.get());
        if (ptr)
            ptr->setMovable(false);
    }
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

QString UserHintHolderForSettings::getUserText() const
{
    return userText;
}

void UserHintHolderForSettings::setMovable(bool value)
{
    movable = value;
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

