//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com

#include "v4l_deviceproperties.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QDebug>
#include <QTimer>

#define FORMAT_WIDGET_ID 0x80000001

const static auto setPol = [](QWidget *w)
{
    QSizePolicy pol = w->sizePolicy();
    pol.setHorizontalPolicy(QSizePolicy::MinimumExpanding);
    w->setSizePolicy(pol);
};

V4LDeviceProperties::V4LDeviceProperties(const v4l2device::device_info device, QWidget *parent):
    DevicePropPaneBase(parent),
    settings_group(device.devname.c_str())
{

    currDevice = device.open();
    if (currDevice)
    {
        auto layout = new QVBoxLayout();
        layout->setAlignment(Qt::AlignTop);
        auto lbl = new QLabel();
        lbl->setText(device.devname.c_str());
        lbl->setAlignment(Qt::AlignHCenter);
        layout->addWidget(lbl);

        presetLbl = new QLabel();
        presetLbl->setAlignment(Qt::AlignHCenter);
        layout->addWidget(presetLbl);
        setSubgroupImpl(0);

        setPol(lbl);
        setLayout(layout);
        listControls();
        listFormats();

        QSettings s;
        s.beginGroup("dev_prop_"+settings_group);
        auto lst = findChildren<QWidget*>("", Qt::FindDirectChildrenOnly);
        for (auto & i: lst)
        {
            auto ptr = qobject_cast<DnDWidget*>(i);
            if (ptr) //saving only movable widgets
            {
                int ind = s.value(i->windowTitle(), -1).toInt();
                if (ind > -1)
                {
                    layout->removeWidget(i);
                    layout->insertWidget(ind, i);
                }
            }
        }
        s.endGroup();
    }
}

V4LDeviceProperties::~V4LDeviceProperties()
{
    //widgets must be destroyed prior holded items or saveVertIndex(v) will give error


    QSettings s;
    s.beginGroup("dev_prop_"+settings_group);
    auto lst = findChildren<QWidget*>("", Qt::FindDirectChildrenOnly);
    for (auto & i: lst)
    {
        auto ptr = qobject_cast<DnDWidget*>(i);
        if (ptr) //saving only movable widgets
        {
            int ind = layout()->indexOf(i);
            s.setValue(i->windowTitle(), ind);
        }
    }
    s.endGroup();
}

QWidget* V4LDeviceProperties::connectGUI(const V4LDeviceProperties::widgetted_pt &ptr)
{
    QWidget* w = nullptr;
    if (ptr)
    {
        ptr->setNewGroup(settings_group); //assigning to widget main group as device name
        ptr->reload();
        w = ptr->createWidget();
        setPol(w);
        layout()->addWidget(w);
    }
    return w;
}

QStringList V4LDeviceProperties::humanReadableSettings() const
{
    QStringList res;
    for (const auto& v : holder)
    {
        auto p1 = dynamic_cast<UserHintHolderForSettings*>(v.second.get());
        if (p1)
            res.push_back(QString("%1:\t%2").arg(p1->getUserText()).arg(v.second->getAsVariant().value<QString>()));
        else
            res.push_back("Here should be some unknown value. dynamic_cast<UserHintHolderForSettings*> failed.");
    }
    return res;
}

void V4LDeviceProperties::controlValueChanged(const V4LDeviceProperties::widgetted_pt &p, const v4l2_query_ext_ctrl &c)
{
    //lambda in constructor grow too big, so moved part of it here
    bool fl = (c.type & V4L2_CTRL_FLAG_HAS_PAYLOAD);//todo: when string/matrix processing will be added - fix here too, until that - ignoring

    if(currDevice && p && !fl)
    {
        auto val = p->getAsVariant().toLongLong();
        if ((c.type & V4L2_CTRL_TYPE_MENU) || (c.type & V4L2_CTRL_TYPE_INTEGER_MENU))
        {
            GlobalComboBoxStorable* cb = dynamic_cast<GlobalComboBoxStorable*>(p.get());
            if (cb)
            {
                //now restoring device's index
                val = cb->getUserData().toLongLong();
            }
        }
        int r = currDevice->as<v4l2device>()->setControlValue(c, val);
#ifdef _DEBUG
        qDebug() <<"Value "<<val<<c.name<<c.id<<" set code: " << r;
#endif
        if (r < 0)
            tryReconnectOnErrorImpl();

        //if (isNeedUpdate(c)) // on my sample camera this flag is not set, so must do full update of controls
        updateControlsImpl();
    }
}

void V4LDeviceProperties::listControls()
{
    controls = currDevice->as<v4l2device>()->listControls();
    for (const auto& c : controls)
    {
        //that is only 1 place where I do read real values from device - defaults, anything else inside device is ignored
        //and replaced by values stored into settings on computer
        widgetted_pt ptr;
#ifdef _DEBUG
        qDebug() <<c.id<<c.name<<c.type<<", default:"<<c.default_value<<"min,max:"<<c.minimum<<c.maximum<<c.step;
#endif
        switch(c.type)
        {
            case V4L2_CTRL_TYPE_BOOLEAN:
                ptr.reset(new GlobalStorableBool(QString(c.name), static_cast<bool>(c.default_value), QString(c.name), ""));
                break;
            case V4L2_CTRL_TYPE_INTEGER:
            case V4L2_CTRL_TYPE_INTEGER64:
            {
                //QSpinBox has limits to be int, also it will be remaped by step...so repeating here as nice hint
                int s = static_cast<int>(c.step);
                auto hint = tr(" (%1 / %2, def: %3)")
                            .arg(static_cast<int>(c.minimum) / s)
                            .arg(static_cast<int>(c.maximum) / s)
                            .arg(static_cast<int>(c.default_value) / s);
                ptr.reset(new GlobalStorableInt(QString(c.name),
                                                static_cast<GlobalStorableInt::ValueType>(c.default_value),
                                                QString(c.name) + hint, "",
                                                static_cast<GlobalStorableInt::ValueType>(c.minimum),
                                                static_cast<GlobalStorableInt::ValueType>(c.maximum),
                                                static_cast<GlobalStorableInt::ValueType>(c.step)));
            };
                break;
            case V4L2_CTRL_TYPE_MENU:
            case V4L2_CTRL_TYPE_INTEGER_MENU:
            {
                //menus are meajured in own indexes, i.e. some maybe skipped like 1,4,8 are valids only, but we will have it stored as 0-1-2
                //so need to remap it...
                auto menu = currDevice->as<v4l2device>()->listMenuForControl(c);
                int def = 0;
                for (const auto &m : menu)
                {
                    if (c.default_value == m.menu.index)
                    {
                        break;
                    }
                    ++def;
                }
                ptr.reset(new GlobalComboBoxStorable(QString(c.name), def, QString(c.name), "",
                                                     [menu](QStringList& s, QVariantList& v){
                              for (auto& itm : menu)
                              {
                                  if (itm.isInteger)
                                  s << QString::number(itm.menu.value, 16);
                                  else
                                  s << QString(reinterpret_cast<const char*>(itm.menu.name));
                                  v << itm.menu.index; //...and hold remap as user data for laters
                              }
                          }
                          ));
            };
                break;
                //todo: add more control types, like bit-fields
            default:
                ptr.reset();
                break;
        }
        if (ptr)
        {
            holder[c.id] = ptr;
            widgetted_ptw wp = ptr;
            //that is what I need actually - I do not read actual current camera settings, I force to set my own stored in settings
            connect(ptr.get(), &ISaveableWidget::valueChanged, this, [this, c, wp]()
            {
                auto p = wp.lock();
                //still lambda is connected to the event, because it allows to have copy of things easy without dancing with class memebers
                //(like c value here)
                controlValueChanged(p, c);

            }, Qt::QueuedConnection);
            auto w = connectGUI(ptr);
            if (w)
                w->setEnabled(isEnabled(c));
        }
    }
}

void V4LDeviceProperties::listFormats()
{
    auto fmts = currDevice->as<v4l2device>()->listFormats();
    if (fmts.size())
    {
        widgetted_pt ptr;
        GlobalComboBoxStorable *tmp;
        ptr.reset(tmp = new GlobalComboBoxStorable("video_format", 0, tr("Input Video Format"), tr("Video Format(s) supported and used by camera."),
                                             [fmts](QStringList& s, QVariantList& v)
        {
                      for (const auto& f: fmts)
                      {
                          s << QString(reinterpret_cast<const char*>(f.description));
                          v << f.pixelformat;
                      }
                  }));

        holder[FORMAT_WIDGET_ID] = ptr;
        widgetted_ptw wp = ptr;

        currentPixelFormatSelected = static_cast<__u32>(0xFFFFFFFF & tmp->getUserData().toLongLong());
        connect(ptr.get(), &ISaveableWidget::valueChanged, this, [this, wp]()
        {
            auto p = wp.lock();
            //set new format selected to device
            GlobalComboBoxStorable* cb = dynamic_cast<GlobalComboBoxStorable*>(p.get());
            if (cb)
            {
                __u32 val = static_cast<__u32>(0xFFFFFFFF & cb->getUserData().toLongLong());
                currentPixelFormatSelected = val;
                //push to device
                if (currDevice)
                    currDevice->as<v4l2device>()->setSourcePixelFormat(val);
            }

        }, Qt::QueuedConnection);

        connectGUI(ptr);
    }
}

bool V4LDeviceProperties::isEnabled(const v4l2_query_ext_ctrl &c)
{
    return isEnabled(c.flags);
}

bool V4LDeviceProperties::isEnabled(__u32 flags)
{
    return !(flags & V4L2_CTRL_FLAG_INACTIVE) && !(flags & V4L2_CTRL_FLAG_READ_ONLY);
}

bool V4LDeviceProperties::isNeedUpdate(const v4l2_query_ext_ctrl &c)
{
    //qDebug() << c.flags;
    return c.flags & V4L2_CTRL_FLAG_UPDATE;
}


void V4LDeviceProperties::resetToDefaultsImpl()
{
    for (auto & v : holder)
    {
        v.second->setDefault();
    }
}

void V4LDeviceProperties::reapplyAllImpl()
{
    for (auto & v : holder)
    {
        emit v.second->valueChanged();
    }
}

void V4LDeviceProperties::tryReconnectOnErrorImpl()
{
    if (currDevice)
    {
        if (!currDevice->is_valid_yet())
        {
            currDevice->reopen();//trying to reopen device, maybe some bug/reboot
            if (currDevice->is_valid_yet())
            {
                //if device reopened need to apply all settings to it, not only this one last,
                //to avoid recursion - doing by timer
                QTimer::singleShot(100, this, [this]()
                {
                    reapplyAllImpl(); //not sure what if device is going to lose connection inside that loop, maybe it will trigger recursive reapplies, some will be repeated a lot
                    emit deviceRestored();

                });
            }
            else
            {
                emit deviceDisconnected();
            }
        }
    }
    else
    {
        emit deviceDisconnected();
    }

}

void V4LDeviceProperties::updateControlsImpl()
{
    for (auto & v : holder)
    {
        if (v.second->lastWidget)
        {
            if (currDevice)
            {
                auto flags = currDevice->as<v4l2device>()->queryControlFlags(v.first);
                if (v.second->lastWidget)
                    v.second->lastWidget->setEnabled(isEnabled(flags));
            }
            else
                v.second->lastWidget->setEnabled(false);
        }
    }
}

void V4LDeviceProperties::setSubgroupImpl(int id)
{

    if (presetLbl)
        presetLbl->setText(tr("Preset: %1").arg(id + 1));

    for (auto & v : holder)
        v.second->switchSubgroup(id);
}
