#pragma once

#include <string>
#include <3ds.h>

namespace Core::Utils {
    /**
     * @brief Loads and applies an IPS patch file into game process memory.
     * 
     * @param ipsPath Path to the .ips patch file.
     * @param baseOffset Base memory address offset (default 0x00100000).
     * @return true if the patch was successfully loaded and applied, false otherwise.
     */
    bool ApplyIpsPatch(const std::string& ipsPath, u32 baseOffset = 0x00100000);
}
