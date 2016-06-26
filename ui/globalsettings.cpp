#include "globalsettings.h"
#include <QSettings>

namespace nmsp_gs
{
    const QString GlobalStorage::defaultGroup = "GlobalValues";

    GlobalStorage::GlobalStorage()
    {

    }

    void GlobalStorage::save(const SaveableToStorage &value, const QString& group) const
    {
        QSettings s; //local copy so it will be thread safe
        s.beginGroup(group);
        s.setValue(value.key(), static_cast<QVariant>(value));
        s.endGroup();
        s.sync();
    }

    void GlobalStorage::save(const SavablePtr &value, const QString &group) const
    {
        if (value)
        {
            QSettings s; //local copy so it will be thread safe
            s.beginGroup(group);
            s.setValue(value->key(), static_cast<QVariant>(*value));
            s.endGroup();
            s.sync();
        }
    }

    void GlobalStorage::load(SaveableToStorage &value, const QString &group) const
    {
        QSettings s; //local copy so it will be thread safe
        s.beginGroup(group);
        value.setVariantValue(s.value(value.key(), value.getDefault()));
        s.endGroup();
    }

    void GlobalStorage::load(SavablePtr &value, const QString &group) const
    {
        if (value)
        {
            QSettings s; //local copy so it will be thread safe
            s.beginGroup(group);
            value->setVariantValue(s.value(value->key(), value->getDefault()));
            s.endGroup();
        }
    }

    GlobalStorage::~GlobalStorage()
    {
    }


    void SaveableToStorage::setGroup(const QString &value)
    {
        group = value;
        //fixme: not sure if we should call reload from here..but what if group changed before save...
    }

    SaveableToStorage::SaveableToStorage(const QString &key, const QVariant &def, const QString &group):
        key_m(key),
        default_m(def),
        group(group)
    {

    }

    void SaveableToStorage::flush()
    {
        GLOBAL_STORAGE.save(*this, group);
    }

    SaveableToStorage::~SaveableToStorage()
    {
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

