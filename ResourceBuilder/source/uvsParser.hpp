#pragma once

#include <string>
#include <vector>
#include <map>

class UVSParser {
    public:
    struct UVValues {
        float u, v, w, h;
    };

    struct UVEntry {
        std::string name;
        uint16_t width, height;
        struct UVValues uv;
        uint32_t reserved;
    };

    struct UVIndexEntry {
        uint32_t nameHash;
        std::vector<UVEntry> entries;

        uint32_t size() {
            return this->entries.size();
        }
    };
    
    public:
    std::map<uint32_t, UVIndexEntry> mEntries;

    UVSParser() {}

    bool load(const std::string& fp);

    void addEntry(const char* nameId);

    void append(const char* nameId, const char* texName, uint16_t twidth, uint16_t theight, const UVValues& uvs);

    bool dump(const std::string& fp);

    bool checkCollision(const UVValues& uvs) const;

    static UVValues createUVBox(uint16_t twidth, uint16_t theight, uint16_t u1, uint16_t v1, uint16_t u2, uint16_t v2) {
        return UVValues{
            (float)u1 / twidth,
            (float)v1 / theight,
            (float)u2 / twidth,
            (float)v2 / theight,
        };
    }
};