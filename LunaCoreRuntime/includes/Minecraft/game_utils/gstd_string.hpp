#pragma once

#include <cstring>
#include <string>
#include <cstdint>

// This class depends on the game implementation of memalloc
// so you cannot try to reserve space for this class without using
// game memalloc
namespace gstd { // to follow rairai's implementation
    class string {
        public:
        char* data;

        string() {
            data = nullptr;
        }

        string(const std::string& str) {
            reinterpret_cast<char**(*)(char**, const char*)>(0x2ff220|1)(&data, str.c_str());
        }

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
    };
}