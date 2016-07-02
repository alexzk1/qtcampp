//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com

#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H

#include <memory>
#include <vector>
#include <map>
#include <type_traits>
#include <atomic>

#include <QString>
#include <QVariant>
#include <QPointer>
#include <QSettings>
#include <QVariant>
#include <QWidget>
#include <QList>
#include <QComboBox>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QDir>
#include <QFileDialog>
#include <QLabel>

#include "variant_convert.h"
#include "singleton.h"

namespace nmsp_gs
{
    class GlobalStorage;

    class SaveableToStorage
    {
        friend class GlobalStorage;
    private:
        const QString  key_m;
        const QVariant default_m;
    protected:
        QString group;

        virtual QVariant valueAsVariant() const = 0 ;
        virtual void     setVariantValue(const QVariant& value) = 0;


        const QString& key() const
        {
            return key_m;
        }

        virtual QVariant getDefault()
        {
            return default_m;
        }

        SaveableToStorage() = delete;
        SaveableToStorage(const SaveableToStorage&) = delete;
        explicit SaveableToStorage(const QString& key, const QVariant& def, const QString& group);

    public:

        void flush(); //forcing save
        virtual void reload() = 0;//because child is template, so it will have to have own reload
        virtual void setDefault() = 0;

        virtual ~SaveableToStorage();
        explicit operator QVariant() const
        {
            return valueAsVariant();
        }
        void setGroup(const QString &value);
    };

    using SavablePtr = std::shared_ptr<SaveableToStorage>;

    class GlobalStorage :protected ItCanBeOnlyOne<GlobalStorage>
    {
    public:
        static const QString defaultGroup;

        GlobalStorage();
        void save(const SaveableToStorage& value, const QString& group = defaultGroup) const;
        void save(const SavablePtr& value, const QString& group = defaultGroup) const;


        void load(SaveableToStorage& value, const QString& group = defaultGroup) const;
        void load(SavablePtr& value, const QString& group = defaultGroup) const;
        virtual ~GlobalStorage();
    };
}

#define GLOBAL_STORAGE globalInstanceConst<nmsp_gs::GlobalStorage>()

//templated savable object, you just set value and it kept in settings, automatically casts to template-type like bool
//this is supposed to be in-memory setting value which will be saved by the destructor
template <typename T, bool use_atomic = false>
class GlobSaveableTempl : public nmsp_gs::SaveableToStorage
{
private:
    using VarType = typename std::conditional<use_atomic, std::atomic<T>, T>::type;
    VarType value;
protected:
    virtual void setVariantValue(const QVariant& val) override
    {
        value = variantTo<T>(val);
    }

public:
    explicit GlobSaveableTempl(const QString& key, const QString& group = nmsp_gs::GlobalStorage::defaultGroup):
        nmsp_gs::SaveableToStorage(key, QVariant(), group)
    {
        reload();
    }

    explicit GlobSaveableTempl(const QString& key, const T& def, const QString& group = nmsp_gs::GlobalStorage::defaultGroup):
        nmsp_gs::SaveableToStorage(key, qVariantFromValue(def), group)
    {
        reload();
    }

    virtual ~GlobSaveableTempl()
    {
        flush();
    }

    virtual QVariant valueAsVariant() const override
    {
        return qVariantFromValue(getCachedValue());
    }

    const T getCachedValue() const
    {
        return static_cast<T>(value);
    }

    void setCachedValue(const T& v)
    {
        value = v;
    }

    explicit operator T() const
    {
        return getCachedValue();
    }

    GlobSaveableTempl<T, use_atomic>& operator = (const T& v)
    {
        setCachedValue(v);
        return *this;
    }
    virtual void reload() override
    {
        GLOBAL_STORAGE.load(*this, group);
    }

    virtual void setDefault() override
    {
        setVariantValue(getDefault());
    }
};

//kinda Interface, so pointer can be stored uniformely and subclasses will do what they need
class ISaveableWidget : public QObject
{
    Q_OBJECT
protected:
    virtual QWidget* createWidget2() = 0; //ownership must be not taken by subclass, GUI will get it
public:
    QPointer<QWidget> lastWidget;

    ISaveableWidget() = default;
    virtual ~ISaveableWidget();

    QWidget* createWidget()
    {
        return lastWidget = createWidget2();
    }

    virtual void exec(){} //maybe called by the program elsewhere, for example for folder selector it must pop folder selector dialog
    virtual void valueSet() = 0; //should update GUI, called when value was modified outside user interaction by direct setter
    virtual void setNewGroup(const QString& group) = 0; //sets new settings' group
    virtual void reload() = 0; //should be called if group changed (and nothing to save yet), because real loading is done by ctor before group change happened
    virtual void setDefault() = 0;
    virtual QVariant getAsVariant() = 0;
signals:
    void valueChanged();
};


//adds GUI to saveable item, templated interface version, so GUI works as, for example, bool variable and shows checkbox to change
template <typename T, bool use_atomic = true>
class SaveableWidgetTempl: public ISaveableWidget
{
protected:
    const bool IsAtomic = use_atomic;
    GlobSaveableTempl<T, use_atomic> state;
public:
    using      ValueType = T;

    SaveableWidgetTempl() = delete;
    SaveableWidgetTempl(const QString& key, T def, const QString& group = nmsp_gs::GlobalStorage::defaultGroup):
        state(key, def, group){}

    virtual void set(const T& s)
    {
        state = s;
        emit valueChanged();
        valueSet();
    }

    operator T() const
    {
        return getValue();
    }

    virtual QVariant getAsVariant() override final
    {
        return state.valueAsVariant();
    }

    virtual void setNewGroup(const QString& group)override
    {
        state.setGroup(group);
    }

    ValueType getValue() const
    {
        return static_cast<ValueType>(state);
    }

    virtual void reload() override
    {
        state.reload();
        emit valueChanged();
        valueSet();
    }

    virtual void setDefault() override
    {
        state.setDefault();
        emit valueChanged();
        valueSet();
    }
};

//list to make global static set of controls (like for global settings)
class StaticSettingsMap :public QObject
{
    Q_OBJECT
private:
    using widgetted_pt = std::shared_ptr<ISaveableWidget>;
    using list_t = std::map<QString, widgetted_pt>;
    using pair_t = list_t::value_type;
    list_t sets;
    StaticSettingsMap(const list_t& init);

public:
    QList<QWidget*> createWidgets() const;
    static const StaticSettingsMap& getGlobalSetts();

    template<class T, bool isatomic = true>
    void readValue(const QString& name, T& value) const
    {
        if (sets.count(name))
        {
            auto p = sets.at(name).get();
            if (p)
                value = *dynamic_cast<SaveableWidgetTempl<T, isatomic>*>(p);
        }
    }

    template<class T, bool isatomic = true>
    void storeValue(const QString& name, const T& value) const
    {
        if (sets.count(name))
        {
            auto p = sets.at(name).get();
            auto p2 = dynamic_cast<SaveableWidgetTempl<T, isatomic>*>(p);
            if (p2)
            {
                p2->set(value);
            }
            emit settingHaveBeenChanged(name);
        }
    }

    void exec(const QString& name) const
    {
        if (sets.count(name))
        {
            auto p = sets.at(name).get();
            if (p)
                p->exec();
        }
    }

    //shortcut to most used
    template<bool isatomic = true>
    bool readBool(const QString& name) const
    {
        bool r = false;
        readValue<decltype (r), isatomic>(name, r);
        return r;
    }

    template<bool isatomic = true>
    int readInt(const QString& name) const
    {
        int r = 0;
        readValue<decltype (r), isatomic>(name, r);
        return r;
    }

signals:
    void settingHaveBeenChanged(const QString& name) const;
};

//this class just a holder of user visible description and hint bound to controls
class UserHintHolderForSettings
{

protected:
    const QString userText;
    const QString userHint;
protected:
    UserHintHolderForSettings() = delete;
    UserHintHolderForSettings(const QString& userText, const QString& userHint = QString()):
        userText(userText),
        userHint(userHint)
    {}
    virtual ~UserHintHolderForSettings();



    void setHint(QWidget *w) const
    {
        w->setToolTip(userHint);
    }

    QWidget* createLabeledField(QWidget *field, int fieldStretch = 3, int labelStretch = 97, QWidget* lbl = nullptr)
    {
        auto hoster = new QWidget(nullptr);
        auto hlayout = new QHBoxLayout();
        if (!lbl)
            lbl = new QLabel(userText);

        hlayout->addWidget(field, fieldStretch);
        hlayout->addWidget(lbl,   labelStretch);

        setHint(lbl);
        setHint(field);

        hoster->setLayout(hlayout);

        return hoster;
    }
};


//----------Classes which keep persistent values in settings and automatically supply widgets for GUI ----------------------------
//say "NO!" to copy-paste ....
#define STORABLE_ATOMIC_CLASS(NAME, TYPE) class NAME: public SaveableWidgetTempl<TYPE, true>, virtual protected UserHintHolderForSettings
#define STORABLE_CLASS(NAME, TYPE) class NAME: public SaveableWidgetTempl<TYPE, false>, virtual protected UserHintHolderForSettings
#define STORABLE_CONSTRUCTOR(NAME) NAME(const QString& key, const ValueType& def, const QString& text, const QString& hint): \
    UserHintHolderForSettings(text, hint) ,SaveableWidgetTempl(key, def){} \
    NAME() = delete;

#define STORABLE_CONSTRUCTOR2(NAME) NAME() = delete; NAME(const QString& key, const ValueType& def, const QString& text, const QString& hint): \
    UserHintHolderForSettings(text, hint) ,SaveableWidgetTempl(key, def)

#define DECL_DESTRUCTOR(NAME) virtual ~NAME()

STORABLE_ATOMIC_CLASS(GlobalStorableBool, bool)
{
private:
    QPointer<QCheckBox> cb;

    virtual QWidget* createWidget2() override
    {
        cb = new QCheckBox(nullptr);
        cb->setChecked(getValue());
        cb->setText(userText);
        setHint(cb);

        QObject::connect(cb, &QCheckBox::toggled, this, [this](bool checked)
        {
            set(checked);
        }, Qt::QueuedConnection);

        return cb;
    }
public:
    virtual void valueSet() override
    {
        if (cb)
            cb->setChecked(getValue());
    }
    STORABLE_CONSTRUCTOR2(GlobalStorableBool)
    {
        cb = nullptr;
    }

    //need to put ANY virtual function to CPP file, so vtable will not be copied for each object
    DECL_DESTRUCTOR(GlobalStorableBool);
};

STORABLE_ATOMIC_CLASS(GlobalStorableInt, int)
{
private:
    QPointer<QSpinBox> field;
    const ValueType min;
    const ValueType max;
    const ValueType step;
    virtual QWidget* createWidget2() override
    {
        field = new QSpinBox();
        field->setRange(static_cast<int>(min / step), static_cast<int>(max / step)); // that is a problem - QSpinBox accepts ints only ... so others like int64 will fail
        field->setValue(static_cast<int>(getValue() / step)); //scaling visually by step, but...

        connect(field, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
                [this](int i)
        {
            set(i * step); //...storing real value of multiplies of the step
        }, Qt::QueuedConnection);
        return createLabeledField(field);
    }
public:
    virtual void valueSet() override
    {
        if (field)
            field->setValue(getValue() / step);
    }
    GlobalStorableInt() = delete;
    GlobalStorableInt(const QString& key, const ValueType& def, const QString& text, const QString& hint, ValueType min, ValueType max, ValueType step = 1):
        UserHintHolderForSettings(text, hint) ,SaveableWidgetTempl(key, def / step),
        min(min), max(max), step(step)
    {
        field = nullptr;
    }
    DECL_DESTRUCTOR(GlobalStorableInt);
};

STORABLE_CLASS(GlobalFileStorable, QString)
{
private:
    QPointer<QLineEdit> txt;
    const QString execText;

    virtual QWidget* createWidget2() override
    {
        txt = new QLineEdit();
        txt->setEnabled(false);
        auto btn = new QPushButton();
        btn->setText("...");
        txt->setText(getValue());
        auto f = createLabeledField(txt, 97, 3, btn);

        connect(btn, &QPushButton::clicked, this, [this]()
        {
            exec();
        }, Qt::QueuedConnection);


        return createLabeledField(f, 80, 20);
    }
public:
    virtual void valueSet() override
    {
        if (txt)
            txt->setText(getValue());
    }

    virtual void exec() override
    {
        QString dir = QFileDialog::getExistingDirectory(nullptr, execText, static_cast<QString>(state),
                                                        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        bool ret = !dir.isEmpty();
        if (ret)
        {
            set(dir);
            valueSet();
        }
    }

    //okey..copy-paste, because I need "execText" to be set elsewhere, not directly in class to be reusable
    GlobalFileStorable() = delete;
    GlobalFileStorable(const QString& key, const ValueType& def, const QString& text, const QString& hint, const QString& execText):
        UserHintHolderForSettings(text, hint),
        SaveableWidgetTempl(key, def),
        execText(execText)
    {
        txt = nullptr;
    }

    //need to put ANY virtual function to CPP file, so vtable will not be copied for each object
    DECL_DESTRUCTOR(GlobalFileStorable);
};


//will store item number selected in combobox, items itself are supplied by functor
//so when each time new widget is created functor may do some different list based on something
//trivial case it will return static list of choices
STORABLE_ATOMIC_CLASS(GlobalComboBoxStorable, int)
{
public:
    using items_supplier_t  = std::function<void (QStringList&, QVariantList &)>;
private:
    const items_supplier_t itemsf;
    QPointer<QComboBox> cb;

    int getStoredSelection() const
    {
        auto val = getValue();
        if (cb)
        {
            if (val >= cb->count())
                val = -1;
        }
        return val;
    }

protected:
    virtual QWidget* createWidget2() override
    {
        cb = new QComboBox();

        QStringList sl;
        QVariantList vl;
        itemsf(sl, vl);
        for (int i = 0, sz = sl.size(); i < sz; ++i)
        {
            cb->addItem(sl.at(i), (vl.size() > i)?vl.at(i):QVariant());
        }


        cb->setCurrentIndex(getStoredSelection());

        connect(cb, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
                [this](int index)
        {
            set(index);
        }, Qt::QueuedConnection);


        return createLabeledField(cb);
    }

    virtual void valueSet() override
    {
        if (cb)
            cb->setCurrentIndex(getStoredSelection());
    }
public:

    GlobalComboBoxStorable() = delete;
    GlobalComboBoxStorable(const QString& key, const ValueType& def, const QString& text, const QString& hint, const items_supplier_t& itemsf):
        UserHintHolderForSettings(text, hint),
        SaveableWidgetTempl(key, def),
        itemsf(itemsf)
    {
        cb = nullptr;
    }

    QVariant getUserData(int index) const
    {
       QVariant r;
       if (cb)
           r = cb->itemData(index, Qt::UserRole);
       return r;
    }
    DECL_DESTRUCTOR(GlobalComboBoxStorable);
};




#undef STORABLE_ATOMIC_CLASS
#undef STORABLE_CLASS
#undef STORABLE_CONSTRUCTOR
#undef STORABLE_CONSTRUCTOR2
#undef DECL_DESTRUCTOR

#endif // GLOBALSETTINGS_H
