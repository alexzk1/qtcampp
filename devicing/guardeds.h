#ifndef GUARDEDS_H
#define GUARDEDS_H

#include <map>
#include <mutex>
#include <functional>

template <class K, class V, class Compare = std::less<K>, class Allocator = std::allocator<std::pair<const K, V> > >
class guarded_map
{
public:
    using HoldedMap    = std::map<K, V, Compare, Allocator>;
    using ForEachT     = typename HoldedMap::value_type;
    using ForEachFuncT = std::function<void(ForEachT&)>;
private:
    HoldedMap _map;
    std::mutex _m;

public:
    void set(const K& key, const V& value)
    {
        std::lock_guard<std::mutex> lk(this->_m);
        this->_map[key] = value;
    }

    V & get(const K& key) const
    {
        std::lock_guard<std::mutex> lk(this->_m);
        return this->_map.at(key);
    }

    size_t count(const K& key) const
    {
        std::lock_guard<std::mutex> lk(this->_m);
        return this->_map.count(key);
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lk(this->_m);
        return this->_map.empty();
    }

    void for_each(const ForEachFuncT& func)
    {
        std::lock_guard<std::mutex> lk(this->_m);
        for (ForEachT& v : this->_map)
            func(v);
    }
};

#endif // GUARDEDS_H
