/**
 * @file
 *
 * @author Smileynator
 *
 * @brief Handles Image coordinates and references for asssistance in display of images.
 *
 * @copyright Thyme is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            2 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#include "always.h"
#include "image.h"

Image::Image() : m_name(), m_filename(), m_textureSize(), m_imageSize(), m_rawTextureData(), m_status(0)
{
    m_UVCoords.hi.x = 1.0f;
    m_UVCoords.hi.y = 1.0f;
    m_textureSize.x = 0;
    m_textureSize.y = 0;
    m_UVCoords.lo.x = 0.0f;
    m_UVCoords.lo.y = 0.0f;
    m_imageSize.x = 0;
    m_imageSize.y = 0;
    m_rawTextureData = nullptr;
}

Image::~Image()
{
    return;
}

// 0x00519260
void Image::Set_Status(uint32_t bit)
{
    m_status |= bit;
}

void Image::Clear_Status(uint32_t bit)
{
    m_status &= ~bit;
}

// 0x00518F10
void Image::Parse_Image_Coords(INI *ini, void *formal, void *store, const void *user_data)
{
    Image *image = static_cast<Image *>(formal);
    const char *token = ini->Get_Next_Sub_Token("Left");
    int left = ini->Scan_Int(token);
    token = ini->Get_Next_Sub_Token("Top");
    int top = ini->Scan_Int(token);
    token = ini->Get_Next_Sub_Token("Right");
    int right = ini->Scan_Int(token);
    token = ini->Get_Next_Sub_Token("Bottom");
    int bottom = ini->Scan_Int(token);
    float lowX = left;
    float lowY = top;
    float highX = right;
    float highY = bottom;
    if (image->m_textureSize.x != 0) {
        lowX = lowX / image->m_textureSize.x;
        highX = highX / image->m_textureSize.x;
    }
    if (image->m_textureSize.y != 0) {
        lowY = lowY / image->m_textureSize.y;
        highY = highY / image->m_textureSize.y;
    }
    if (image->m_UVCoords.hi.x != 0) {
        image->m_UVCoords.lo.x = lowX;
        image->m_UVCoords.lo.y = lowY;
        image->m_UVCoords.hi.x = highX;
        image->m_UVCoords.hi.y = highY;
    }
    image->m_imageSize.x = right - left;
    image->m_imageSize.y = bottom - top;
}

// 0x00519030
void Image::Parse_Image_Status(INI *ini, void *instance, void *store, const void *user_data)
{
    const char *_imageStatusNames[3] = { "ROTATED_90_CLOCKWISE", "RAW_TEXTURE", nullptr };

    Image *image = static_cast<Image *>(instance);
    ini->Parse_Bitstring32(ini, instance, store, _imageStatusNames);
    uint8_t *byte = static_cast<uint8_t *>(store);
    if (*byte & IMAGE_STATUS_ROTATED_90_CLOCKWISE) {
        int oldx = image->m_imageSize.x;
        image->m_imageSize.x = image->m_imageSize.y;
        image->m_imageSize.y = oldx;
    }
}
