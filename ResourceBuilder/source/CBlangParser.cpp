#include "CBlangParser.hpp"

#include <cstring>
#include <memory>
#include <fstream>

struct CBlangHeader {
    u32 sig;
    u32 elementsCount;
    u32 totalTextsLength;
};

struct LangEntryRaw {
    u32 hashId;
    u32 textOffset;
};

void CBlangParser::dump(const std::string& fp) {
    if (this->mEntries.empty()) return;

    std::ofstream clangFile(fp, std::ios::binary);
    if (!clangFile.is_open()) return;

    CBlangHeader header{};
    memcpy(&header.sig, "CLNG", 4);

    header.elementsCount = this->mEntries.size();
    LangEntryRaw* entriesBuffer = new(std::nothrow) LangEntryRaw[this->mEntries.size()];
    if (!entriesBuffer) return;

    u32 totalTextsLength = 0;
    u32 index = 0;
    for (auto& entry : this->mEntries) {
        entriesBuffer[index].hashId = entry.first;
        entriesBuffer[index++].textOffset = totalTextsLength;
        totalTextsLength += entry.second.size() + 1;
    }
    header.totalTextsLength = totalTextsLength;

    char* textsBuffer = new(std::nothrow) char[totalTextsLength];
    if (!textsBuffer) {
        delete[] entriesBuffer;
        return;
    }

    u32 offset = 0;
    for (auto& entry : this->mEntries) {
        memcpy(textsBuffer + offset, entry.second.data(), entry.second.size() + 1);
        offset += entry.second.size() + 1;
    }

    clangFile.write(reinterpret_cast<char*>(&header), sizeof(CBlangHeader));
    clangFile.write(reinterpret_cast<char*>(entriesBuffer), header.elementsCount * sizeof(LangEntryRaw));
    clangFile.write(textsBuffer, totalTextsLength);

    delete[] entriesBuffer;
    delete[] textsBuffer;
}