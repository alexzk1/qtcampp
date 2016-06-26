#ifndef CLOSABLE_H
#define CLOSABLE_H

/*
 * (C) By Oleksiy Zakharov 2016, alexzkhr@gmail.com
 *
 * Simple resource auto-closable manager, accepts functor to be used on destroy.
 * Should be used with things like file descriptor.
 *
*/
#include <functional>

template <class T>
class auto_closable
{
public:
    using ValueType = typename std::enable_if<std::is_fundamental<T>::value, T>::type;
    using DctorType = std::function<void(T&)>;
private:
    ValueType value;
    DctorType dctor;
public:

    auto_closable() = delete;
    auto_closable(const auto_closable&) = delete;
    auto_closable& operator=(const auto_closable&) = delete;

    auto_closable(const ValueType& res, const DctorType& func):
        value(res),
        dctor(func)
    {
    }

    ~auto_closable()
    {
        dctor(value);
    }

    explicit operator ValueType () const
    {
       return get();
    }

    ValueType get() const
    {
        return value;
    }
};


#endif // CLOSABLE_H
