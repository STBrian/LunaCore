#pragma once

#include <map>
#include <string>
#include <3ds/types.h>

class LangParser {
    public:
    std::map<std::string, std::string> mEntries;

    public:
    LangParser() {}

    void load(const std::string& fp);

    bool empty() const {
        return this->mEntries.empty();
    }
};