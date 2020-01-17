/*  Simple framebuffer graphics library
    Copyright:  riban 2019
    Author:     Brian Walotn (brian@riban.co.uk)
    License:    LGPL
*/
#include "ribanfblib.h"
#include <assert.h> //Provides assert error checking
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/types.h>
#include <cmath> //Provides sin,cos
#define PI 3.1415926535897932

ribanfblib::ribanfblib(const char* device)
{
    //Open framebuffer, get screen info and map to memory
    m_nFbHandle = open(device, O_RDWR);
    assert(m_nFbHandle);
    assert(ioctl(m_nFbHandle, FBIOGET_VSCREENINFO, &m_fbVarScreeninfo) == 0);
    assert(ioctl(m_nFbHandle, FBIOGET_FSCREENINFO, &m_fbFixScreeninfo) == 0);
    m_pFbmmap = (uint8_t *)mmap(0, m_fbFixScreeninfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, m_nFbHandle, 0);
    assert(m_pFbmmap != MAP_FAILED);
    m_nFtLibInit = -1;
    m_nFtFace = -1;
    if(FT_Init_FreeType(&m_ftLibrary) == 0)
    {
        if(m_fbFixScreeninfo.type == FB_TYPE_PACKED_PIXELS && // Only support packed pixels
           ((m_fbFixScreeninfo.visual == FB_VISUAL_TRUECOLOR) | (m_fbFixScreeninfo.visual == FB_VISUAL_DIRECTCOLOR))) //Only support truecolor | directcolor
           {
               m_nFtLibInit = 0;
               m_nRedMask = ((1 << m_fbVarScreeninfo.red.length) - 1) << (24 - m_fbVarScreeninfo.red.length);
               m_nRedShift = 24 - m_fbVarScreeninfo.red.length - m_fbVarScreeninfo.red.offset;
               m_nGreenMask = ((1 << m_fbVarScreeninfo.green.length) - 1) << (16 - m_fbVarScreeninfo.green.length);
               m_nGreenShift = 16 - m_fbVarScreeninfo.green.length - m_fbVarScreeninfo.green.offset;
               m_nBlueMask = ((1 << m_fbVarScreeninfo.blue.length) - 1) << (8 - m_fbVarScreeninfo.blue.length);
               m_nBlueShift = 8 - m_fbVarScreeninfo.blue.length - m_fbVarScreeninfo.blue.offset;
           }
    }
    if(m_nFtLibInit)
        printf("ERROR: Failed to initiate framebuffer - (%dx%d) %dbpp %s %s not supported by this library\n",
               GetWidth(), GetHeight(), GetDepth(), GetType(m_fbFixScreeninfo.type).c_str(), GetVisual(m_fbFixScreeninfo.visual).c_str()); //!@todo Remove this debug message
    SetFont(16, 16, "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
}

ribanfblib::~ribanfblib()
{
    munmap(m_pFbmmap, m_fbFixScreeninfo.smem_len);
    close(m_nFbHandle);
    if(m_nFtLibInit)
        FT_Done_FreeType(m_ftLibrary);
    for(auto it=m_mmBitmaps.begin(); it != m_mmBitmaps.end(); ++it)
        delete it->second;
}

bool ribanfblib::IsReady()
{
    return (m_nFtLibInit == 0);
}

uint32_t ribanfblib::GetWidth()
{
    return m_fbVarScreeninfo.xres;
}

uint32_t ribanfblib::GetHeight()
{
    return m_fbVarScreeninfo.yres;
}

uint32_t ribanfblib::GetDepth()
{
    return m_fbVarScreeninfo.bits_per_pixel;
}

void ribanfblib::Clear(uint32_t colour)
{
    if(!m_pFbmmap)
        return;
    if(!colour)
		memset(m_pFbmmap, 0, m_fbFixScreeninfo.smem_len);
    else
    {
        for(uint32_t x = 0; x < m_fbFixScreeninfo.line_length; ++x)
            for(uint32_t y = 0; y < m_fbVarScreeninfo.yres; ++y)
                DrawPixel(x, y, colour);
    }
}

void ribanfblib::DrawPixel(uint32_t x, uint32_t y, uint32_t colour)
{
//!@todo Would DrawPixel become too inefficient if we calculated each byte for different colour depths?
    if(x >= GetWidth() || y >= GetHeight())
        return; //Don't attempt to draw outside framebuffer
    switch(GetDepth())
    {
    case 32:
        *(uint32_t*)(m_pFbmmap + (y * m_fbFixScreeninfo.line_length + x * 4)) = colour;
        break;
    case 24:
        *(uint8_t*)(m_pFbmmap + (y * m_fbFixScreeninfo.line_length + x * 3)) = (uint8_t)(colour);
        *(uint8_t*)(m_pFbmmap + (y * m_fbFixScreeninfo.line_length + x * 3) + 1) = (uint8_t)(colour>> 8);
        *(uint8_t*)(m_pFbmmap + (y * m_fbFixScreeninfo.line_length + x * 3) + 2) = (uint8_t)(colour >> 16);
        break;
    case 16:
        *(uint16_t*)(m_pFbmmap + (y * m_fbFixScreeninfo.line_length + x * 2)) = (uint16_t)GetColour(colour);
        break;
    case 8:
        *(uint8_t*)(m_pFbmmap + (y * m_fbFixScreeninfo.line_length + x)) = (uint8_t)GetColour(colour);
    }
    /* Algorithm for plotting pixel is slower than using wider (than 8-bit) words
    uint8_t nBytes = GetDepth() / 8;
    uint32_t nColour = GetColour(colour);
    for(uint8_t nByte = 0; nByte < nBytes; ++nByte)
    {
        *(m_pFbmmap + (y * m_fbFixScreeninfo.line_length + x * nBytes) + nByte) = (uint8_t)(nColour >> (nByte * 8));
    }
    */
}

void ribanfblib::DrawLine(int x1, int y1, int x2, int y2, uint32_t colour, uint8_t weight)
{
    int nOffsetY = (x1 == x2)?0:1;
    int nOffsetX = (x1 == x2)?1:0;
    for(int n = 0; n < weight; ++n)
        drawLine(x1 + n * nOffsetX, y1 + n* nOffsetY, x2 + n * nOffsetX, y2 + n* nOffsetY, colour);
}

void ribanfblib::drawLine(int x1, int y1, int x2, int y2, uint32_t colour)
{
    const bool steep = (abs(y2 - y1) > abs(x2 - x1));
    if(steep)
    {
        std::swap(x1, y1);
        std::swap(x2, y2);
    }

    if(x1 > x2)
    {
        std::swap(x1, x2);
        std::swap(y1, y2);
    }

    const float dx = x2 - x1;
    const float dy = abs(y2 - y1);

    float error = dx / 2.0f;
    const int ystep = (y1 < y2) ? 1 : -1;
    int y = y1;

    for(int x = x1; x <= x2; x++)
    {
        if(steep)
        {
            DrawPixel(y,x, colour);
        }
        else
        {
            DrawPixel(x,y, colour);
        }

        error -= dy;
        if(error < 0)
        {
            y += ystep;
            error += dx;
        }
    }
}

void ribanfblib::DrawRect(int x1, int y1, int x2, int y2, uint32_t colour, uint8_t border, uint32_t fillColour, uint8_t round, uint32_t radius)
{
    //!@todo Validate rounded corners implementation
    if(x1 > x2)
        std::swap(x1, x2);
    if(y1 > y2)
        std::swap(y1, y2);
    if(fillColour != NO_FILL)
    {
        for(int nRow = y1 + border; nRow <= y2 - border; ++nRow)
            DrawLine(x1 + border, nRow, x2 - border + 1, nRow, fillColour);
    }
    DrawLine(x1 + radius, y1, x2 - radius, y1, colour, border); //Top
    DrawLine(x1 + radius, y2 - border + 1, x2 - radius, y2 - border + 1, colour, border); //Bottom
    DrawLine(x1, y1 + radius, x1, y2 - radius, colour, border); //Left
    DrawLine(x2 - border + 1, y1 + radius, x2 - border + 1, y2 - radius, colour, border); //Right
    if(radius)
    {
        if(round & QUADRANT_TOP_LEFT)
            drawQuadrant(x1 + radius, y1 + radius, radius, colour, border, QUADRANT_TOP_LEFT);
        if(round & QUADRANT_TOP_RIGHT)
            drawQuadrant(x2 - radius, y1 + radius, radius, colour, border, QUADRANT_TOP_RIGHT);
        if(round & QUADRANT_BOTTOM_LEFT)
            drawQuadrant(x1 + radius, y2 - radius, radius, colour, border, QUADRANT_BOTTOM_LEFT);
        if(round & QUADRANT_BOTTOM_RIGHT)
            drawQuadrant(x2 - radius, y2 - radius, radius, colour, border, QUADRANT_BOTTOM_RIGHT);
    }

}

void ribanfblib::DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t colour, uint8_t border, uint32_t fillColour)
{
    if(fillColour != NO_FILL)
    {
        //Sort vertices ascending by y axis to facilitate fill algorithm
        if(y1 > y2)
        {
            std::swap(y1, y2);
            std::swap(x1, x2);
        }
        if(y2 > y3)
        {
            std::swap(y2, y3);
            std::swap(x2, x3);
        }
        if(y1 > y2)
        {
            std::swap(y1, y2);
            std::swap(x1, x2);
        }
        float dx1 = 0;
        float dx2 = 0;
        float dx3 = 0;
        if(y2 - y1 > 0)
            dx1 = float(x2 - x1) / float(y2 - y1);
        if(y3 - y1 > 0)
            dx2 = float(x3 - x1) / float(y3 - y1);
        if(y3 - y2 > 0)
            dx3 = float(x3 - x2) / float(y3 - y2);
        float xS = x1;
        float yS = y1;
        float xE = x1;
        float yE = y1;
        if(dx1 > dx2)
        {
            for(; yS <= y2; yS++, yE++, xS += dx2, xE += dx1)
                DrawLine(xS, yS, xE, yS, fillColour);
            xE = x2;
            yE = y2;
            for(; yS <= y3; yS++, yE++, xS += dx2, xE += dx3)
                DrawLine(xS, yS, xE, yS, fillColour);
        } else {
            for(; yS <= y2; yS++, yE++, xS += dx1, xE += dx2)
                DrawLine(xS, yS, xE, yS, fillColour);
            xS = x2;
            yS = y2;
            for(; yS <= y3; yS++, yE++, xS +=dx3, xE += dx2)
                DrawLine(xS, yS, xE, yS, fillColour);
        }
    }
    DrawLine(x1, y1, x2, y2, colour, border);
    DrawLine(x2, y2, x3, y3, colour, border);
    DrawLine(x3, y3, x1, y1, colour, border);
}

void ribanfblib::DrawCircle(int x0, int y0, uint32_t radius, uint32_t colour, uint8_t border, uint32_t fillColour)
{
    if(fillColour != NO_FILL)
    {
        int nXoffset = 0;
        int nYoffset = radius;
        int balance = -radius;

        while(nXoffset <= nYoffset) //iterate between 0 and radius
        {
            int p0 = x0 - nXoffset; //x component of point on circumference LHS
            int p1 = x0 - nYoffset; //x component of point on circumference RHS

            int w0 = nXoffset + nXoffset; //width of circle at current y position (top / bottom quarter)
            int w1 = nYoffset + nYoffset; //width of circle at current y position (middle quarters)

            DrawLine(p0, y0 + nYoffset, p0 + w0, y0 + nYoffset, fillColour); //Horizontal line in lower quarter
            DrawLine(p0, y0 - nYoffset, p0 + w0, y0 - nYoffset, fillColour); //Horizontal line in upper quarter

            DrawLine(p1, y0 + nXoffset, p1 + w1, y0 + nXoffset, fillColour); //Horizontal line in lower half
            DrawLine(p1, y0 - nXoffset, p1 + w1, y0 - nXoffset, fillColour); //Horizontal line in upper half

            ++nXoffset;
            if((balance += nXoffset) >= 0)
            {
                --nYoffset;
                balance -= nYoffset;
            }
        }
    }
    drawQuadrant(x0, y0, radius, colour, border);
}

void ribanfblib::drawQuadrant(int x0, int y0, uint32_t radius, uint32_t colour, uint8_t border, uint8_t quadrant)
{
    bool bQ1 = ((quadrant & QUADRANT_TOP_RIGHT) == QUADRANT_TOP_RIGHT);
    bool bQ2 = ((quadrant & QUADRANT_BOTTOM_RIGHT) == QUADRANT_BOTTOM_RIGHT);
    bool bQ3 = ((quadrant & QUADRANT_BOTTOM_LEFT) == QUADRANT_BOTTOM_LEFT);
    bool bQ4 = ((quadrant & QUADRANT_TOP_LEFT) == QUADRANT_TOP_LEFT);
    //Draw concentric circles to width of border
    for(unsigned int nRadius = radius; nRadius > radius - border; --nRadius)
    {
        //Paint the top, bottom, left and right points that the simplified circle algorithm misses
        if(bQ1)
        {
            DrawPixel(x0, y0 - nRadius, colour); //Top
            DrawPixel(x0 + nRadius, y0, colour); //Right
        }
        if(bQ2)
        {
            DrawPixel(x0 + nRadius, y0, colour); //Right
            DrawPixel(x0, y0 + nRadius, colour); //Bottom
        }
        if(bQ3)
        {
            DrawPixel(x0, y0 + nRadius, colour); //Bottom
            DrawPixel(x0 - nRadius, y0, colour); //Left
        }
        if(bQ4)
        {
            DrawPixel(x0 - nRadius, y0, colour); //Left
            DrawPixel(x0, y0 - nRadius, colour); //Top
        }

        int f = 1 - nRadius;
        int ddF_x = 0;
        int ddF_y = -2 * nRadius;
        int x = 0;
        int y = nRadius;
        while(x < y)
        {
            if(f >= 0)
            {
                y--;
                ddF_y += 2;
                f += ddF_y;
            }
            x++;
            ddF_x += 2;
            f += ddF_x + 1;
            if(bQ1)
            {
                DrawPixel(x0 + x, y0 - y, colour); //0-45
                DrawPixel(x0 + y, y0 - x, colour); //45-90
            }
            if(bQ2)
            {
                DrawPixel(x0 + y, y0 + x, colour); //90-135
                DrawPixel(x0 + x, y0 + y, colour); //135-180
            }
            if(bQ3)
            {
                DrawPixel(x0 - x, y0 + y, colour); //180-225
                DrawPixel(x0 - y, y0 + x, colour); //225-270
            }
            if(bQ4)
            {
                DrawPixel(x0 - y, y0 - x, colour); //270-325
                DrawPixel(x0 - x, y0 - y, colour); //325-360
            }
        }
    }
}

bool ribanfblib::SetFont(int height, int width, std::string path)
{
    if(m_nFtLibInit)
        return false; //Freetype library not initialised
    if(path != "")
    {
        if(m_nFtFace == 0)
            FT_Done_Face(m_ftFace);
        m_nFtFace = FT_New_Face(m_ftLibrary, path.c_str(), 0, &m_ftFace);
    }
    if(m_nFtFace)
        return false;
    FT_Set_Pixel_Sizes(m_ftFace, width, height);
    return true;
}

void ribanfblib::DrawText(std::string text, int x, int y, uint32_t colour, float angle)
{
    if(m_nFtLibInit || m_nFtFace)
        return; //FreeType library not initialised or typeface not loaded
    FT_Matrix matrix;
    FT_Vector pen;
    FT_GlyphSlot slot = m_ftFace->glyph;
    //The matrix transforms Cartesian coordinates through angle storing as 16.16 fixed point numbers
    //Note cmath sin / cos accepts angles in radians
    matrix.xx = (FT_Fixed)(cos(PI * angle / 180) * 0x10000L); //Multiply by 0x10000 to convert to FT_FIXED (16.16)
    matrix.xy = (FT_Fixed)(-sin(PI * angle / 180) * 0x10000L); //Negate xy transform to rotate clockwise (opposite to polar coordinate system)
    matrix.yx = (FT_Fixed)(sin(PI * angle / 180) * 0x10000L);
    matrix.yy = (FT_Fixed)(cos(PI * angle / 180) * 0x10000L);
    pen.x = x * 64;
    pen.y = (GetHeight() - y) * 64;

    for(unsigned int n = 0; n < text.length(); ++n)
    {
        FT_Set_Transform(m_ftFace, &matrix, &pen);
        if(FT_Load_Char(m_ftFace, text[n], FT_LOAD_RENDER | FT_LOAD_MONOCHROME))
            continue;
        drawBitmap(&slot->bitmap, slot->bitmap_left, GetHeight() - slot->bitmap_top, colour);
        pen.x += slot->advance.x;
        pen.y += slot->advance.y;
    }
}

bool ribanfblib::LoadBitmap(std::string sFilename, std::string sName)
{
    bitmap_image* pImage =  new bitmap_image(sFilename);
    if(!pImage)
        return false;
    auto it = m_mmBitmaps.find(sName);
    if(it != m_mmBitmaps.end())
        delete it->second;
    m_mmBitmaps[sName] = pImage;
    return true;
}

bool ribanfblib::DrawBitmap(std::string sName, int x, int y)
{
    auto it = m_mmBitmaps.find(sName);
    if(it == m_mmBitmaps.end())
        return false; //bitmap not loaded
    bitmap_image* pImage = it->second;
    for(unsigned int y = 0; y < pImage->height(); ++y)
        for(unsigned int x = 0; x < pImage->width(); ++x)
        {
            rgb_t colour;
            pImage->get_pixel(x, y, colour);
            DrawPixel(x, y, GetColour32(colour));
        }
    return true;
}

void ribanfblib::drawBitmap(FT_Bitmap* bitmap, int x, int y, uint32_t colour)
{
    int nYmin = 0;
    int nYmax = bitmap->rows;
    int nYdir = 1;
    if(bitmap->pitch < 0)
    {
        nYmin = bitmap->rows;
        nYmax = 0;
        nYdir = -1;
    }
    //!@todo Validate negative TrueType pitch implementation
    //!@todo Optimise pitch calculation - might be able to draw different rather than iterate in reverse
    for(int dY = nYmin; dY < nYmax; dY += nYdir)
    {
        int nXcount = bitmap->width;
        for(int dX = 0; dX < bitmap->pitch && nXcount; ++dX)
        {
            uint8_t nMapByte = bitmap->buffer[dY * abs(bitmap->pitch) + dX];
            uint8_t nMask = 0x80;
            for(int dBit = 0; (dBit < 8) && nXcount; ++dBit)
            {
                if(nMapByte & nMask)
                    DrawPixel(x + dX * 8 + dBit, y + dY, colour);
                nMask >>= 1;
                --nXcount;
            }
        }
    }
}


uint32_t ribanfblib::GetColour(uint8_t red, uint8_t green, uint8_t blue, uint8_t depth)
{
    if(depth == 0)
        depth = GetDepth(); //Try to use framebuffer bpp
    if(depth == 0)
        depth = 16; //Fallback to 16bpp
    return GetColour(GetColour32(red, green, blue), depth);
}

uint32_t ribanfblib::GetColour32(uint8_t red, uint8_t green, uint8_t blue)
{
    return (red << 16) | (green << 8) | (blue);
}

uint32_t ribanfblib::GetColour32(rgb_t colour)
{
    return GetColour32(colour.red, colour.green, colour.blue);
}

uint32_t ribanfblib::GetColour(uint32_t colour32, uint8_t depth)
{
    switch(depth)
    {
    case 8: //332
        return ((colour32 & 0xE00000) >> 16) | ((colour32 & 0x00E000) >> 11) | ((colour32 & 0x0000C0) >> 6);
        break;
    case 16: //565
        return ((colour32 & 0xF80000) >> 8) | ((colour32 & 0x00FC00) >> 5) | ((colour32 & 0x0000F8) >> 3);
        break;
    case 24: //888
    case 32: //8888
        return colour32;
        break;
    }
    return colour32;
}

uint32_t ribanfblib::GetColour(uint32_t colour)
{
    return ((colour & m_nRedMask) >> m_nRedShift) | ((colour & m_nGreenMask) >> m_nGreenShift) | ((colour & m_nBlueMask) >> m_nBlueShift);
}

std::string ribanfblib::GetType(uint32_t type)
{
  switch(type)
  {
    case FB_TYPE_PACKED_PIXELS:
      return "Packed pixels";
      break;
    case FB_TYPE_PLANES:
      return "Planes";
      break;
    case FB_TYPE_INTERLEAVED_PLANES:
      return "Interleaved planes";
      break;
    case FB_TYPE_FOURCC:
      return "Four CC";
      break;
  }
  return "Unknown type";
}

std::string ribanfblib::GetVisual(uint32_t visual)
{
  switch(visual)
  {
    case FB_VISUAL_MONO01:
      return "Mono 01";
    case FB_VISUAL_MONO10:
      return "Mono 10";
    case FB_VISUAL_TRUECOLOR:
      return "Truecolor";
    case FB_VISUAL_PSEUDOCOLOR :
      return "Pseudocolor";
    case FB_VISUAL_STATIC_PSEUDOCOLOR:
      return "Static pseudocolor";
    case FB_VISUAL_DIRECTCOLOR:
      return "Directcolor";
    case FB_VISUAL_FOURCC:
      return "Four CC";
  }
  return "Unknown visual";
}
