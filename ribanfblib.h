/*  Simple framebuffer graphics library
    Copyright:  riban 2019
    Author:     Brian Walotn (brian@riban.co.uk)
    License:    LGPL
*/
#pragma once

#include "colours.h" //Preset colours
#include "bitmap_image.hpp" //Provides bitmap manipulation
#include <stdint.h> //Provides fixed size int types
#include <string> //Provides std::string
#include <map> // Provides std::map
#include <linux/fb.h> //Provides framebuffer
#include <ft2build.h> //Provides freetype 2
#include FT_FREETYPE_H //Macro provides freetype 2 header

#define QUADRANT_TOP_RIGHT      0x01
#define QUADRANT_BOTTOM_RIGHT   0x02
#define QUADRANT_BOTTOM_LEFT    0x04
#define QUADRANT_TOP_LEFT       0x08
#define QUADRANT_TOP            0x09
#define QUADRANT_BOTTOM         0x06
#define QUADRANT_LEFT           0x0C
#define QUADRANT_RIGHT          0x03
#define QUADRANT_ALL            0x0F
#define QUADRANT_NONE           0x00
#define NO_FILL                 0xFFFFFFFF

extern "C" {

/** Class provides simple graphic element drawing to the framebuffer.
    All coordinates are in screen orientation starting with (0,0) at top left.
    Colours are 32-bit ARGB but alpha channel should be set to zero (used for internal flags).
    Helper functions allow conversion between colour depths.
    Framebuffer colour depth is identified and conversion applied from 32-bit ARGB colour.
    Angles are in degrees (not radians).
    Text rendering uses FreeType library to access supported fonts including TrueType. Size and rotation is implemented.
    Single functions for each fundamental shape are provided which allow the border thickness and internal fill colour to be specified.
    Supported framebuffer formats: packed pixels, truecolor, directcolor.
*/
class ribanfblib
{
    public:
        /** @brief  Instantiate a framebuffer object
        *   @param  device Name of framebuffer [default = /dev/fb0]
        */
        ribanfblib(const char* device = "/dev/fb0");

        /** @brief  Destroy the framebuffer object
        */
        virtual ~ribanfblib();

        /** @brief  Check if library is initiated successfully
        *   @retval bool True if library is ready to use
        */
        bool IsReady();

        /** @brief  Get the screen width
        *   @retval int Screen width in pixels (0 on error)
        */
        uint32_t GetWidth();

        /** @brief  Get the screen height
        *   @retval int Screen height in pixels (0 on error)
        */
        uint32_t GetHeight();

        /** @brief  Get the colour depth
        *   @retval int Colour depth in bits per pixel (0 on error)
        */
        uint32_t GetDepth();

        /** @brief  Clear the screen
        *   @param  colour Colour to wash screen [Default: Black]
        */
        void Clear(uint32_t colour = BLACK);

        /** @brief  Draw a single pixel
        *   @param  x The horizontal offset from left edge of screen
        *   @param  y The vertical offset from top edge of screen
        *   @param  colour Foreground colour [Default: White]
        */
        void DrawPixel(uint32_t x, uint32_t y, uint32_t colour = WHITE);

        /** @brief  Draw a straight line between two points
        *   @param  x1 The horizontal offset of the start of the line from left edge of screen
        *   @param  y1 The vertical offset of the start of the line from top edge of screen
        *   @param  x2 The horizontal offset of the end of the line from left edge of screen
        *   @param  y2 The vertical offset of the end of the line from top edge of screen
        *   @param  colour The colour of the line [Default: White]
        *   @param  weight The thickness of the line in pixels [Default: 1]
        */
        void DrawLine(int x1, int y1, int x2, int y2, uint32_t colour = WHITE, uint8_t weight = 1);

        /** @brief  Draw a rectangle
        *   @param  x1 The horizontal offset of the top left from left edge of screen
        *   @param  y1 The vertical offset of the top left from top edge of screen
        *   @param  x2 The horizontal offset of the bottom right from left edge of screen
        *   @param  y2 The vertical offset of the bottom right from top edge of screen
        *   @param  colour The colour of the border [Default: White]
        *   @param  border The thickness of the line in pixels [Default: 1]
        *   @param  fillColour The colour to fill the shape with [Default: (no fill)
        *   @param  round Bitwise flag indicating which corners should be rounded [Default: None]
        *   @param  radius Radius of curved corners in pixels
        */
        void DrawRect(int x1, int y1, int x2, int y2, uint32_t colour = WHITE, uint8_t border = 1, uint32_t fillColour = NO_FILL, uint8_t round = QUADRANT_NONE, uint32_t radius = 0);

        /** @brief  Draw a triangle
        *   @param  x1 The horizontal offset of first point from left edge of screen
        *   @param  y1 The vertical offset of first point left from top edge of screen
        *   @param  x2 The horizontal offset of second point from left edge of screen
        *   @param  y2 The vertical offset of second point left from top edge of screen
        *   @param  x3 The horizontal offset of third point from left edge of screen
        *   @param  y3 The vertical offset of third point left from top edge of screen
        *   @param  colour The colour of the border [Default: White]
        *   @param  border The thickness of the line in pixels [Default: 1]
        *   @param  fillColour The colour to fill the shape with [Default: no fill]
        */
        void DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t colour = WHITE, uint8_t border = 1, uint32_t fillColour = NO_FILL);

        /** @brief  Draw a circle
        *   @param  x The horizontal offset of centre from left edge of screen
        *   @param  y The vertical offset of centre from top edge of screen
        *   @param  radius The radius of the circle
        *   @param  colour The colour of the border [Default: White]
        *   @param  border The thickness of the line in pixels [Default: 1]
        *   @param  fillColour The colour to fill the shape with [Default: no fill]
        */
        void DrawCircle(int x, int y, uint32_t radius, uint32_t colour = WHITE, uint8_t border = 1, uint32_t fillColour = NO_FILL);

        /** @brief  Load a font to use for drawing text
        *   @param  height Font height
        *   @param  width Font width [Default: Same as height]
        *   @param  path Path to font [Default: Use last loaded font]
        *   @retval bool True on success
        *   @note   Height is specified first to allow only height to be passed (which is common usage). This differs to convention where height usually follows width.
        */
        bool SetFont(int height, int width = 0, std::string path = "");

        /** @brief  Draw text in currently selected font
        *   @param  sText Text to draw
        *   @param  x1 The horizontal offset of bottom left from left edge of screen
        *   @param  y1 The vertical offset of bottom left from top edge of screen
        *   @param  colour Foreground colour [Default: White]
        *   @param  angle Rotation angle in degrees (0..360) [Default: 0]
        *   @see    SetFont() to specify font face, size, etc.
        */
        void DrawText(std::string sText, int x, int y, uint32_t colour = WHITE, float angle = 0);

	/** @brief Load a bitmap into memory
	*   @param sFilename Full path and filename of bitmap file to load
	*   @param sName Name to use to refer to bitmap
	*   @retval bool True on success
	*/
	bool LoadBitmap(std::string sFilename, std::string sName);

	/** @brief Draw bitmap
	*   @param sName Name of a preloaded bitmap
	*   @param x X coordinate of top left corner
	*   @param y Y coordinate of top left corner
	*   @retval bool True on success
	*/
	bool DrawBitmap(std::string sName, int x, int y);

        /** @brief  Get a colour value based on the specified colour depth
        *   @param  red Red component
        *   @param  green Green component
        *   @param  blue Blue component
        *   @param  depth Colour depth in bits per pixel [8|16|24|32 0 for current framebuffer depth]
        *   @retval uint32_t Colour value at specified depth
        *   @note   If framebuffer is not initialised and automatic depth detection is requested then colour depth of 16 will be used
        */
        uint32_t GetColour(uint8_t red, uint8_t green, uint8_t blue, uint8_t depth);

        /** @brief  Get a 32-bit colour value from 8-bit RGB values
        *   @param  red Red component value
        *   @param  green Green component value
        *   @param  blue Blue component value
        *   @retval uint32_t 32-bit colour value
        */
        static uint32_t GetColour32(uint8_t red, uint8_t green, uint8_t blue);

        /** @brief  Get a 32-bit colour value from bitmap_image colour value
        *   @param  colour rgb_t 24-bit colour value
        *   @retval uint32_t 32-bit colour value
        */
        static uint32_t GetColour32(rgb_t colour);

        /** @brief  Get a colour at the specified depth from a 32-bit colour value
        *   @param  colour32 32-bit colour value
        *   @param  depth Quantity of bits in colour to return [8 | 16 | 24 | 32]
        *   @retval uint32_t Colour value at specified depth
        */
        static uint32_t GetColour(uint32_t colour, uint8_t depth);

        /** @brief  Convert a 24/32-bit colour value to framebuffer colour format
        *   @param  colour 32/24-bit colour value
        *   @retval uint32_t Colour value in framebuffer format
        */
        uint32_t GetColour(uint32_t colour32);

        /** @brief Get the textual representation of a framebuffer type
        *   @param  type The framebuffer type
        *   @retval string name of type
        */
        static std::string GetType(uint32_t type);

        /** @brief Get the textual representation of a framebuffer visual
        *   @param  type The framebuffer visual
        *   @retval string name of visual
        */
        static std::string GetVisual(uint32_t visual);

    protected:

    private:
        void drawBitmap(FT_Bitmap* bitmap, int x, int y, uint32_t colour);
        int drawChar(char c, int x, int y, int colour); //low level draw character from font, returns x coord of next character
        void drawLine(int x1, int y1, int x2, int y2, uint32_t colour); //Bresenham's line algorithm
        void drawQuadrant(int x0, int y0, uint32_t radius, uint32_t colour, uint8_t border, uint8_t quadrant = QUADRANT_ALL); //Draw each circle quadrant indicated by 4-bit (LSB) of quadrant

        int m_nLineLength; //Bytes in each line of framebuffer memory map (width x bbp)
        struct fb_var_screeninfo m_fbVarScreeninfo; //Framebuffer variable sceen info structure
        struct fb_fix_screeninfo m_fbFixScreeninfo; //Framebuffer fixed sceen info structure
        uint8_t* m_pFbmmap; //Pointer to framebuffer memory map
        int m_nFbHandle; //File handle for framebuffer device

        uint32_t m_nRedMask; //32-bit mask for red colour component
        uint32_t m_nGreenMask; //32-bit mask for green colour component
        uint32_t m_nBlueMask; //32-bit mask for blue colour component
        uint8_t m_nRedShift; //Quantity of bits to shift red colour component to match framebuffer colour format
        uint8_t m_nGreenShift; //Quantity of bits to shift green colour component to match framebuffer colour format
        uint8_t m_nBlueShift; //Quantity of bits to shift blue colour component to match framebuffer colour format

        FT_Library m_ftLibrary; //Freetype library
        FT_Face m_ftFace; //Freetype typeface
        int m_nFtLibInit; // 0 if Freetype library successfully initialised
        int m_nFtFace; // 0 if Freetype typeface loaded

	std::map<std::string,bitmap_image*> m_mmBitmaps; // Map of loaded bitmaps
};

}
