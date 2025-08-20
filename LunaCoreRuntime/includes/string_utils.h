#pragma once

#ifndef __LCEX__
#ifndef USE_CUSTOM_STRING
#error "This header should be used with USE_CUSTOM_STRING flag"
#endif
#include <string.h>
#else
#include "extras.h"
#include "lcruntime_wrapper.h"
#endif

#include <stdarg.h>

#include "types.h"

struct sstring_s {
    char* cstr;
    u32 length;
};

typedef struct sstring_s* sstring;

#ifdef __cplusplus
extern "C" {
#endif

sstring sfromcstr(const char* cstr);

void sassigncstr(sstring string, const char* cstr);

void sassignformat(sstring string, const char* format, ...);

sstring sformat(const char* format, ...);

void scatcstr(sstring string, const char* cstr);

void sdestroy(sstring string);

#ifdef __cplusplus
}

#ifndef __LCEX__
#include <string>
#endif

#define ASSERT(cond) \
    do { \
        if (!(cond)) { \
            Core::Debug::LogError(SString("ASSERT failed: ") + #cond + "(" + __FILE__ + ": " + std::to_string(__LINE__) + ")"); \
            abort(); \
        } \
    } while (0)

class SString;

namespace Core::Debug {
    void LogError(const SString& msg);
    void LogMessage(const SString& msg, bool showOnScreen);
}

class SString {
    sstring data = NULL;

    public:
    SString() {
        Core::Debug::LogMessage("Constructor SString empty", false);
        data = sfromcstr("");
        ASSERT(data != NULL && data->cstr != NULL);
    }

    SString(const SString& s) {
        Core::Debug::LogMessage("Constructor SString SString", false);
        data = sfromcstr(s.data->cstr);
        ASSERT(data != NULL && data->cstr != NULL);
    }

    SString(const char* s) {
        Core::Debug::LogMessage("Constructor const char*", false);
        data = sfromcstr(s);
        ASSERT(data != NULL && data->cstr != NULL);
    }

    #ifndef __LCEX__
    SString(const std::string& s) {
        Core::Debug::LogMessage("Constructor SString std::string", false);
        data = sfromcstr(s.c_str());
        ASSERT(data != NULL && data->cstr != NULL);
    }

    operator std::string() const {
        Core::Debug::LogMessage("operator SString std::string", false);
        ASSERT(data != NULL && data->cstr != NULL);
        return std::string(data->cstr, data->length - 1);
    }
    #endif

    ~SString() {
        Core::Debug::LogMessage("Destructor SString", false);
        ASSERT(data != NULL && data->cstr != NULL);
        sdestroy(data);
    }

    const char* c_str() const {
        Core::Debug::LogMessage("SString to_cstr", false);
        return data->cstr;
    }

    size_t size() const {
        Core::Debug::LogMessage("SString size", false);
        return data->length - 1;
    }

    size_t length() const {
        Core::Debug::LogMessage("SString length", false);
        return data->length - 1;
    }

    bool empty() const {
        Core::Debug::LogMessage("SString empty", false);
        return size() == 0;
    }

    void clear() {
        Core::Debug::LogMessage("SString clear", false);
        ASSERT(data != NULL && data->cstr != NULL);
        sdestroy(data);
        data = sfromcstr("");
    }

    void assign(const char* s) {
        Core::Debug::LogMessage("SString assign", false);
        sassigncstr(data, s);
        ASSERT(data != NULL && data->cstr != NULL);
    }

    SString& operator=(const char* s) {
        Core::Debug::LogMessage("operator SString = const char*", false);
        assign(s);
        return *this;
    }

    SString& operator+=(const char* rhs) {
        Core::Debug::LogMessage("operator SString += const char*", false);
        scatcstr(data, rhs);
        return *this;
    }

    SString& operator+=(const SString& rhs) {
        Core::Debug::LogMessage("operator SString += SString", false);
        *this += rhs.data->cstr;
        return *this;
    }

    bool operator==(const char* rhs) const {
        Core::Debug::LogMessage("operator SString == const char*", false);
        return strcmp(this->data->cstr, rhs) == 0;
    }

    bool operator==(const SString& rhs) const {
        Core::Debug::LogMessage("operator SString == SString", false);
        return *this == rhs.data->cstr;
    }

    friend SString operator+(const SString& lhs, const char* rhs) {
        Core::Debug::LogMessage("operator SString + const char*", false);
        SString tmp(lhs);
        tmp += rhs;
        return tmp;
    }

    friend SString operator+(const char* lhs, const SString& rhs) {
        Core::Debug::LogMessage("operator const char* + SString", false);
        SString tmp(lhs);
        tmp += rhs;
        return tmp;
    }

    friend SString operator+(const SString& lhs, const SString& rhs) {
        Core::Debug::LogMessage("operator SString + SString", false);
        SString tmp(lhs);
        tmp += rhs;
        return tmp;
    }
};

#ifndef __LCEX__
namespace std {
    template<>
    struct hash<SString> {
        size_t operator()(const SString& s) const noexcept {
            const char* str = s.c_str();
            u32 hash_ = 0;
            while (*str)
            {
                hash_ += *str;
                hash_ &= 0xFFFFFFFF;
                hash_ += (hash_ << 10);
                hash_ &= 0xFFFFFFFF;
                hash_ ^= (hash_ >> 6);
                str++;
            }
            hash_ += (hash_ << 3);
            hash_ &= 0xFFFFFFFF;
            hash_ ^= (hash_ >> 11);
            hash_ += (hash_ << 15);
            return hash_ & 0xFFFFFFFF;
        }
    };
}
#endif

#endif