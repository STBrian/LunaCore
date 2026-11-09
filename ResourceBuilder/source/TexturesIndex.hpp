#pragma once

#include <3ds/types.h>

struct TexturesIndex_header {
    u32 entries;
};

enum TextureSource : u32 {
    TEXSOURCE_MOD,
    TEXSOURCE_RESOURCEPACK,
};

struct TexturesIndex_entry {
    u32 textureHashId; // hash("textures/items/name")
    
    union {
        u32 asU32;
        char* asPtr;
    } texturePathOffset;
    char* texturePath;
    
    TextureSource source;
};