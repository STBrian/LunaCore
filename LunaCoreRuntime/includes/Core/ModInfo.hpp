#pragma once

#include <string>
#include <vector>

namespace Core {
    struct ModInfo {
        std::string id;
        std::string name;
        std::string description;
        std::string version;
        std::vector<std::string> dependencies;
        std::vector<std::string> optionalDependencies;
        std::string folderPath;
    };

    extern std::vector<ModInfo> LoadedModList;
}
