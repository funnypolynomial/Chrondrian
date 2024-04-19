#pragma once
namespace Graphics
{
  void LargeDigit(int x0, int y0, char ch, word onColour, word offColour);
  int LargeDigitWidth();
  int LargeDigitHeight();

  void SmallDigit(int x0, int y0, char ch, word onColour, word offColour);
  int SmallDigitWidth();
  int SmallDigitHeight();
  
  void SmallChar(int x0, int y0, char ch, word onColour, word offColour);
  int SmallCharWidth();
  int SmallCharHeight();

  void VerySmallChar(int x0, int y0, char ch, word onColour, word offColour);
  int VerySmallCharWidth();
  int VerySmallCharHeight();
  
  void Moon(int x0, int y0, uint8_t segments, word onColour, word offColour);
  int MoonWidth();
  int MoonHeight();  

  void Degrees(int x0, int y0, bool celcius, word onColour, word offColour);
  int DegreesWidth();
  int DegreesHeight();

  void Weather(int x0, int y0, int idx, word onColour, word offColour);
  int WeatherWidth();
  int WeatherHeight();

  enum TextBlocks {WeatherText, MoonText, RoomText, PMText, AlarmText, BellText};
  void Text(int x0, int y0, TextBlocks text, word foreColour, word backColour);
  void TextSize(TextBlocks text, int& w, int& h);

  const int DebugCharWidth = 4;
  uint16_t PaintDebugChar(int16_t x, int16_t y, char ch, uint16_t foreColour, uint16_t backColour);
  uint16_t PaintDebugStr(int16_t x, int16_t y, const char* pStr, uint16_t foreColour, uint16_t backColour);
};
