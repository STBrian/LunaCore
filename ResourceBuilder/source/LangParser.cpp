#include "LangParser.hpp"

#include <fstream>

void LangParser::load(const std::string& fp) {
    std::ifstream langFile(fp);
    std::string line;

    if (!langFile.is_open()) return;
    
    while (std::getline(langFile, line)) {
        std::size_t sep = line.find('=');
        if (sep == std::string::npos) continue;

        std::string key = line.substr(0, sep);
        std::string value = line.substr(sep + 1, line.size() - sep - 1);
        this->mEntries[key] = value;
    }
}