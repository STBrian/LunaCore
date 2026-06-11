#pragma once

#include <CTRPluginFramework.hpp>
#include "CoreGlobals.hpp"
#include "Core/Filesystem.hpp"

#include <unordered_map>

namespace CTRPF = CTRPluginFramework;

namespace Core {
    namespace Utils {
        std::string strip(const std::string& str);
        std::string LoadFile(const std::string& filepath);
    }
    
    class Config;
    Config LoadConfig(const std::string& filepath);

    class Config {
        private:
        std::unordered_map<std::string, std::string> values;
        std::string _path;
        bool loaded = false;

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
            Core::File configFile(_path, FS_OPEN_WRITE|FS_OPEN_CREATE);
            if (!configFile.isOpen()) {
                return false;
            }
            for (auto key : this->values) {
                std::string writeContent = CTRPF::Utils::Format("%s = %s\n", key.first.c_str(), key.second.c_str());
                configFile.write(writeContent.c_str(), writeContent.size());
            }
            return true;
        }

        bool isLoaded() const {
            return this->loaded;
        }

        friend Config Core::LoadConfig(const std::string& fp);
    };

    inline Config LoadConfig(const std::string& filepath) {
        Config config(filepath);
        if (!Core::Filesystem::FileExists(filepath)) {
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
        config.loaded = true;
        return config;
    }
}