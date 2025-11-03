#pragma once

#include "json.hpp"
#include "types.h"

class BjsonParser {
    public:
    class BjsonStructEntry{
        public:
        u32 data_type;
        u32 rawValue1;
        u32 rawValue2;

        bool asBoolean() {
            return rawValue1 > 0;
        }

        void setBoolean(bool value) {
            rawValue1 = value ? 1 : 0;
            rawValue2 = 0;
        }

        s32 asInteger() {
            s32 res;
            memcpy(&res, &rawValue1, 4);
            return res;
        }

        void setInteger(s32 value) {
            memcpy(&rawValue1, &value, 4);
            rawValue2 = 0;
        }

        float asFloat() {
            float res;
            memcpy(&res, &rawValue1, 4);
            return res;
        }

        void setFloat(float value) {
            memcpy(&rawValue1, &value, 4);
            rawValue2 = 0;
        }

        u32 getStringBankPosition() {
            return rawValue2;
        }

        u32 objectLength() {
            return rawValue1;
        }
    };

    typedef struct {
        u32 stringHash;
        u32 stringPosition;
        u32 headerIndex;
    } BjsonHeaderEntry;

    class BjsonRegions {
        public:
        std::vector<BjsonStructEntry> structure;
        const char* stringsBank;
        std::vector<u32> arrayIndexes;
        std::vector<BjsonHeaderEntry> headerIndexes;
        const char* headerStringsBank;
    };

    class Tracking {
        public:
        u32 item_idx = 0;
        u32 objects_length = 0;
        u32 arrays_length = 0;
    };

    static nlohmann::ordered_json parse(const char* data, size_t datasize);

    static void parseObject(nlohmann::ordered_json& root, BjsonStructEntry& object, BjsonRegions& regions, Tracking& track);

    static void parseArray(nlohmann::ordered_json& root, BjsonStructEntry& object, BjsonRegions& regions, Tracking& track);

};