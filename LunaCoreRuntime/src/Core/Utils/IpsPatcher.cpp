#include "Core/Utils/IpsPatcher.hpp"
#include "Core/Filesystem.hpp"
#include <CTRPluginFramework.hpp>
#include <vector>
#include <cstring>
#include <csvc.h>

namespace Core::Utils {

    bool ApplyIpsPatch(const std::string& ipsPath, u32 baseOffset) {
        if (ipsPath.empty())
            return false;

        std::vector<u8> buffer;

        Core::File coreFile(ipsPath, FS_OPEN_READ);
        if (coreFile.isOpen()) {
            int fileSize = coreFile.getSize();
            if (fileSize >= 8 && fileSize <= 0x10000000) {
                buffer.resize(static_cast<size_t>(fileSize));
                coreFile.rewind();
                if (coreFile.read(buffer.data(), fileSize) != fileSize) {
                    buffer.clear();
                }
            }
            coreFile.close();
        }

        if (buffer.size() < 8)
            return false;

        // Verify "PATCH" header
        if (std::memcmp(buffer.data(), "PATCH", 5) != 0)
            return false;

        size_t cursor = 5;
        bool eofFound = false;

        while (cursor + 3 <= buffer.size()) {
            // Check for "EOF" marker
            if (buffer[cursor] == 'E' && buffer[cursor + 1] == 'O' && buffer[cursor + 2] == 'F') {
                eofFound = true;
                cursor += 3;
                break;
            }

            // 3-byte offset (big-endian)
            u32 offset = (static_cast<u32>(buffer[cursor]) << 16) |
                         (static_cast<u32>(buffer[cursor + 1]) << 8) |
                         static_cast<u32>(buffer[cursor + 2]);
            cursor += 3;

            if (cursor + 2 > buffer.size())
                return false;

            // 2-byte size (big-endian)
            u16 size = (static_cast<u16>(buffer[cursor]) << 8) |
                        static_cast<u16>(buffer[cursor + 1]);
            cursor += 2;

            u32 targetAddr = baseOffset + offset;

            if (size > 0) {
                // Normal record
                if (cursor + size > buffer.size())
                    return false;

                void* patchData = reinterpret_cast<void*>(const_cast<u8*>(&buffer[cursor]));
                if (!CTRPluginFramework::Process::Patch(targetAddr, patchData, size))
                    return false;

                cursor += size;
            } else {
                // RLE record
                if (cursor + 3 > buffer.size())
                    return false;

                u16 rleCount = (static_cast<u16>(buffer[cursor]) << 8) |
                                static_cast<u16>(buffer[cursor + 1]);
                cursor += 2;
                u8 rleVal = buffer[cursor];
                cursor += 1;

                if (rleCount > 0) {
                    std::vector<u8> rleData(rleCount, rleVal);
                    if (!CTRPluginFramework::Process::Patch(targetAddr, rleData.data(), rleCount))
                        return false;
                }
            }
        }

        if (!eofFound)
            return false;

        // Ensure CPU instruction and data caches are flushed and synchronized
        svcInvalidateEntireInstructionCache();
        svcFlushEntireDataCache();

        return true;
    }

}
