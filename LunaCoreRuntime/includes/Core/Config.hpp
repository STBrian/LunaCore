#pragma once

#include <CTRPluginFramework.hpp>
#include "CoreGlobals.hpp"

#include <unordered_map>

namespace CTRPF = CTRPluginFramework;

namespace Core {
    namespace Debug {
        void LogWarn(const std::string& msg);
    }

    namespace Utils {
        std::string strip(const std::string& str);
        std::string LoadFile(const std::string& filepath);
    }

    class Config {
        private:
        std::unordered_map<std::string, std::string> values;
        std::string _path;

        Config() {}

        public:
        Config(const std::string &path) : _path(path) {}

        void set(const std::string& key, const std::string& value) {
            values[key] = value;
            save();
        }

        void set(const std::string& key, bool value) {
            if (value) values[key] = "true";
            else values[key] = "false";
            save();
        }

        bool getBool(const std::string& key, bool def = false) {
            if (values.find(key) != values.end())
                return values[key] == "true";
            else {
                set(key, def);
                return def;
            }
        }

        std::string getString(const std::string& key, const std::string& def = "") {
            if (values.find(key) != values.end())
                return values[key];
            else {
                set(key, def);
                return def;
            }
        }

        /* save method is already called when something is changed. This is done this way
        to avoid the repetition of save code fragments across files */
        bool save() {
            CTRPF::File configFile;
            CTRPF::File::Open(configFile, _path, CTRPF::File::WRITE);
            if (!configFile.IsOpen()) {
                if (!CTRPF::File::Exists(_path)) {
                    CTRPF::File::Create(_path);
                    CTRPF::File::Open(configFile, _path);
                    if (!configFile.IsOpen()) {
                        Core::Debug::LogWarn("Failed to save configs");
                        return false;
                    }
                } else {
                    Core::Debug::LogWarn("Failed to save configs");
                    return false;
                }
            }
            configFile.Clear();
            for (auto key : this->values) {
                std::string writeContent = CTRPF::Utils::Format("%s = %s\n", key.first.c_str(), key.second.c_str());
                configFile.Write(writeContent.c_str(), writeContent.size());
            }
            return true;
        }
    };

    inline Config LoadConfig(const std::string& filepath) {
        Config config(filepath);
        if (!CTRPF::File::Exists(filepath)) {
            Core::Debug::LogWarn("Config file not found. Using defaults");
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
                    config.set(key, value);
                }
            }
            fileContentPtr++;
            newLinePos++;
            pos = newLinePos;
        }
        return config;
    }
}