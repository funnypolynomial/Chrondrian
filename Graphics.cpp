#include <Arduino.h>
#include "Clock.h"
#include "Graphics.h"

namespace Graphics
{
// Render segment-text, digits, labels and icons

// Include the arrays of graphics data
#if defined(LCD_SMALL) || defined(FAKE_SMALL)
#include "Data_Small.h"
#else
#include "Data_Large.h"
#endif

// Assemble the graphics data into groups
static const uint8_t* const LargeDigitTable[] PROGMEM =
{
  LargeDigit0,
  LargeDigit1,
  LargeDigit2,
  LargeDigit3,
  LargeDigit4,
  LargeDigit5,
  LargeDigit6,
};

int LargeDigitWidth() { return LargeDigit_WIDTH; }
int LargeDigitHeight() { return LargeDigit_HEIGHT; }

static const uint8_t* const SmallDigitTable[] PROGMEM =
{
  SmallDigit0,
  SmallDigit1,
  SmallDigit2,
  SmallDigit3,
  SmallDigit4,
  SmallDigit5,
  SmallDigit6,
};

int SmallDigitWidth() { return SmallDigit_WIDTH; }
int SmallDigitHeight() { return SmallDigit_HEIGHT; }

static const uint8_t* const SmallCharTable[] PROGMEM =
{
  SmallChar0,
  SmallChar1,
  SmallChar2,
  SmallChar3,
  SmallChar4,
  SmallChar5,
  SmallChar6,
  SmallChar7,
  SmallChar8,
  SmallChar9,
  SmallChar10,
  SmallChar11,
  SmallChar12,
  SmallChar13,
};

int SmallCharWidth() { return SmallChar_WIDTH; }
int SmallCharHeight() { return SmallChar_HEIGHT; }

static const uint8_t* const VerySmallCharTable[] PROGMEM =
{
  VerySmallChar0,
  VerySmallChar1,
  VerySmallChar2,
  VerySmallChar3,
  VerySmallChar4,
  VerySmallChar5,
  VerySmallChar6,
  VerySmallChar7,
  VerySmallChar8,
  VerySmallChar9,
  VerySmallChar10,
  VerySmallChar11,
  VerySmallChar12,
  VerySmallChar13,
};

int VerySmallCharWidth() { return VerySmallChar_WIDTH; }
int VerySmallCharHeight() { return VerySmallChar_HEIGHT; }

static const uint8_t* const MoonTable[] PROGMEM =
{
  Moon0,
  Moon1,
  Moon2,
  Moon3,
};

int MoonWidth() { return Moon_WIDTH; }
int MoonHeight() { return Moon_HEIGHT; }

static const uint8_t* const DegreesTable[] PROGMEM =
{
  Degrees0, // main
  Degrees1, // middle
  Degrees2, // bottom
};

int DegreesWidth() { return Degrees_WIDTH; }
int DegreesHeight() { return Degrees_HEIGHT; }

static const uint8_t* const SunTable[] PROGMEM =
{
  Sun0, // ring and LHS
  Sun1, // RHS
};

int WeatherWidth() { return Sun_WIDTH; }
int WeatherHeight() { return Sun_HEIGHT; }

static const uint8_t* const CloudTable[] PROGMEM =
{
  Cloud0, // body
  Cloud1, // base
  Cloud2, // marker
};

static const uint8_t* const StormTable[] PROGMEM =
{
  Storm0, // drops
  Storm1, // lightening
};

static const uint8_t* const TextTable[] PROGMEM =
{
  Text0,  // Weather
  Text1,  // Moon
  Text2,  // Room
  Text3,  // PM
  Text4,  // Alarm
  Text5,  // <Bell>
};

void PaintRegion(int x0, int y0, const uint8_t* ptr, word colour, bool offset)
{
  // Paint the region pointed to by ptr, at x0, y0, with colour
  if (offset)
  {
    x0 += pgm_read_byte_near(ptr++);
    y0 += pgm_read_byte_near(ptr++);
  }
  else
    ptr += 2;
  uint8_t w  = pgm_read_byte_near(ptr++);
  if (w)
  {
    // Bulk fill
    uint8_t h = pgm_read_byte_near(ptr++);
    uint8_t dx = pgm_read_byte_near(ptr++);
    uint8_t dy = pgm_read_byte_near(ptr++);
    if (colour)
      LCD_FILL_COLOUR(LCD_BEGIN_FILL(x0 + dx, y0 + dy, w, h), colour);
    else
      LCD_FILL_BYTE(LCD_BEGIN_FILL(x0 + dx, y0 + dy, w, h), 0);
  }
  do
  {
    w = pgm_read_byte_near(ptr++);
    if (w & 0x80)  // skip rows
      y0 += (w & 0x7F);
    else if (w) // draw strip, offset follows
    {
      uint8_t offs = pgm_read_byte_near(ptr++);
      if (offs & 0x80)
      {
        y0++;
      }
      offs &= 0x7F;
      if (colour)
        LCD_FILL_COLOUR(LCD_BEGIN_FILL(x0 + offs, y0, w, 1), colour);
      else
        LCD_FILL_BYTE(LCD_BEGIN_FILL(x0 + offs, y0, w, 1), 0);
    }
  } while (w);
} 

void RegionOffset(const uint8_t* ptr, int& x, int& y)
{
  // Provide the offset to the reqion
  x = pgm_read_byte_near(ptr++);
  y = pgm_read_byte_near(ptr++);
}

/*
  A
F   B
  G
E   C
  D     
*/
byte p7SegPatterns[] =
{
//    0GFEDCBA
    0b00111111, // '0'
    0b00000110,
    0b01011011,
    0b01001111,
    0b01100110,
    0b01101101,
    0b01111101,
    0b00000111,
    0b01111111,
    0b01101111, // '9'
};

void LargeDigit(int x0, int y0, char ch, word onColour, word offColour)
{
  // Draw a large 7-seg digit
  byte segments = 0x00;
  if ('0' <= ch && ch <= '9')
    segments = p7SegPatterns[ch - '0'];
  for (int s = 0; s < 7; s++)
    PaintRegion(x0, y0,  pgm_read_ptr_near(LargeDigitTable + s), (segments & (1 << s))?onColour:offColour, true);
}

void SmallDigit(int x0, int y0, char ch, word onColour, word offColour)
{
  // Draw a small 7-seg digit
  byte segments = 0x00;
  if ('0' <= ch && ch <= '9')
    segments = p7SegPatterns[ch - '0'];
  else if (ch == '-')
    segments = 0b01000000;
  else if (ch & 0x80)
    segments = ch & 0x7F;    // custom: hi bit set, use the lower 7 as segments
  for (int s = 0; s < 7; s++)
    PaintRegion(x0, y0,  pgm_read_ptr_near(SmallDigitTable + s), (segments & (1 << s))?onColour:offColour, true);
}

// 14 segments:
//     --A--
// |F \H |I /J |B
//   -G1- -G2-
// |E /K |L \M |C
//     --D--
// Bit 0 is segment A
// based on https://line.17qq.com/articles/wmgsmkgny.html etc
static const uint16_t p14SegAZPatternss[] PROGMEM = 
{
//          GG
//    MLKJIH21FEDCBA
  0b0000000011110111, // A
  0b0001001010001111,
  0b0000000000111001,
  0b0001001000001111,
  0b0000000011111001,
  0b0000000011110001,
  0b0000000010111101,
  0b0000000011110110,
  0b0001001000001001,
  0b0000000000011110,
  0b0010010001110000,
  0b0000000000111000,
  0b0000010100110110,
  0b0010000100110110,
  0b0000000000111111,
  0b0000000011110011,
  0b0010000000111111,
  0b0010000011110011,
  0b0000000011101101,
  0b0001001000000001,
  0b0000000000111110,
  0b0000110000110000,
  0b0010100000110110,
  0b0010110100000000,
  0b0001010100000000,
  0b0000110000001001, // Z
};

static const uint16_t p14Seg09Patterns[] PROGMEM = 
{
//          GGFEDCBA
  0b0000000000111111, // '0'
  0b0000000000000110,
  0b0000000011011011,
  0b0000000011001111,
  0b0000000011100110,
  0b0000000011101101,
  0b0000000011111101,
  0b0000000000000111,
  0b0000000011111111,
  0b0000000011101111, // '9'
};

void SmallChar(int x0, int y0, char ch, word onColour, word offColour)
{
  // Draw a small 14-seg character
  word segments = 0x000;
  if ('A' <= ch && ch <= 'Z')
    segments = pgm_read_word_near(p14SegAZPatternss + (ch - 'A'));
  else if ('0' <= ch && ch <= '9')
    segments = pgm_read_word_near(p14Seg09Patterns + (ch - '0'));
  for (int s = 0; s < 14; s++)
    PaintRegion(x0, y0, pgm_read_ptr_near(SmallCharTable + s), (segments & (1 << s))?onColour:offColour, true);
}

void VerySmallChar(int x0, int y0, char ch, word onColour, word offColour)
{
  // Draw a very small 14-seg character
  word segments = 0x000;
  if ('A' <= ch && ch <= 'Z')
    segments = pgm_read_word_near(p14SegAZPatternss + (ch - 'A'));
  else if ('0' <= ch && ch <= '9')
    segments = pgm_read_word_near(p14Seg09Patterns + (ch - '0'));
  else if (ch == '/')
    segments = 0b0000110000000000;
  for (int s = 0; s < 14; s++)
    PaintRegion(x0, y0, pgm_read_ptr_near(VerySmallCharTable + s), (segments & (1 << s))?onColour:offColour, true);
}


void Moon(int x0, int y0, uint8_t segments, word onColour, word offColour)
{
  // segments is a bitset. if LSB set, left-most segments is ON (black)
  for (int i = 0; i < (int)(sizeof(MoonTable)/sizeof(MoonTable[0])); i++)
  {
    PaintRegion(x0, y0, pgm_read_ptr_near(MoonTable + i), (segments & 0x01)?onColour:offColour, true);
    segments >>= 1;
  }
}

void Degrees(int x0, int y0, bool celcius, word onColour, word offColour)
{
  // Paint <degree char> C/F
  PaintRegion(x0, y0, pgm_read_ptr_near(DegreesTable), onColour, true);
  PaintRegion(x0, y0, pgm_read_ptr_near(DegreesTable + 1), (!celcius)?onColour:offColour, true);
  PaintRegion(x0, y0, pgm_read_ptr_near(DegreesTable + 2), (celcius)?onColour:offColour, true);
}

void Weather(int x0, int y0, int idx, word onColour, word offColour)
{
  // Paint the idx'th weather icon
  bool sunBurst = false;
  switch (idx)
  {
    case 0: // Sun
      PaintRegion(x0, y0, pgm_read_ptr_near(SunTable + 0), (true)?onColour:offColour, true);
      PaintRegion(x0, y0, pgm_read_ptr_near(SunTable + 1), (true)?onColour:offColour, true);
      break;
    case 1: // Cloud + Sun
      PaintRegion(x0, y0, pgm_read_ptr_near(CloudTable + 0), (true)?onColour:offColour, true);
      PaintRegion(x0, y0, pgm_read_ptr_near(CloudTable + 1), (true)?onColour:offColour, true);
      sunBurst = true;
      break;
    case 2: // Cloud
      PaintRegion(x0, y0, pgm_read_ptr_near(CloudTable + 0), (true)?onColour:offColour, true);
      PaintRegion(x0, y0, pgm_read_ptr_near(CloudTable + 1), (true)?onColour:offColour, true);
      break;
    case 3: // Cloud + Sun + Rain
      PaintRegion(x0, y0, pgm_read_ptr_near(CloudTable + 0), (true)?onColour:offColour, true);
      PaintRegion(x0, y0, pgm_read_ptr_near(StormTable + 0), (true)?onColour:offColour, true);
      sunBurst = true;
      break;
    case 4: // Cloud + Lightening
      PaintRegion(x0, y0, pgm_read_ptr_near(CloudTable + 0), (true)?onColour:offColour, true);
      PaintRegion(x0, y0, pgm_read_ptr_near(StormTable + 1), (true)?onColour:offColour, true);
      break;
  }
  
  if (sunBurst)
  {
    int x, y;
    RegionOffset(pgm_read_ptr_near(CloudTable + 2), x, y);
    PaintRegion(x0 + x, y0 + y, pgm_read_ptr_near(SunTable + 1), (true)?onColour:offColour, false);
  }
}

void Text(int x0, int y0, TextBlocks text, word foreColour, word backColour)
{
  // Paint the text graphic ("WEATHER" etc) indexed by text
  const uint8_t* pData = pgm_read_ptr_near(TextTable + text);
  int w = pgm_read_byte_near(pData++);
  int h = pgm_read_byte_near(pData++);
  unsigned long ctr = LCD_BEGIN_FILL(x0, y0, w, h);
  while (ctr)
  {
    byte b = pgm_read_byte_near(pData++);;
    byte mask = 1;
    while (mask && ctr)
    {
      if (b & mask)
        if (foreColour)
          LCD_FILL_COLOUR(1, foreColour);
        else
          LCD_FILL_BYTE(1, 0x00);
      else
        if (backColour)
          LCD_FILL_COLOUR(1, backColour);
        else
          LCD_FILL_BYTE(1, 0x00);
      ctr--;
      mask <<= 1;
    }
  }
}

void TextSize(TextBlocks text, int& w, int& h)
{
  // Lookup the text graphics data and return the graphic block's width & height
  const uint8_t* pData = pgm_read_ptr_near(TextTable + text);
  w = pgm_read_byte_near(pData++);
  h = pgm_read_byte_near(pData++);
}

// Very small bmp font, used for debug info on-screen
// 3x5 Font, digits and capital letters, pluas a few extras
// 5x 3-bit rows.  b31 is unused but indicates a descender, b30 is top-left corner, b0 is bottom right
#define RANGE(_from, _to) (uint16_t)(_from | (_to << 8))  // encode a block of char defn's
#define FONT_WIDTH DebugCharWidth
#define FONT_HEIGHT 5
const uint16_t font3x5[] PROGMEM = 
{
  RANGE('0', 'Z'),
  //   0       1       2       3       4       5       6       7       8       9 
  075557, 022222, 071747, 071717, 055711, 074717, 074757, 071111, 075757, 075711,
  //   :       ;       <       =       >       ?       @
  066066, 060624, 012421, 007070, 042124, 075102, 077747,  
  //   A       B       C       D       E       F       G       H       I       J       K       L       M 
  025755, 065756, 074447, 065556, 074747, 074744, 074557, 055755, 072227, 072226, 055655, 044447, 077755, 
  //   N       O       P       Q       R       S       T       U       V       W       X       Y       Z 
  075555, 025552, 075744, 075561, 065655, 074717, 072222, 055557, 055552, 055777, 055255, 055717, 071247,
  RANGE('\0', '\0') // (end)
};

uint16_t PaintDebugChar(int16_t x, int16_t y, char ch, uint16_t foreColour, uint16_t backColour)
{
  // paints char, y is baseline, x is left, 3x5 font, in a 4x5 cell
  // returns the x of the next char
  if (0 > x || x > (LCD_WIDTH - FONT_WIDTH) || FONT_HEIGHT > y || y >= LCD_HEIGHT) return x;
  LCD_BEGIN_FILL(x, y - FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT);
  // Look up char
  const uint16_t* pPtr = font3x5;
  uint32_t data = 0;
  uint16_t range = pgm_read_word(pPtr++);
  while (range)
  {
    char from = range & 0xFF;
    char to = range >> 8;
    if (from <= ch && ch <= to)
    {
      data = pgm_read_word(pPtr + (ch - from));
      break;
    }
    else
    {
      pPtr += (to - from) + 1;
      range = pgm_read_word(pPtr++);
    }
  }
  uint16_t mask = 040000;
  int ctr = 0;
  while (mask)
  {
    if (mask & data)
      LCD_FILL_COLOUR(1, foreColour);
    else
      LCD_FILL_COLOUR(1, backColour);
    ctr++;
    if ((ctr % (FONT_WIDTH-1)) == 0)  // pad cell width
      LCD_FILL_COLOUR(1, backColour);
    mask >>= 1;
  }
  return x + FONT_WIDTH;
}

uint16_t PaintDebugStr(int16_t x, int16_t y, const char* pStr, uint16_t foreColour, uint16_t backColour)
{
  // paints str, y is baseline, x is left, 3x5 font, in a 4x5 cell
  // returns the x of the next char
  while (*pStr)
  {
    PaintDebugChar(x, y, *pStr++, foreColour, backColour);
    x += FONT_WIDTH;
  }
  return x;
}

};
