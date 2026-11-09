#include "uvsParser.hpp"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <new>
#include <memory>
#include <ostream>

#include "string_hash.hpp"

struct tmp_UVEntry {
    float u1, v1, u2, v2;
    uint16_t width, height;
    uint32_t nameOffset;
    uint32_t reserved1; // magic number? bit mask?
};

struct tmp_UVIndexEntry {
    uint32_t nameHash, elementsCount, index;
};

bool UVSParser::load(const std::string& fp) {
    std::ifstream uvsFile(fp, std::ios::binary);
    if (!uvsFile.is_open()) {
        return false;
    }

    struct {
        uint32_t count_unique_elements = 0, count_elements = 0, string_section_length = 0;
    } uvs_header;

    if (!uvsFile.read(reinterpret_cast<char*>(&uvs_header), sizeof(uvs_header))) return false;
    
    char* namesBufferPtr = new(std::nothrow) char[uvs_header.string_section_length];
    if (!namesBufferPtr) {
        return false;
    }
    std::unique_ptr<char[]> namesBuffer(namesBufferPtr);

    if (!uvsFile.read(namesBuffer.get(), uvs_header.string_section_length)) return false;

    tmp_UVEntry* uvEntriesPtr = new(std::nothrow) tmp_UVEntry[uvs_header.count_elements];
    if (!uvEntriesPtr) {
        return false;
    }
    std::unique_ptr<tmp_UVEntry[]> uvEntries(uvEntriesPtr);

    if (!uvsFile.read(reinterpret_cast<char*>(uvEntries.get()), sizeof(tmp_UVEntry) * uvs_header.count_elements)) return false;

    tmp_UVIndexEntry* uvIndexEntriesPtr = new(std::nothrow) tmp_UVIndexEntry[uvs_header.count_unique_elements];
    if (!uvIndexEntriesPtr) {
        return false;
    }
    std::unique_ptr<tmp_UVIndexEntry[]> uvIndexEntries(uvIndexEntriesPtr);

    if (!uvsFile.read(reinterpret_cast<char*>(uvIndexEntries.get()), sizeof(tmp_UVIndexEntry) * uvs_header.count_unique_elements)) return false;

    for (uint32_t i = 0; i < uvs_header.count_unique_elements; i++) {
        uint32_t nameHash = uvIndexEntries[i].nameHash;
        uint32_t elementsCount = uvIndexEntries[i].elementsCount;
        uint32_t indexOffset = uvIndexEntries[i].index;

        this->mEntries[nameHash] = UVIndexEntry{};
        this->mEntries[nameHash].nameHash = uvIndexEntries[i].nameHash;
        this->mEntries[nameHash].entries.resize(elementsCount);

        for (uint32_t j = 0; j < elementsCount; j++) {
            tmp_UVEntry* entry = uvEntries.get() + (indexOffset + j);
            UVEntry& refEntry = this->mEntries[nameHash].entries[j];

            refEntry.name = (namesBuffer.get() + entry->nameOffset);
            refEntry.width = entry->width;
            refEntry.height = entry->height;
            refEntry.uv.u = entry->u1;
            refEntry.uv.v = entry->v1;
            refEntry.uv.w = entry->u2;
            refEntry.uv.h = entry->v2;
            refEntry.reserved = entry->reserved1;
        }
    }

    return true;
}

void UVSParser::addEntry(const char* nameId) {
    uint32_t hashValue = hash(nameId);
    this->mEntries[hashValue] = UVIndexEntry{};
}

void UVSParser::append(const char* nameId, const char* texName, uint16_t width, uint16_t height, const UVValues& uvs) {
    uint32_t hashValue = hash(nameId);
    this->mEntries[hashValue].entries.push_back(UVEntry{texName, width, height, uvs, 0x0C0C0C00});
}

bool UVSParser::dump(const std::string& fp) {
    if (this->mEntries.size() == 0) return true;

    std::ofstream uvsFile(fp, std::ios::binary);
    if (!uvsFile.is_open()) {
        return false;
    }

    struct {
        uint32_t count_unique_elements = 0, count_elements = 0, string_section_length = 0;
    } uvs_header;

    for (auto& pair : this->mEntries) {
        if (pair.second.entries.empty()) continue;

        uvs_header.count_unique_elements++;
        uvs_header.count_elements += pair.second.entries.size();

        for (auto& entry : pair.second.entries) {
            uvs_header.string_section_length += entry.name.size() + 1;
        }
    }

    if (uvs_header.count_elements == 0) return true;

    char* namesBuffer = (char*)malloc(uvs_header.string_section_length);
    tmp_UVEntry* uvsElementsBuffer = (tmp_UVEntry*)malloc(uvs_header.count_elements * sizeof(tmp_UVEntry));
    tmp_UVIndexEntry* uvsEntriesBuffer = (tmp_UVIndexEntry*)malloc(uvs_header.count_unique_elements * sizeof(tmp_UVIndexEntry));

    if (!namesBuffer || !uvsElementsBuffer || !uvsEntriesBuffer) {
        if (namesBuffer) free(namesBuffer);
        if (uvsElementsBuffer) free(uvsElementsBuffer);
        if (uvsEntriesBuffer) free(uvsEntriesBuffer);
        return false;
    }

    uint32_t currEntryIndex = 0, currElementIndex = 0, currNameOffset = 0;
    for (auto& pair : this->mEntries) {
        if (pair.second.entries.empty()) continue;

        uvsEntriesBuffer[currEntryIndex].elementsCount = pair.second.entries.size();
        uvsEntriesBuffer[currEntryIndex].nameHash = pair.first;
        uvsEntriesBuffer[currEntryIndex].index = currElementIndex;
        currEntryIndex++;

        for (auto& entry : pair.second.entries) {
            memcpy(&uvsElementsBuffer[currElementIndex].u1, &entry.uv, sizeof(UVValues));
            uvsElementsBuffer[currElementIndex].nameOffset = currNameOffset;
            uvsElementsBuffer[currElementIndex].width = entry.width;
            uvsElementsBuffer[currElementIndex].height = entry.height;
            uvsElementsBuffer[currElementIndex].reserved1 = entry.reserved;
            currElementIndex++;

            strncpy(namesBuffer + currNameOffset, entry.name.c_str(), entry.name.size() + 1);
            currNameOffset += entry.name.size() + 1;
        }
    }

    uvsFile.write(reinterpret_cast<char*>(&uvs_header), sizeof(uvs_header));
    uvsFile.write(namesBuffer, uvs_header.string_section_length);
    uvsFile.write(reinterpret_cast<char*>(uvsElementsBuffer), uvs_header.count_elements * sizeof(tmp_UVEntry));
    uvsFile.write(reinterpret_cast<char*>(uvsEntriesBuffer), uvs_header.count_unique_elements * sizeof(tmp_UVIndexEntry));

    return true;
}

static bool checkAABBCollision(const UVSParser::UVValues& uvsA, const UVSParser::UVValues& uvsB) {
    return (
        uvsA.u < uvsB.w && 
        uvsA.w > uvsB.u && 
        uvsA.v < uvsB.h &&
        uvsA.h > uvsB.v
    );
}

bool UVSParser::checkCollision(const UVValues& uvs) const {
    for (auto& entry : this->mEntries) {
        for (auto& element : entry.second.entries) {
            if (checkAABBCollision(uvs, element.uv)) {
                return true;
            }
        }
    }
    return false;
}