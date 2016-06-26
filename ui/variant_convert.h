//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com

#ifndef VARIANT_CONVERT_H
#define VARIANT_CONVERT_H
#include <QVariant>
#include <QString>
#include <QDate>

//okey...maybe all that can be skipped and used QVariant::value<T>() ... however,types must be registered with meta-system then

template<class T, typename T2 = typename std::enable_if<!std::is_enum<T>::value, void>::type>
T variantTo(const QVariant& var);

//to resolve enum is ambiqous with ints ... making different amount of template args
template<typename T, typename T3 = int, typename T2 = typename std::enable_if<std::is_enum<T>::value, T>::type>
inline
T variantTo(const QVariant& var)
{
    return static_cast<T>(var.toInt());
}

template<>
inline QString variantTo(const QVariant& var)
{
    return var.toString();
}

template<>
inline QDate variantTo(const QVariant& var)
{
    return var.toDate();
}

template<>
inline int variantTo(const QVariant& var)
{
    return var.toInt();
}

template<>
inline qint64 variantTo(const QVariant& var)
{
    return var.toLongLong();
}

template<>
inline bool variantTo(const QVariant& var)
{
    return var.toBool();
}

template<>
inline uint variantTo(const QVariant& var)
{
    return var.toUInt();
}

template<>
inline quint64 variantTo(const QVariant& var)
{
    return var.toULongLong();
}

template<>
inline double variantTo(const QVariant& var)
{
    return var.toDouble();
}
#endif // VARIANT_CONVERT_H
