#pragma once

#include "Minecraft/Common/Client/Game/MinecraftGame.hpp"
#include "Minecraft/src-deps/Renderer/TextureGroup.hpp"
#include "Minecraft/src-deps/Renderer/TexturePtr.hpp"
#include "Minecraft/Common/Resources/ResourceLocation.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Debug.hpp"
#include "game/ggspgpu.h"
#include <CTRPluginFramework.hpp>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cstring>

namespace Core {
    class TextureLoader {
    public:
        #pragma pack(push, 1)
        struct Header3DST {
            char magic[4];       // "3DST"
            uint32_t mode;       // 3
            uint32_t format;     // 0 = RGBA8, etc.
            uint32_t fullWidth;  // padded width
            uint32_t fullHeight; // padded height
            uint32_t width;      // image width
            uint32_t height;     // image height
            uint32_t mipLevels;  // mip levels (usually 1)
        };
        #pragma pack(pop)

        static bool Load3DSTTexture(
            MC3DSPluginFramework::MinecraftGame* game,
            const std::string& filePath,
            const std::string& resourceName,
            MC3DSPluginFramework::TexturePtr& outTexturePtr,
            int& outWidth,
            int& outHeight,
            float& outUMax,
            float& outVMax)
        {
            if (!game || filePath.empty()) return false;
            if (!Core::Filesystem::FileExists(filePath)) return false;

            // Core::Debug::LogInfof("[TextureLoader] Loading 3DST texture from: %s", filePath.c_str());

            CTRPluginFramework::File file;
            if (CTRPluginFramework::File::Open(file, filePath, CTRPluginFramework::File::READ) != CTRPluginFramework::File::SUCCESS) {
                // Core::Debug::LogErrorf("[TextureLoader] Failed to open file: %s", filePath.c_str());
                return false;
            }

            u64 fileSize = file.GetSize();
            if (fileSize < sizeof(Header3DST)) {
                return false;
            }

            std::vector<uint8_t> fileBuffer(fileSize);
            if (file.Read(fileBuffer.data(), fileSize) != CTRPluginFramework::File::SUCCESS) {
                return false;
            }

            const Header3DST* header = reinterpret_cast<const Header3DST*>(fileBuffer.data());
            if (header->magic[0] != '3' || header->magic[1] != 'D' ||
                header->magic[2] != 'S' || header->magic[3] != 'T') {
                return false;
            }

            uint32_t fullW = header->fullWidth ? header->fullWidth : header->width;
            uint32_t fullH = header->fullHeight ? header->fullHeight : header->height;
            uint32_t imgW = header->width ? header->width : fullW;
            uint32_t imgH = header->height ? header->height : fullH;

            if (fullW == 0 || fullH == 0) return false;

            outWidth = (int)imgW;
            outHeight = (int)imgH;
            outUMax = (fullW > 0) ? ((float)imgW / (float)fullW) : 1.0f;
            outVMax = (fullH > 0) ? ((float)imgH / (float)fullH) : 1.0f;

            MC3DSPluginFramework::TextureGroup& textures = game->getTextures();
            MC3DSPluginFramework::ResourceLocation resLoc(resourceName.c_str(), 0);

            // Call TextureGroup::createTexture (0x004F4038)
            using CreateTextureFn = void* (*)(MC3DSPluginFramework::TextureGroup*, const MC3DSPluginFramework::ResourceLocation&, int, int);
            void* tex = ((CreateTextureFn)0x004F4038)(&textures, resLoc, fullW, fullH);
            if (!tex) {
                // Core::Debug::LogErrorf("[TextureLoader] createTexture failed for %s", resourceName.c_str());
                return false;
            }

            // Retrieve the CTR GPU buffer from tex->mResources[0]
            // Note: std::vector<TextureResource> mResources is at tex + 0x40 (offset 64 bytes)
            char* texBytes = reinterpret_cast<char*>(tex);
            void* resourceVector = *reinterpret_cast<void**>(texBytes + 0x40);
            if (!resourceVector) {
                // Core::Debug::LogErrorf("[TextureLoader] mResources vector begin is null for %s", resourceName.c_str());
                return false;
            }

            void* gpuBuffer = *reinterpret_cast<void**>(resourceVector);
            uint32_t gpuBufSize = *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(resourceVector) + 4);

            // Core::Debug::LogInfof("[TextureLoader] tex=%p, resourceVector=%p, gpuBuffer=%p, gpuBufSize=%u",
            //     tex, resourceVector, gpuBuffer, gpuBufSize);

            if (gpuBuffer && gpuBufSize > 0) {
                size_t payloadSize = fileSize - sizeof(Header3DST);
                size_t copyBytes = std::min<size_t>(payloadSize, (size_t)gpuBufSize);
                memcpy(gpuBuffer, fileBuffer.data() + sizeof(Header3DST), copyBytes);

                // Flush CTR data cache so GPU sees updated texture pixels
                gGSPGPU_FlushDataCache(gpuBuffer, copyBytes);
            }

            // Mark texture as static native CTR format (offset 0x99 = 153 is mIsDynamic/mNeedsConversion)
            // By setting byte 153 to 0, 0x1B32C8 will bind TextureResource[0] directly without
            // calling ConvertTextureResourceToNativeFormat (which would double-swizzle the already-tiled .3dst data).
            texBytes[153] = 0;

            // Finalize/initialize the CTR native Texture2D GPU resource
            using FinalizeTextureFn = void (*)(void*);
            ((FinalizeTextureFn)0x001B32C8)(tex);

            outTexturePtr = MC3DSPluginFramework::TexturePtr(textures, resLoc);
            // Core::Debug::LogInfof("[TextureLoader] Successfully loaded 3DST texture: %s (img %dx%d, full %dx%d, uMax=%.2f, vMax=%.2f, loaded=%d)",
            //     filePath.c_str(), (int)imgW, (int)imgH, (int)fullW, (int)fullH, outUMax, outVMax, (int)texBytes[52]);
            return true;
        }

        static bool UnloadTexture(
            MC3DSPluginFramework::MinecraftGame* game,
            const std::string& resourceName)
        {
            if (!game || resourceName.empty()) return false;

            MC3DSPluginFramework::TextureGroup& textures = game->getTextures();
            MC3DSPluginFramework::ResourceLocation resLoc(resourceName.c_str(), 0);

            // Call TextureGroup::softUnloadTexture (0x004F42CC)
            // This frees Texture2D native GPU memory (0x004F042C) and the TextureResource buffer (0x004FC000),
            // resetting mIsLoaded to 0 while leaving the map node reusable for future textures.
            using SoftUnloadFn = void (*)(MC3DSPluginFramework::TextureGroup*, const MC3DSPluginFramework::ResourceLocation&);
            ((SoftUnloadFn)0x004F42CC)(&textures, resLoc);

            // Core::Debug::LogInfof("[TextureLoader] Unloaded texture: %s", resourceName.c_str());
            return true;
        }
    };
}
