#ifndef UTILS_H
#define UTILS_H

#include <string>

namespace utils
{
    template<typename... Args>
    std::string string_format(const char* fmt, Args... args)
    {
        size_t size = snprintf(nullptr, 0ul, fmt, args...);
        std::string buf;
        buf.reserve(size + 1);
        buf.resize(size);
        snprintf(&buf[0], size + 1, fmt, args...);
        return buf;
    }
}

#endif // UTILS_H
