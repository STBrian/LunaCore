#pragma once

#include "game/gstd.hpp"
#include "game/types.h"

#ifndef NOSTDLIB_BUILD
#include <string>
#endif

// If building with nostdlib, you must provide a minimal libc
#include <string.h>

namespace gstd { // to follow rairai's implementation
    class string {
        public:
        char* data = nullptr;

        string() {
            reinterpret_cast<char**(*)(char**, const char*)>(0x2ff220|1)(&data, "");
        }

        #ifndef NOSTDLIB_BUILD
        string(const std::string& str) {
            reinterpret_cast<char**(*)(char**, const char*)>(0x2ff220|1)(&data, str.c_str());
        }
        #endif

        string(const char* str) {
            reinterpret_cast<char**(*)(char**, const char*)>(0x2ff220|1)(&data, str);
        }

        ~string() {
            reinterpret_cast<void(*)(char**)>(0x2febbc|1)(&data);
        }

        uint32_t length() const {
            return *reinterpret_cast<uint32_t*>(data - 4);
        }

        bool empty() {
            return length() == 0;
        }

        const char* c_str() {
            return data;
        }

        #ifndef NOSTDLIB_BUILD
        std::string to_std() {
            return std::string(data, length());
        }

        operator std::string() {
            return to_std();
        }

        bool operator==(const std::string& rhs) {
            return length() == rhs.length() && memcmp(data, rhs.data(), length()) == 0;
        }
        #endif

        bool operator==(const char* rhs) {
            return strncmp(data, rhs, length()) == 0 && rhs[length()] == '\0';
        }

        bool operator==(const string& rhs) const {
            return length() == rhs.length() && memcmp(data, rhs.data, length()) == 0;
        }
    };

    inline gstd::string format(const char* fmt, ...) {
        char buffer[0x100] = {0};
        va_list argList;
        va_start(argList, fmt);
        gstd::vsnprintf(buffer, sizeof(buffer), fmt, argList);
        va_end(argList);
        return gstd::string(buffer);
    }
}