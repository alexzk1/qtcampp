//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com
#ifndef SINGLETON_H
#define SINGLETON_H

#include <atomic>
#include <memory>
#include <string>

template <class T>
inline T& globalInstance()
{
    static T inst;
    return inst;
}

template <class T>
const inline T& globalInstanceConst()
{
    static const T inst;
    return inst;
}

template <class T>
class ItCanBeOnlyOne
{
private:
    T* dummy;
    std::atomic_flag& lock()
    {
        static std::atomic_flag locked = ATOMIC_FLAG_INIT;
        return locked;
    }

protected:
    ItCanBeOnlyOne() :
        dummy(nullptr)
    {
        if (lock().test_and_set())
        {
            std::string s = "It can be only one instance of " + std::string(typeid(T).name());
            throw std::runtime_error(s);
        }
    }
    virtual ~ItCanBeOnlyOne()
    {
        lock().clear();
    }
};


#endif // SINGLETON_H
