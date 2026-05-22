#pragma once

#include <string>
#include "types.h"

namespace Core {
    void GetCoreInfo(std::string& plgTitle, std::string& plgAuthor, std::string& plgSummary, std::string& plgDescription);

    void ParseVersion(u32 ver);

    void InitCore();

    void LoadLuaEnv();

    bool LoadBuffer(const char *buffer, size_t size, const char* name);

    bool LoadScript(const std::string& fp, const std::string& name);

    void PreloadScripts();

    void LoadMods();
}