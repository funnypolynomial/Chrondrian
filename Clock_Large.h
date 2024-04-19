#pragma once
// Large version uses 320x480 LCD Shield, ILI948x interface, optionally with touch

#include "ILI948x.h"
// The LCD interface
// Initialise
#define LCD_INIT() { ILI948x::Init();ILI948x::DisplayOn(); }
// Define a window to fill with pixels at (_x,_y) width _w, height _h
// Returns the number of pixels to fill (unsigned long)
#define LCD_BEGIN_FILL(_x,_y,_w,_h) ILI948x::Window(_x,_y,_w,_h)
// Sends _sizeUL (unsigned long) pixels of the 16-bit colour
#define LCD_FILL_COLOUR(_sizeUL, _colorWord) ILI948x::ColourWord( _colorWord, _sizeUL)
// Sends _sizeUL (unsigned long) pixels of the 8-bit colour.
// The byte is duplicated, 0xFF and 0x00 really only make sense. Slightly faster than above.
#define LCD_FILL_BYTE(_sizeUL, _colorByte) ILI948x::ColourByte(_colorByte, _sizeUL)
// Sends a single white pixel
#define LCD_ONE_WHITE() ILI948x::OneWhite()
// Sends a single black pixel
#define LCD_ONE_BLACK() ILI948x::OneBlack()
// True if there is a touch. Returns position in (int) _x, _y
#define LCD_GET_TOUCH(_x, _y) ILI948x::GetTouch(_x, _y)

//#define LCD_HAS_TOUCH
