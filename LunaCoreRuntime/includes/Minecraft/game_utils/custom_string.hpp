#pragma once

#include <cstring>
#include <string>
#include <cstdint>

// This class depends on the game implementation of memalloc
// so you cannot try to reserve space for this class without using
// game memalloc
class CustomString {
    public:
    char* data;

    CustomString() {
        data = nullptr;
    }

    CustomString(const std::string& str) {
        reinterpret_cast<char**(*)(char**, const char*)>(0x2ff220|1)(&data, str.c_str());
    }

    CustomString(const char* str) {
        reinterpret_cast<char**(*)(char**, const char*)>(0x2ff220|1)(&data, str);
    }

    ~CustomString() {
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

    bool operator==(const std::string& rhs) {
        return length() == rhs.length() && std::memcmp(data, rhs.data(), length()) == 0;
    }

    bool operator==(const char* rhs) {
        return std::strncmp(data, rhs, length()) == 0 && rhs[length()] == '\0';
    }

    bool operator==(const CustomString& rhs) const {
        return length() == rhs.length() && std::memcmp(data, rhs.data, length()) == 0;
    }
};