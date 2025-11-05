#pragma once

#include <string>
#include <time.h>

#include "CoreGlobals.hpp"
#include "lua_common.h"

namespace Core {
    namespace Utils {
        bool checkTitle();

        void getRegion(std::string &reg);

        bool checkCompatibility();

        std::string formatTime(time_t time);

        std::string strip(const std::string& str);

        void toLower(std::string &str);

        std::string convertToLower(const std::string &str);

        std::string LoadFile(const std::string& filepath);

        inline bool startsWith(const std::string& str, const std::string& prefix) {
            return str.compare(0, prefix.size(), prefix) == 0;
        }

        inline void Replace(std::string& str, const std::string& pattern, const std::string& repl) {
            size_t pos = 0;
            while ((pos = str.find(pattern, pos)) != std::string::npos) {
                str.replace(pos, pattern.size(), repl);
                pos += repl.size();
            }
        }
    }
    
    bool RegisterUtilsModule(lua_State *L);

    void UnregisterUtilsModule(lua_State *L);
}