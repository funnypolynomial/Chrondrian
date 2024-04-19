#pragma once

// Small-size display on large LCD
//#define FAKE_SMALL
// Switch between Large (320x480) and Small (240x320) LCD's
//#define LCD_SMALL
#ifdef LCD_SMALL
#include "Clock_Small.h"
#else
#include "Clock_Large.h"
#endif

#ifdef FAKE_SMALL
#undef LCD_WIDTH 
#undef LCD_HEIGHT
#define LCD_WIDTH  320
#define LCD_HEIGHT 240
#endif

// make an RGB word
#define RGB(_r, _g, _b) (word)((_b & 0x00F8) >> 3) | ((_g & 0x00FC) << 3) | ((_r & 0x00F8) << 8)

//#define DEBUG
#ifdef DEBUG
// Dump variable to serial
#define DBG(_x) { Serial.print(#_x);Serial.print(":");Serial.println(_x); }
#else
#define DBG(_x)
#endif

// Force showing sunny weather when N/A
//#define DEMO

namespace Clock
{
  // There are 5 cells
  enum tCells 
  {
    TimeCell,
    TemperatureCell,
    AlarmCell,
    WeatherCell,
    DateCell,
    
    Num_Cells
  };

  void Init();
  void Loop();
  void Reset();
  void Face();
  bool CheckTouch();
  int InCell(int x, int y);

  void FlashDLS(bool On);

  bool AlarmActive();

  void ShowTime(char* str, size_t sizeofStr, int Hour24, int Minute, byte Mask);
  void ShowAlarm(char* str, size_t sizeofStr, int Hour24, int Minute, byte Mask);
  void ShowDate(char* str, size_t sizeofStr, int day, int date, int month, int year, word Mask);
  void PaintDate(const char* pDate, uint16_t mask);
  void PaintMoon(uint8_t segments);
  void Copy(char*& pStr, const char* pValue);
  void Format(char*& pStr, int value, int MSD, char pad);
  bool GetTouch(int& x, int& y);
  void WaitForNoTouch();
};
