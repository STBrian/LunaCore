#pragma once

#include "CoreGlobals.hpp"

#include <unordered_map>

namespace Core {
    namespace Config {
        std::unordered_map<std::string, std::string> LoadConfig(const std::string& filepath);

        bool SaveConfig(const std::string& filePath, std::unordered_map<std::string, std::string>& config);

        bool GetBoolValue(std::unordered_map<std::string, std::string>& config, const std::string& field, bool def);
    }
}