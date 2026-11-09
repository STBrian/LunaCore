#pragma once

#include <map>
#include <string>
#include <3ds/types.h>

#include "string_hash.hpp"

class CBlangParser {
    private:
    std::map<u32, std::string> mEntries;

    public:
    CBlangParser() {}

    void dump(const std::string& fp);

    void add(const std::string& key, const std::string& value) {
        this->mEntries[hash(key.c_str())] = value;
    }
};