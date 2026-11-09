#pragma once

#include <3ds/types.h>
#include <cstdlib>
#include <vector>
#include <string>

#include "string_hash.hpp"

#include <filesystem>
#include <fstream>
#include <algorithm>

class ModResourcesHashDB {
    std::vector<u32> mEntries;
    bool sorted = true;

    public:
    ModResourcesHashDB() {}
    
    // void load(const char* fp) {
    //     Core::File file(fp, FS_OPEN_READ);
    //     u32 elements = 0;
    //     if (!file.isOpen() || file.read(&elements, sizeof(u32)) != sizeof(u32) || elements == 0) return;

    //     u32 size = sizeof(u32) * elements;
    //     this->mEntries = (u32*)malloc(size);
    //     if (!this->mEntries) return;
    //     if (file.read(this->mEntries, size) != size) { // corrupted
    //         free(this->mEntries);
    //         this->mEntries = nullptr;
    //         return;
    //     }
    //     this->mSize = elements;
    // }

    // ~ModResourcesHashDB() {
    //     if (this->mEntries) free(this->mEntries);
    // }

    bool contains(u32 hash) {
        if (this->mEntries.empty()) return false;

        if (!this->sorted) {
            std::sort(this->mEntries.begin(), this->mEntries.end());
            this->sorted = true;
        }

        int low = 0;
        int high = this->mEntries.size() - 1;

        u32 target = hash;
        auto& arr = this->mEntries;

        while (low <= high) {
            int mid = low + (high - low) / 2;

            if (arr[mid] == target) {
                return true;
            }
            if (arr[mid] < target) {
                low = mid + 1;
            } else {
                high = mid - 1;
            }
        }
        return false;
    }

    bool contains(const char* id) {
        return this->contains(hash(id));
    }

    void add(const char* id) {
        this->mEntries.push_back(hash(id));
        this->sorted = false;
    }

    bool empty() const {
        return this->mEntries.empty();
    }

    bool dump(const std::string& fp) {
        namespace fs = std::filesystem;

        if (this->mEntries.empty()) return false;

        std::ofstream outFile(fp);
        if (!outFile.is_open()) return false;

        if (!this->sorted) {
            std::sort(this->mEntries.begin(), this->mEntries.end());
            this->sorted = true;
        }

        u32 size = this->mEntries.size();
        outFile.write(reinterpret_cast<char*>(&size), sizeof(u32));

        outFile.write(reinterpret_cast<char*>(this->mEntries.data()), size * sizeof(u32));
        outFile.close();
        return true;
    }
};