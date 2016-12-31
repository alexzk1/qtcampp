#ifndef NO_CONST_H
#define NO_CONST_H

#include <type_traits>

namespace utility
{
    template <class T>
    T* noconst(const T *ptr)
    {
        return const_cast<T*>(ptr);
    }
}
#endif // NO_CONST_H
