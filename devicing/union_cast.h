#ifndef UNION_CAST_H
#define UNION_CAST_H

namespace utility
{

    template<class T, class Src>
    T union_cast(Src src)
    {
        union
        {
            Src s;
            T d;
        } tmp;
        tmp.s = src;
        return tmp.d;
    }
}
#endif // UNION_CAST_H
