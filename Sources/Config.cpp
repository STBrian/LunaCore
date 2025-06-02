#include "Config.hpp"

#include "Debug.hpp"
#include "Utils/Utils.hpp"

#include <CTRPluginFramework.hpp>

namespace CTRPF = CTRPluginFramework;

std::unordered_map<std::string, std::string> Core::Config::LoadConfig(const std::string &filepath) {
    std::unordered_map<std::string, std::string> config;
    if (!CTRPF::File::Exists(filepath)) {
        Core::Debug::LogMessage("Config file not found. Using defaults", false);
        return config;
    }
    std::string fileContent = Core::Utils::LoadFile(filepath);
    std::string line;
    size_t pos = 0, newLinePos = 0;
    const char *fileContentPtr = fileContent.c_str();

    while (pos < fileContent.size()) {
        while (*fileContentPtr != '\n' && *fileContentPtr != '\0') {
            fileContentPtr++;
            newLinePos++;
        }
        line = fileContent.substr(pos, newLinePos - pos);
        size_t delPos = line.find('=');
        if (delPos != std::string::npos) {
            std::string key = Core::Utils::strip(line.substr(0, delPos));
            std::string value = Core::Utils::strip(line.substr(delPos + 1));
            if (!key.empty() && !value.empty()) {
                config[key] = value;
            }
        }
        fileContentPtr++;
        newLinePos++;
        pos = newLinePos;
    }
    return config;
}

bool Core::Config::SaveConfig(const std::string &filePath, std::unordered_map<std::string, std::string> &config) {
    CTRPF::File configFile;
    CTRPF::File::Open(configFile, filePath, CTRPF::File::WRITE);
    if (!configFile.IsOpen()) {
        if (!CTRPF::File::Exists(filePath)) {
            CTRPF::File::Create(filePath);
            CTRPF::File::Open(configFile, filePath);
            if (!configFile.IsOpen())
                return false;
        } else
            return false;
    }
    configFile.Clear();
    for (auto key : config) {
        std::string writeContent = CTRPF::Utils::Format("%s = %s\n", key.first.c_str(), key.second.c_str());
        configFile.Write(writeContent.c_str(), writeContent.size());
    }
    return true;
}

bool Core::Config::GetBoolValue(std::unordered_map<std::string, std::string> &config, const std::string &field, bool def) {
    bool value = false;
    if (config.find(field) != config.end()) {
        if (config[field] == "true")
            value = true;
    } else { // Load default
        value = def;
        if (def)
            config[field] = "true";
    }
    if (!value)
        config[field] = "false";
    return value;
}