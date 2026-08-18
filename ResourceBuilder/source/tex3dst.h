#pragma once

#ifndef __cplusplus
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#endif

#ifdef __cplusplus
#include <string>
#include <cstdint>
#include <cstdlib>
#endif

#define TEX3DST_RGBA8 0
#define TEX3DST_RGB8 1
#define TEX3DST_RGBA5551 2
#define TEX3DST_RGB565 3
#define TEX3DST_RGBA4 4
#define TEX3DST_LA8 5
#define TEX3DST_hilo8 6
#define TEX3DST_L8 7
#define TEX3DST_A8 8
#define TEX3DST_LA4 9

typedef struct _formatInfo {
    uint32_t id;
    bool supported;
    unsigned int pixel_size;
    unsigned int pixel_channels;
} Format3dstInfo;

typedef struct _pixelData {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
    uint8_t l;
} PixelData;

struct _size {
    uint32_t width;
    uint32_t height;
};

typedef struct _headerTexture3dst {
    uint32_t mode;
    uint32_t format;
    struct _size full_size;
    struct _size size;
    uint32_t mip_level;
} _HeaderTexture3dst;

typedef struct tex3dst{
    _HeaderTexture3dst header;
    struct _size size;
    uint8_t **textureData;
} tex3dst;

typedef void *Texture3dst_c;

#ifdef __cplusplus

class Texture3dst {
    public:
        _HeaderTexture3dst header;
        struct _size size;
        uint8_t *textureData = NULL;

        enum Tex3dstState {
            SUCCESS,
            NOLOADED,
            FILEERROR,
            NOSIG,
            UNSUPPORTEDMODE,
            UNSUPPORTEDFORMAT,
            INVALIDSIZE,
            INVALIDMIPLEVEL,
            MEMORYERROR,
            UNEXPECTEDEOF,
            READERROR,
            INVALIDPOSITION,
            OTHER
        };

        Texture3dst::Tex3dstState texstate = Texture3dst::Tex3dstState::NOLOADED;
        Texture3dst::Tex3dstState opstate = Texture3dst::Tex3dstState::SUCCESS;

        enum Tex3dstFormats {
            RGBA8 = 0,
            RGB8 = 1,
            RGBA5551 = 2,
            RGB565 = 3,
            RGBA4 = 4,
            LA8 = 5,
            HILO8 = 6,
            L8 = 7,
            A8 = 8,
            LA4 = 9
        };

        Texture3dst() {};
        ~Texture3dst() {
            if (this->textureData)
            {
                free(this->textureData);
                this->textureData = NULL;
            }
        }

        void open(const std::string &path);

        void create(uint32_t width, uint32_t height, uint32_t mip_level, Texture3dst::Tex3dstFormats format);

        void flipVertical();

        void setPixel(uint32_t x, uint32_t y, PixelData *pixel_data);

        void getPixel(uint32_t x, uint32_t y, PixelData *pixel_data);

        void crop(Texture3dst &dst, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2);

        bool paste(uint32_t x, uint32_t y, Texture3dst &tex2);

        void getFormatInfo(Format3dstInfo *dst);

        bool compare(Texture3dst &tex2, bool ignore_alpha);

        void save(const std::string &path);
    
    private:
        void _formatPixelData(uint8_t *out_data, Format3dstInfo *fmtInfo);

        void _processMipLevels(uint8_t *texdata, uint32_t texwidth, uint32_t texheight, Format3dstInfo *fmtInfo, uint8_t mip_level);
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

Texture3dst_c Tex3DSTOpen(const char *path);

Texture3dst_c Tex3DSTCreate(uint32_t width, uint32_t height, uint32_t mip_level, uint32_t format);

void Tex3DSTFlipVertical(Texture3dst_c obj);

void Tex3DSTSetPixel(Texture3dst_c obj, uint32_t x, uint32_t y, PixelData *pixel_data);

void Tex3DSTGetPixel(Texture3dst_c obj, uint32_t x, uint32_t y, PixelData *pixel_data);

Texture3dst_c Tex3DSTCrop(Texture3dst_c obj, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2);

bool Tex3DSTPaste(Texture3dst_c obj, Texture3dst_c tex2, uint32_t x, uint32_t y);

void Tex3DSTGetFormatInfo(Texture3dst_c obj, Format3dstInfo *dst);

bool Tex3DSTCompare(Texture3dst_c obj, Texture3dst_c tex2, bool ignore_alpha);

void Tex3DSTSave(Texture3dst_c obj, const char *path);

void Tex3DSTFree(Texture3dst_c obj);

#ifdef __cplusplus
}
#endif