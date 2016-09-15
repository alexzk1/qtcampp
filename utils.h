#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <type_traits>

#ifdef QT_CORE_LIB
#include <QString>
#endif

//usage std::string s = string_format() << "Operation with id = " << id << " failed, because data1 (" << data1 << ") is incompatible with data2 (" << data2 << ")"
//from https://habrahabr.ru/post/131977/

#ifndef _WIN32
#include <limits.h>
#include <stdlib.h>
#else
#include <windows.h>
#endif

namespace utility
{
    class string_format
    {
    public:
        template<class T>
        string_format& operator<< (const T& arg)
        {
            m_stream << arg;
            return *this;
        }
        operator std::string() const
        {
            return m_stream.str();
        }

#ifdef QT_CORE_LIB
        operator QString() const
        {
            return QString::fromUtf8(m_stream.str().c_str());
        }
#endif

    protected:
        std::stringstream m_stream;
    };


    template< typename... Args >
    std::string string_sprintf( const char* format, Args... args )
    {
        int length = std::snprintf( nullptr, 0, format, args... );
        char* buf = new char[length + 1];
        std::snprintf( buf, length + 1, format, args... );
        std::string str( buf );
        delete[] buf;
        return str;
    }

    //gets filename without path and WITH extension
    inline std::string baseFileName(const std::string& pathname)
    {
        return {std::find_if(pathname.rbegin(), pathname.rend(),
                             [](char c) { return c == '/' || c == '\\'; }).base(),
                    pathname.end()};
    }

    inline std::string getAbsolutePath(const std::string& fileName)
    {
        std::string result;
#ifndef _WIN32
        char *full_path = realpath(fileName.c_str(), nullptr);
        if (full_path)
        {
            result = std::string(full_path);
            free(full_path);
        }
#else
        TCHAR full_path[MAX_PATH];
        GetFullPathName(_T(fileName.c_str()), MAX_PATH, full_path, NULL);
        result = std::string(full_path);
#error Revise this code for windows
#endif
        return result;
    }


    //from this article: http://cpp.indi.frih.net/blog/2014/09/how-to-read-an-entire-file-into-memory-in-cpp/
    template <typename CharT = char,
              typename Traits = std::char_traits<char>>
    std::streamsize streamSizeToEnd(std::basic_istream<CharT, Traits>& in)
    {
        auto const start_pos = in.tellg();
        if (std::streamsize(-1) == start_pos)
            throw std::ios_base::failure{"error"};

        if (!in.ignore(std::numeric_limits<std::streamsize>::max()))
            throw std::ios_base::failure{"error"};

        const std::streamsize char_count = in.gcount();

        if (!in.seekg(start_pos))
            throw std::ios_base::failure{"error"};

        return char_count;
    }

    template <typename Container = std::string,
              typename CharT = char,
              typename Traits = std::char_traits<char>>
    Container read_stream_into_container(
            std::basic_istream<CharT, Traits>& in,
            typename Container::allocator_type alloc = {})
    {
        static_assert(
                    // Allow only strings...
                    std::is_same<Container, std::basic_string<CharT,
                    Traits,
                    typename Container::allocator_type>>::value ||
                    // ... and vectors of the plain, signed, and
                    // unsigned flavours of CharT.
                    std::is_same<Container, std::vector<CharT,
                    typename Container::allocator_type>>::value ||
                    std::is_same<Container, std::vector<
                    typename std::make_unsigned<CharT>::type,
                    typename Container::allocator_type>>::value ||
                    std::is_same<Container, std::vector<
                    typename std::make_signed<CharT>::type,
                    typename Container::allocator_type>>::value,
                    "only strings and vectors of ((un)signed) CharT allowed");

        auto const char_count = streamSizeToEnd(in);

        auto container = Container(std::move(alloc));
        container.resize(char_count);

        if (0 != container.size())
        {
            if (!in.read(reinterpret_cast<CharT*>(&container[0]), container.size()))
                throw std::ios_base::failure{"File size differs"};
        }
        return container;
    }

    inline std::string toLower(std::string src)
    {
        std::transform(src.begin(), src.end(), src.begin(), ::tolower);
        return src;
    }

    inline bool endsWith (std::string const &fullString, std::string const &ending)
    {
        if (fullString.length() >= ending.length())
        {
            return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
        }
        return false;
    }

    inline std::string removeExtension(const std::string& fileName)
    {
        size_t lastindex = fileName.find_last_of(".");
        return fileName.substr(0, lastindex);
    }
}

#endif // UTILS_H
