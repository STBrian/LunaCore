#include "tex3dst.h"

#include <algorithm>
#include <cctype>

#include <cstdio>
#include <cstring>
#include <cmath>
#include <unistd.h>

void _read3dstHeader(FILE *file, _HeaderTexture3dst *headerDst)
{
    fread(&headerDst->mode, sizeof(uint32_t), 1, file);
    fread(&headerDst->format, sizeof(uint32_t), 1, file);
    fread(&headerDst->full_size.width, sizeof(uint32_t), 1, file);
    fread(&headerDst->full_size.height, sizeof(uint32_t), 1, file);
    fread(&headerDst->size.width, sizeof(uint32_t), 1, file);
    fread(&headerDst->size.height, sizeof(uint32_t), 1, file);
    fread(&headerDst->mip_level, sizeof(uint32_t), 1, file);
}

bool _isMipLevelValid(size_t width, size_t height, uint32_t mip_level)
{
    uint32_t num1 = (int)log2(width);
    uint32_t num2 = (int)log2(height);
    bool res = (mip_level <= num1 && mip_level <= num2);
    return res;
}

bool _isPowerOfTwo(uint32_t num)
{
    return (num & (num - 1)) == 0;
}

uint32_t _getClosestPowerOfTwo(uint32_t num)
{
    uint32_t min = 1;
    while (min < num)
        min *= 2;
    return min;
}

uint32_t _getTextureLinearPosition(uint32_t x, uint32_t y, uint32_t width)
{
    uint32_t dst_pos = ((((y >> 3) * (width >> 3) + (x >> 3)) << 6) + ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3)));
    return dst_pos;
}

bool _isFormatSupported(uint32_t format)
{
    bool valid = false;
    bool supported[] = {1, 1, 1, 1, 1, 1, 0, 0, 0, 1};
    if (format <= 9)
    {
        if (supported[format])
            valid = true;
    }
    return valid;
}

unsigned int _getFormatPixelSize(uint32_t format)
{
    unsigned int res = 0;
    unsigned int pixel_size[] = {4, 3, 2, 2, 2, 2, 2, 1, 1, 1};
    if (format <= 9)
    {
        res = pixel_size[format];
    }
    return res;
}

unsigned int _getFormatPixelChannels(uint32_t format)
{
    unsigned int res = 0;
    unsigned int pixel_channels[] = {4, 3, 4, 3, 4, 2, 2, 1, 1, 2};
    if (format <= 9)
    {
        res = pixel_channels[format];
    }
    return res;
}

void _getFormatInfo(Format3dstInfo *dst, uint32_t format)
{
    dst->id = format;
    dst->supported = _isFormatSupported(format);
    dst->pixel_size = _getFormatPixelSize(format);
    dst->pixel_channels = _getFormatPixelChannels(format);
}

uint8_t *_createPixelDataStructure(size_t width, size_t heigth, unsigned int length)
{
    uint8_t *data_struct = (uint8_t *)calloc(heigth * width * length, sizeof(uint8_t));
    return data_struct;
}

uint32_t _maxIntBits(unsigned int n)
{
    return (pow(2, n) - 1);
}

uint32_t _convertPixelDataToBytes(uint32_t format, PixelData *pixel_data)
{
    PixelData new_pixel_data;
    uint32_t combined;
    switch (format)
    {
        case 0:
            combined = (pixel_data->r << 24) | (pixel_data->g << 16) | (pixel_data->b << 8) | pixel_data->a;
            break;
        case 1:
            combined = (pixel_data->r << 16) | (pixel_data->g << 8) | pixel_data->b;
            break;
        case 2:
            new_pixel_data.r = (uint8_t)((float)pixel_data->r / 0xFF * _maxIntBits(5));
            new_pixel_data.g = (uint8_t)((float)pixel_data->g / 0xFF * _maxIntBits(5));
            new_pixel_data.b = (uint8_t)((float)pixel_data->b / 0xFF * _maxIntBits(5));
            new_pixel_data.a = (uint8_t)((float)pixel_data->a > 127);
            combined = (new_pixel_data.r << 11) | (new_pixel_data.g << 6) | (new_pixel_data.b << 1) | new_pixel_data.a;
            break;
        case 3:
            new_pixel_data.r = (uint8_t)((float)pixel_data->r / 0xFF * _maxIntBits(5));
            new_pixel_data.g = (uint8_t)((float)pixel_data->g / 0xFF * _maxIntBits(6));
            new_pixel_data.b = (uint8_t)((float)pixel_data->b / 0xFF * _maxIntBits(5));
            combined = (new_pixel_data.r << 11) | (new_pixel_data.g << 5) | new_pixel_data.b;
            break;
        case 4:
            new_pixel_data.r = (uint8_t)((float)pixel_data->r / 0xFF * 0xF);
            new_pixel_data.g = (uint8_t)((float)pixel_data->g / 0xFF * 0xF);
            new_pixel_data.b = (uint8_t)((float)pixel_data->b / 0xFF * 0xF);
            new_pixel_data.a = (uint8_t)((float)pixel_data->a / 0xFF * 0xF);
            combined = (new_pixel_data.r << 11) | (new_pixel_data.g << 6) | (new_pixel_data.b << 1) | new_pixel_data.a;
            break;
        case 5:
            combined = (pixel_data->l << 8) | pixel_data->a;
            break;
        case 9:
            new_pixel_data.l = (uint8_t)((float)pixel_data->l / 0xFF * 0xF);
            new_pixel_data.a = (uint8_t)((float)pixel_data->a / 0xFF * 0xF);
            combined = (new_pixel_data.l << 4) | new_pixel_data.a;
            break;
        default:
            combined = 0;
            break;
    }
    return combined;
}

void _convertBytesToPixelData(uint32_t format, uint32_t bytes, PixelData *dst_pixel_data)
{
    *dst_pixel_data = (PixelData){0};
    switch (format)
    {
        case 0:
            dst_pixel_data->r = (bytes >> 24) & 0xFF;
            dst_pixel_data->g = (bytes >> 16) & 0xFF;
            dst_pixel_data->b = (bytes >> 8) & 0xFF;
            dst_pixel_data->a = bytes & 0xFF;
            break;
        case 1:
            dst_pixel_data->r = (bytes >> 16) & 0xFF;
            dst_pixel_data->g = (bytes >> 8) & 0xFF;
            dst_pixel_data->b = bytes & 0xFF;
            break;
        case 2:
            dst_pixel_data->r = (float)((bytes >> 11) & 0b11111) / _maxIntBits(5) * 0xFF;
            dst_pixel_data->g = (float)((bytes >> 6) & 0b11111) / _maxIntBits(5) * 0xFF;
            dst_pixel_data->b = (float)((bytes >> 1) & 0b11111) / _maxIntBits(5) * 0xFF;
            dst_pixel_data->a = (float)(bytes & 0b1) * 0xFF;
            break;
        case 3:
            dst_pixel_data->r = (float)((bytes >> 11) & 0b11111) / _maxIntBits(5) * 0xFF;
            dst_pixel_data->g = (float)((bytes >> 5) & 0b111111) / _maxIntBits(6) * 0xFF;
            dst_pixel_data->b = (float)(bytes & 0b11111) / _maxIntBits(5) * 0xFF;
            break;
        case 4:
            dst_pixel_data->r = (float)((bytes >> 12) & 0xF) / 0xF * 0xFF;
            dst_pixel_data->g = (float)((bytes >> 8) & 0xF) / 0xF * 0xFF;
            dst_pixel_data->b = (float)((bytes >> 4) & 0xF) / 0xF * 0xFF;
            dst_pixel_data->a = (float)(bytes & 0xF) / 0xF * 0xFF;
            break;
        case 5:
            dst_pixel_data->l = (bytes >> 8) & 0xFF;
            dst_pixel_data->a = bytes & 0xFF;
            break;
        case 9:
            dst_pixel_data->l = (float)((bytes >> 4) & 0xF) / 0xF * 0xFF;
            dst_pixel_data->a = (float)(bytes & 0xF) / 0xF * 0xFF;
            break;
        default:
            dst_pixel_data->a = 0;
            break;
    }
}

void _resizeHalf(uint8_t *outTex, uint8_t *tex, uint32_t texWidth, uint32_t texHeight, Format3dstInfo *fmtInfo) {
    uint32_t resWidth = texWidth >> 1;
    uint32_t resHeight = texHeight >> 1;

    uint32_t format = fmtInfo->id;
    uint32_t pixel_size = fmtInfo->pixel_size;
    uint32_t ogTexLineSize = texWidth * pixel_size;
    uint32_t resTexLineSize = resWidth * pixel_size;

    PixelData pixel1, pixel2, pixel3, pixel4, newInterpolation;
    uint32_t pixelRawData = 0;
    uint32_t newPixelRawData = 0;

    for (uint32_t i = 0; i < resHeight; i++) 
    {
        for (uint32_t j = 0; j < resWidth; j++) 
        {
            memcpy(&pixelRawData, &tex[i * 2 * ogTexLineSize + j * 2 * pixel_size], pixel_size);
            _convertBytesToPixelData(format, pixelRawData, &pixel1);
            memcpy(&pixelRawData, &tex[i * 2 * ogTexLineSize + ((j*2)+1) * pixel_size], pixel_size);
            _convertBytesToPixelData(format, pixelRawData, &pixel2);
            memcpy(&pixelRawData, &tex[((i*2)+1) * ogTexLineSize + j * 2 * pixel_size], pixel_size);
            _convertBytesToPixelData(format, pixelRawData, &pixel3);
            memcpy(&pixelRawData, &tex[((i*2)+1) * ogTexLineSize + ((j*2)+1) * pixel_size], pixel_size);
            _convertBytesToPixelData(format, pixelRawData, &pixel4);
            
            newInterpolation.r = (pixel1.r + pixel2.r + pixel3.r + pixel4.r) / 4;
            newInterpolation.g = (pixel1.g + pixel2.g + pixel3.g + pixel4.g) / 4;
            newInterpolation.b = (pixel1.b + pixel2.b + pixel3.b + pixel4.b) / 4;
            newInterpolation.a = (pixel1.a + pixel2.a + pixel3.a + pixel4.a) / 4;
            newInterpolation.l = (pixel1.l + pixel2.l + pixel3.l + pixel4.l) / 4;

            newPixelRawData = _convertPixelDataToBytes(format, &newInterpolation);

            memcpy(&outTex[i * resTexLineSize + j * pixel_size], &newPixelRawData, pixel_size);
        }
    }
}

void Texture3dst::open(const std::string &path) {
    if (this->texstate != Texture3dst::Tex3dstState::NOLOADED)
        return;

    FILE *textureFileBuffer = fopen(path.c_str(), "rb");
    if (!textureFileBuffer)
    {
        this->texstate = Texture3dst::Tex3dstState::FILEERROR;
        return;
    }

    char sig[5];
    fread(sig, 1, 4, textureFileBuffer);
    sig[4] = '\0';
    if (strcmp(sig, "3DST") != 0)
    {
        this->texstate = Texture3dst::Tex3dstState::NOSIG;
        fclose(textureFileBuffer);
        return;
    }

    _read3dstHeader(textureFileBuffer, &this->header);

    if (this->header.mode != 3)
    {
        this->texstate = Texture3dst::Tex3dstState::UNSUPPORTEDMODE;
        fclose(textureFileBuffer);
        return;
    }

    Format3dstInfo fmtInfo;
    _getFormatInfo(&fmtInfo, this->header.format);
    if (!fmtInfo.supported)
    {
        this->texstate = Texture3dst::Tex3dstState::UNSUPPORTEDFORMAT;
        fclose(textureFileBuffer);
        return;
    }

    uint32_t full_width = this->header.full_size.width;
    uint32_t full_height = this->header.full_size.height;
    if (!_isPowerOfTwo(full_width) || !_isPowerOfTwo(full_height))
    {
        this->texstate = Texture3dst::Tex3dstState::INVALIDSIZE;
        fclose(textureFileBuffer);
        return;
    }

    uint32_t mip_level = this->header.mip_level;
    if (mip_level == 0 || !_isMipLevelValid(full_width, full_height, mip_level))
    {
        this->texstate = Texture3dst::Tex3dstState::INVALIDMIPLEVEL;
        fclose(textureFileBuffer);
        return;
    }

    this->size.width = this->header.size.width;
    this->size.height = this->header.size.height;

    uint8_t *unarranged_texture_data = _createPixelDataStructure(full_width, full_height, fmtInfo.pixel_size);
    if (!unarranged_texture_data)
    {
        this->texstate = Texture3dst::Tex3dstState::MEMORYERROR;
        fclose(textureFileBuffer);
        return;
    }

    uint32_t pixel_line_size = full_width * fmtInfo.pixel_size;
    uint32_t pixel_size = fmtInfo.pixel_size;
    for (uint32_t i = 0; i < full_height; i++)
    {
        for (uint32_t j = 0; j < full_width; j++)
        {
            size_t readBytes = fread(&unarranged_texture_data[i * pixel_line_size + j * pixel_size], pixel_size, 1, textureFileBuffer);
            if (!readBytes)
            {
                size_t currPos = ftell(textureFileBuffer);
                fseek(textureFileBuffer, 0, SEEK_END);
                size_t eof = ftell(textureFileBuffer);
                if (currPos == eof)
                    this->texstate = Texture3dst::Tex3dstState::UNEXPECTEDEOF;
                else
                    this->texstate = Texture3dst::Tex3dstState::READERROR;
                fclose(textureFileBuffer);
                free(unarranged_texture_data);
                return;
            }
        }
    }

    fclose(textureFileBuffer);

    this->textureData = _createPixelDataStructure(full_width, full_height, fmtInfo.pixel_size);
    if (!this->textureData)
    {
        this->texstate = Texture3dst::Tex3dstState::MEMORYERROR;
        free(unarranged_texture_data);
        return;
    }

    uint32_t dst_pos;
    for (uint32_t i = 0; i < full_height; i++)
    {
        for (uint32_t j = 0; j < full_width; j++)
        {
            dst_pos = _getTextureLinearPosition(j, i, full_width) * pixel_size;
            memcpy(&this->textureData[i * pixel_line_size + j * pixel_size], &unarranged_texture_data[dst_pos], pixel_size);
        }
    }
    free(unarranged_texture_data);

    this->texstate = Texture3dst::Tex3dstState::SUCCESS;
    this->flipVertical();
}

void Texture3dst::create(uint32_t width, uint32_t height, uint32_t mip_level, Texture3dst::Tex3dstFormats format)
{
    if (this->texstate != Texture3dst::Tex3dstState::NOLOADED)
        return;

    if (width <= 0 || height <= 0)
    {
        this->texstate = Texture3dst::Tex3dstState::INVALIDSIZE;
        return;
    }

    uint32_t full_width = _getClosestPowerOfTwo(width);
    uint32_t full_height = _getClosestPowerOfTwo(height);

    if (mip_level <= 0 || !_isMipLevelValid(full_width, full_height, mip_level))
    {
        this->texstate = Texture3dst::Tex3dstState::INVALIDMIPLEVEL;
        return;
    }

    Format3dstInfo fmtInfo;
    _getFormatInfo(&fmtInfo, format);
    if (!fmtInfo.supported)
    {
        this->texstate = Texture3dst::Tex3dstState::UNSUPPORTEDFORMAT;
        return;
    }

    this->header.mode = 3;
    this->header.format = format;
    this->header.full_size.width = full_width;
    this->header.full_size.height = full_height;
    this->header.size.width = width;
    this->header.size.height = height;
    this->header.mip_level = mip_level;

    this->size.width = width;
    this->size.height = height;

    this->textureData = _createPixelDataStructure(full_width, full_height, fmtInfo.pixel_size);
    this->texstate = Texture3dst::Tex3dstState::SUCCESS;
}

void Texture3dst::flipVertical() {
    if (this->texstate != Texture3dst::Tex3dstState::SUCCESS)
        return;

    Format3dstInfo fmtInfo;
    _getFormatInfo(&fmtInfo, this->header.format);
    uint32_t full_width = this->header.full_size.width;
    uint32_t full_height = this->header.full_size.height;

    uint8_t *newTextureData = _createPixelDataStructure(full_width, full_height, fmtInfo.pixel_size);
    if (!newTextureData)
    {
        this->opstate = Texture3dst::Tex3dstState::MEMORYERROR;
        return;
    }
    uint32_t pixel_line_size = full_width * fmtInfo.pixel_size;
    uint32_t pixel_size = fmtInfo.pixel_size;

    for (uint32_t i = 0; i < full_height; i++)
    {
        for (uint32_t j = 0; j < full_width; j++)
            memcpy(&newTextureData[(full_height-1-i) * pixel_line_size + j * pixel_size], &this->textureData[i * pixel_line_size + j * pixel_size], pixel_size);
    }
    free(this->textureData);
    this->textureData = newTextureData;
}

void Texture3dst::setPixel(uint32_t x, uint32_t y, PixelData *pixel_data)
{
    if (this->texstate != Texture3dst::Tex3dstState::SUCCESS)
        return;
    if (x >= this->size.width || y >= this->size.height)
    {
        this->opstate = Texture3dst::Tex3dstState::INVALIDPOSITION;
        return;
    }
    
    Format3dstInfo fmtInfo;
    _getFormatInfo(&fmtInfo, this->header.format);
    uint32_t full_width = this->header.full_size.width;
    uint32_t pixel_data_bytes = _convertPixelDataToBytes(this->header.format, pixel_data);
    uint32_t pixel_line_size = full_width * fmtInfo.pixel_size;
    uint32_t pixel_size = fmtInfo.pixel_size;
    memcpy(&this->textureData[y * pixel_line_size + x * pixel_size], &pixel_data_bytes, pixel_size);
    this->opstate = Texture3dst::Tex3dstState::SUCCESS;
}

void Texture3dst::getPixel(uint32_t x, uint32_t y, PixelData *pixel_data)
{
    if (this->texstate != Texture3dst::Tex3dstState::SUCCESS)
        return;
    if (x >= this->size.width || y >= this->size.height)
    {
        this->opstate = Texture3dst::Tex3dstState::INVALIDPOSITION;
        return;
    }
    
    Format3dstInfo fmtInfo;
    _getFormatInfo(&fmtInfo, this->header.format);
    uint32_t full_width = this->header.full_size.width;
    uint32_t pixel_data_bytes = 0;
    uint32_t pixel_line_size = full_width * fmtInfo.pixel_size;
    uint32_t pixel_size = fmtInfo.pixel_size;
    memcpy(&pixel_data_bytes, &this->textureData[y * pixel_line_size + x * pixel_size], pixel_size);
    _convertBytesToPixelData(this->header.format, pixel_data_bytes, pixel_data);
    this->opstate = Texture3dst::Tex3dstState::SUCCESS;
}

void Texture3dst::crop(Texture3dst &dst, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2)
{
    if (this->texstate != Texture3dst::Tex3dstState::SUCCESS)
        return;

    if (x1 >= this->size.width || x2 > this->size.width || x1 >= x2 || y1 >= this->size.height || y2 > this->size.height || y1 >= y2)
    {
        this->opstate = Texture3dst::Tex3dstState::INVALIDPOSITION;
        dst.opstate = Texture3dst::Tex3dstState::INVALIDPOSITION;
        return;
    }

    dst.create(x2 - x1, y2 - y1, 1, static_cast<Texture3dst::Tex3dstFormats>(this->header.format));
    if (dst.texstate != Texture3dst::Tex3dstState::SUCCESS)
    {
        this->opstate = Texture3dst::Tex3dstState::OTHER;
        return;
    }

    PixelData pixel_data;
    for (uint32_t i = y1; i < y2; i++)
    {
        for (uint32_t j = x1; j < x2; j++)
        {
            this->getPixel(j, i, &pixel_data);
            dst.setPixel(j - x1, i - y1, &pixel_data);
        }
    }
    this->opstate = Texture3dst::Tex3dstState::SUCCESS;
    return;
}

bool Texture3dst::paste(uint32_t x, uint32_t y, Texture3dst &tex2)
{
    if (this->texstate != Texture3dst::Tex3dstState::SUCCESS || tex2.texstate != Texture3dst::Tex3dstState::SUCCESS)
        return false;

    uint32_t this_width = this->size.width;
    uint32_t this_height = this->size.height;
    uint32_t tex_width = tex2.size.width;
    uint32_t tex_height = tex2.size.height;
    if (x >= this_width || y >= this_height || tex_width + x > this_width || tex_height + y > this_height)
    {
        this->opstate = Texture3dst::Tex3dstState::INVALIDPOSITION;
        return false;
    }

    PixelData pixel_data;
    for (uint32_t i = 0; i < tex_height; i++)
    {
        for (uint32_t j = 0; j < tex_width; j++)
        {
            tex2.getPixel(j, i, &pixel_data);
            this->setPixel(x + j, y + i, &pixel_data);
        }
    }
    this->opstate = Texture3dst::Tex3dstState::SUCCESS;
    return true;
}

void Texture3dst::getFormatInfo(Format3dstInfo *dst)
{
    _getFormatInfo(dst, this->header.format);
    this->opstate = Texture3dst::Tex3dstState::SUCCESS;
}

bool Texture3dst::compare(Texture3dst &tex2, bool ignore_alpha)
{
    if (this->texstate != Texture3dst::Tex3dstState::SUCCESS || tex2.texstate != Texture3dst::Tex3dstState::SUCCESS)
        return false;
    
    if (this->header.format != tex2.header.format)
        return false;

    if (this->size.width != tex2.size.width || this->size.height != tex2.size.height)
        return false;

    PixelData pixel_data1, pixel_data2;
    for (uint32_t i = 0; i < this->size.height; i++)
    {
        for (uint32_t j = 0; j < this->size.width; j++)
        {
            this->getPixel(j, i, &pixel_data1);
            tex2.getPixel(j, i, &pixel_data2);
            switch (this->header.format)
            {
                case 0: case 2: case 4:
                    if (pixel_data1.r != pixel_data2.r || pixel_data1.g != pixel_data2.g || pixel_data1.b != pixel_data2.b || pixel_data1.a != pixel_data2.a)
                    {
                        if (ignore_alpha)
                        {
                            if (pixel_data1.a != 0 || pixel_data2.a != 0)
                                return false;
                        }
                        else
                            return false;
                    }
                    break;
                case 1: case 3: 
                    if (pixel_data1.r != pixel_data2.r || pixel_data1.g != pixel_data2.g || pixel_data1.b != pixel_data2.b)
                        return false;
                    break;
                case 5: case 9:
                    if (pixel_data1.l != pixel_data2.l || pixel_data1.a != pixel_data2.a)
                    {
                        if (ignore_alpha)
                        {
                            if (pixel_data1.a != 0 || pixel_data2.a != 0)
                                return false;
                        }
                        else
                            return false;
                    }
                    break;
                default:
                    break;
            }
        }
    }
    this->opstate = Texture3dst::Tex3dstState::SUCCESS;
    return true;
}


void Texture3dst::_formatPixelData(uint8_t *out_data, Format3dstInfo *fmtInfo)
{
    uint32_t full_width = this->header.full_size.width;
    uint32_t full_height = this->header.full_size.height;
    uint32_t pixel_line_size = full_width * fmtInfo->pixel_size;
    uint32_t pixel_size = fmtInfo->pixel_size;

    if (this->header.mip_level > 1)
    {
        memcpy(out_data, this->textureData, full_width * full_height * pixel_size);
        this->_processMipLevels(out_data, this->header.full_size.width, this->header.full_size.height, fmtInfo, 2);
    }

    this->flipVertical();

    uint32_t dst_pos;
    for (uint32_t i = 0; i < full_height; i++)
    {
        for (uint32_t j = 0; j < full_width; j++)
        {
            dst_pos = _getTextureLinearPosition(j, i, full_width) * pixel_size;
            memcpy(&out_data[dst_pos], &this->textureData[i * pixel_line_size + j * pixel_size], pixel_size);
        }
    }

    this->flipVertical();
    this->opstate = Texture3dst::Tex3dstState::SUCCESS;
}

void Texture3dst::_processMipLevels(uint8_t *texdata, uint32_t texwidth, uint32_t texheight, Format3dstInfo *fmtInfo, uint8_t mip_level)
{
    uint32_t curr_width = texwidth >> 1;
    uint32_t curr_height = texheight >> 1;
    uint32_t pixel_size = fmtInfo->pixel_size;
    uint32_t curr_pixel_line_size = curr_width * pixel_size;

    uint8_t *dst_tex = texdata + texwidth * texheight * pixel_size;
    _resizeHalf(dst_tex, texdata, texwidth, texheight, fmtInfo);
    
    if (mip_level < this->header.mip_level)
        this->_processMipLevels(dst_tex, curr_width, curr_height, fmtInfo, mip_level + 1);

    uint8_t *flipped_texture = _createPixelDataStructure(curr_width, curr_height, pixel_size);
    if (!flipped_texture)
        return;

    for (uint32_t i = 0; i < curr_height; i++)
    {
        for (uint32_t j = 0; j < curr_width; j++)
            memcpy(&flipped_texture[(curr_height-1-i) * curr_pixel_line_size + j * pixel_size], &dst_tex[i * curr_pixel_line_size + j * pixel_size], pixel_size);
    }
    
    uint8_t *linear_pixel_data = flipped_texture;
    uint32_t dst_pos;
    for (uint32_t i = 0; i < curr_height; i++)
    {
        for (uint32_t j = 0; j < curr_width; j++)
        {
            dst_pos = _getTextureLinearPosition(j, i, curr_width) * pixel_size;
            memcpy(&dst_tex[dst_pos], &linear_pixel_data[i * curr_pixel_line_size + j * pixel_size], pixel_size);
        }
    }
    free(flipped_texture);
}

void Texture3dst::save(const std::string &path)
{
    if (this->texstate != Texture3dst::Tex3dstState::SUCCESS)
        return;

    FILE *fileBuffer = fopen(path.c_str(), "wb");
    if (!fileBuffer)
    {
        this->opstate = Texture3dst::Tex3dstState::FILEERROR;
        return;
    }

    Format3dstInfo fmtInfo;
    _getFormatInfo(&fmtInfo, this->header.format);
    uint32_t full_width = this->header.full_size.width;
    uint32_t full_height = this->header.full_size.height;
    uint32_t total_size = 0;
    for (uint32_t i = 0; i < this->header.mip_level; i++)
    {
        uint32_t mip_width = std::max(static_cast<uint32_t>(1), full_width >> i);
        uint32_t mip_height = std::max(static_cast<uint32_t>(1), full_height >> i);
        total_size += mip_width * mip_height * fmtInfo.pixel_size;
    }
    uint8_t *outData = (uint8_t*)calloc(total_size, sizeof(uint8_t));
    if (!outData)
    {
        this->opstate = Texture3dst::Tex3dstState::MEMORYERROR;
        fclose(fileBuffer);
        return;
    }

    this->_formatPixelData(outData, &fmtInfo);

    fwrite("3DST", 1, 4, fileBuffer);
    fwrite(&this->header.mode, sizeof(uint32_t), 1, fileBuffer);
    fwrite(&this->header.format, sizeof(uint32_t), 1, fileBuffer);
    fwrite(&full_width, sizeof(uint32_t), 1, fileBuffer);
    fwrite(&full_height, sizeof(uint32_t), 1, fileBuffer);
    fwrite(&this->size.width, sizeof(uint32_t), 1, fileBuffer);
    fwrite(&this->size.height, sizeof(uint32_t), 1, fileBuffer);
    fwrite(&this->header.mip_level, sizeof(uint32_t), 1, fileBuffer);

    fwrite(outData, 1, total_size, fileBuffer);

    free(outData);
    fclose(fileBuffer);
    this->opstate = Texture3dst::Tex3dstState::SUCCESS;
}

Texture3dst_c Tex3DSTOpen(const char *path)
{
    Texture3dst *texture = new Texture3dst();
    texture->open(path);
    return texture;
}

Texture3dst_c Tex3DSTCreate(uint32_t width, uint32_t height, uint32_t mip_level, uint32_t format)
{
    Texture3dst *texture = new Texture3dst();
    texture->create(width, height, mip_level, static_cast<Texture3dst::Tex3dstFormats>(format));
    return texture;
}

void Tex3DSTFlipVertical(Texture3dst_c obj)
{
    static_cast<Texture3dst*>(obj)->flipVertical();
}

void Tex3DSTSetPixel(Texture3dst_c obj, uint32_t x, uint32_t y, PixelData *pixel_data)
{
    static_cast<Texture3dst*>(obj)->setPixel(x, y, pixel_data);
}

void Tex3DSTGetPixel(Texture3dst_c obj, uint32_t x, uint32_t y, PixelData *pixel_data)
{
    static_cast<Texture3dst*>(obj)->getPixel(x, y, pixel_data);
}

Texture3dst_c Tex3DSTCrop(Texture3dst_c obj, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2)
{
    Texture3dst *textureDst = new Texture3dst();
    static_cast<Texture3dst*>(obj)->crop(*textureDst, x1, y1, x2, y2);
    return textureDst;
}

bool Tex3DSTPaste(Texture3dst_c obj, Texture3dst_c tex2, uint32_t x, uint32_t y)
{
    return static_cast<Texture3dst*>(obj)->paste(x, y, *static_cast<Texture3dst*>(tex2));
}

void Tex3DSTGetFormatInfo(Texture3dst_c obj, Format3dstInfo *dst)
{
    static_cast<Texture3dst*>(obj)->getFormatInfo(dst);
}

bool Tex3DSTCompare(Texture3dst_c obj, Texture3dst_c tex2, bool ignore_alpha)
{
    return static_cast<Texture3dst*>(obj)->compare(*static_cast<Texture3dst*>(tex2), ignore_alpha);
}

void Tex3DSTSave(Texture3dst_c obj, const char *path)
{
    static_cast<Texture3dst*>(obj)->save(path);
}

void Tex3DSTFree(Texture3dst_c obj)
{
    delete static_cast<Texture3dst*>(obj);
}