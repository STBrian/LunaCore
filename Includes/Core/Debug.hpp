#pragma once

#include <string>

#include "lua_common.h"

namespace Core {
    namespace Debug {
        extern const char *tab;
        
        bool OpenLogFile(const std::string &filepath);

        void CloseLogFile();

        void LogRaw(const std::string& msg);

        void LogMessage(const std::string& msg, bool showOnScreen);

        void LogError(const std::string& msg);

        void Message(const std::string& msg);

        void Error(const std::string& msg);
    }

    namespace Module {
        bool RegisterDebugModule(lua_State *L);
    }
}