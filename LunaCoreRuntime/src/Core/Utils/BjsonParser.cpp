#include "Core/Utils/BjsonParser.hpp"

#include <vector>
#include <string>

using json = nlohmann::ordered_json;
using BjsonHeaderEntry = BjsonParser::BjsonHeaderEntry;

template <typename T>
T read_value(const char*& ptr) {
    T value;
    memcpy(&value, ptr, sizeof(T));
    ptr += sizeof(T);
    return value;
}

json BjsonParser::parse(const char* data, size_t datasize) {
    size_t total_elements = read_value<u32>(data);
    BjsonRegions regions;
    for (size_t i = 0; i < total_elements; i++) {
        regions.structure.push_back(read_value<BjsonStructEntry>(data));
    }

    size_t stringBankLen = read_value<u32>(data);
    regions.stringsBank = data;
    data += stringBankLen;

    size_t total_array_indexes = read_value<u32>(data);
    for (size_t i = 0; i < total_array_indexes; i++) {
        regions.arrayIndexes.push_back(read_value<u32>(data));
    }

    size_t total_header_items = read_value<u32>(data);
    for (size_t i = 0; i < total_header_items; i++) {
        regions.headerIndexes.push_back(read_value<BjsonHeaderEntry>(data));
    }

    size_t headerStringBankLen = read_value<u32>(data);
    regions.headerStringsBank = data;
    data += headerStringBankLen;

    Tracking track;
    BjsonStructEntry& entry = regions.structure[0];
    json root;
    if (entry.data_type == 6) {
        root = json::object();
        parseObject(root, entry, regions, track);
    } else if (entry.data_type == 4) {
        root = json::array();
        parseArray(root, entry, regions, track);
    }
    
    return root;
}

BjsonHeaderEntry* searchForHeader(std::vector<BjsonHeaderEntry>& headerIndexes, u32 index) {
    for (auto& element : headerIndexes) {
        if (element.headerIndex == index) {
            return &element;
        }
    }
    return nullptr;
}

void BjsonParser::parseObject(nlohmann::ordered_json& root, BjsonStructEntry& object, BjsonRegions& regions, Tracking& track) {
    for (size_t i = 0; i < object.objectLength(); i++) {
        track.item_idx++;
        BjsonStructEntry& entry = regions.structure[track.item_idx];
        BjsonHeaderEntry* header = searchForHeader(regions.headerIndexes, track.item_idx);
        if (!header) {
            return;
        } else {
            std::string headerString(regions.headerStringsBank + header->stringPosition);
            switch (entry.data_type)
            {
                case 0:
                    root[headerString] = nullptr;
                    break;
                case 1:
                    root[headerString] = entry.asBoolean();
                    break;
                case 2:
                    root[headerString] = entry.asInteger();
                    break;
                case 3:
                    root[headerString] = entry.asFloat();
                    break;
                case 4:
                    root[headerString] = json::array();
                    parseArray(root[headerString], entry, regions, track);
                    break;
                case 5:
                    root[headerString] = std::string(regions.stringsBank + entry.getStringBankPosition());
                    break;
                case 6:
                    root[headerString] = json::object();
                    parseObject(root[headerString], entry, regions, track);
                    break;
            }
        }
    }
}

void BjsonParser::parseArray(nlohmann::ordered_json& root, BjsonStructEntry& object, BjsonRegions& regions, Tracking& track) {
    for (size_t i = 0; i < object.objectLength(); i++) {
        track.item_idx++;
        BjsonStructEntry& entry = regions.structure[track.item_idx];
        
        switch (entry.data_type)
        {
            case 0:
                root.push_back(nullptr);
                break;
            case 1:
                root.push_back(entry.asBoolean());
                break;
            case 2:
                root.push_back(entry.asInteger());
                break;
            case 3:
                root.push_back(entry.asFloat());
                break;
            case 4: {
                json narray = json::array();
                parseArray(narray, entry, regions, track);
                root.push_back(narray);
                break;
            }
            case 5:
                root.push_back(std::string(regions.stringsBank + entry.getStringBankPosition()));
                break;
            case 6: {
                json nobject = json::object();
                parseObject(nobject, entry, regions, track);
                root.push_back(nobject);
                break;
            }
        }
    }
}