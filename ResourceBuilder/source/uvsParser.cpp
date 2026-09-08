#include "uvsParser.hpp"

#include <cstdint>
#include <fstream>
#include <new>
#include <memory>

struct tmp_UVEntry {
    float u1, v1, u2, v2;
    uint16_t width, height;
    uint32_t nameOffset;
    uint32_t reserved1; // magic number? bit mask?
};

struct tmp_UVIndexEntry {
    uint32_t 
};

void UVSParser::load(const std::string& fp) {
    std::ifstream uvsFile(fp, std::ios::binary);
    if (!uvsFile.is_open()) {
        return;
    }

    uint32_t count_unique_elements, count_elements, string_section_length;
    uvsFile.read(reinterpret_cast<char*>(&count_unique_elements), sizeof(uint32_t));
    uvsFile.read(reinterpret_cast<char*>(&count_elements), sizeof(uint32_t));
    uvsFile.read(reinterpret_cast<char*>(&string_section_length), sizeof(uint32_t));
    
    char* namesBufferPtr = new(std::nothrow) char[string_section_length];
    if (!namesBufferPtr) {
        return;
    }
    std::unique_ptr<char> namesBuffer(namesBufferPtr);

    uvsFile.read(namesBuffer.get(), string_section_length);

    this->mLoaded = true;
}