#pragma once

#include "CoreGlobals.hpp"

#include <unordered_map>

namespace Core {
    namespace Config {
        std::unordered_map<STRING_CLASS, STRING_CLASS> LoadConfig(const STRING_CLASS& filepath);

        bool SaveConfig(const STRING_CLASS& filePath, std::unordered_map<STRING_CLASS, STRING_CLASS>& config);

        bool GetBoolValue(std::unordered_map<STRING_CLASS, STRING_CLASS>& config, const STRING_CLASS& field, bool def);
    }
}