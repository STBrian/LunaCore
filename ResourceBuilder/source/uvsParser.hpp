#pragma once

#include <string>
#include <vector>

class UVSParser {
    public:
    struct UVValues {
        uint16_t u, v, w, h;
    };

    struct UVEntry {
        std::string name;
        uint16_t width, height;
        struct UVValues uv;
    };

    private:
    bool mLoaded = false;
    std::vector<UVEntry> mEntries;
    
    public:

    UVSParser() {}

    void load(const std::string& fp);


    bool isLoaded() {
        return this->mLoaded;
    }
};