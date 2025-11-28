#pragma once

#include "game/types.h"

#ifndef NOSTDLIB_BUILD
#include <cstring>
#include <string>
#endif

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
            return length() == rhs.length() && std::memcmp(data, rhs.data(), length()) == 0;
        }

        bool operator==(const char* rhs) {
            return std::strncmp(data, rhs, length()) == 0 && rhs[length()] == '\0';
        }

        bool operator==(const string& rhs) const {
            return length() == rhs.length() && std::memcmp(data, rhs.data, length()) == 0;
        }
        #endif
    };
}