#include <Arduino.h>

#include "Clock.h"
#include "RTC.h"
#include "Config.h"
#include "Weather.h"
#include "Moon.h"
#include "Alarm.h"
#include "Graphics.h"

namespace Clock 
{

// Define the positions and colours of the "cells" on the clock face
// Cells are on a grid of 3rds and 4ths of the width/height
#define HORIZONTAL_DIV  LCD_WIDTH/3
#define VERTICAL_DIV    LCD_HEIGHT/4

#define YELLOW         RGB(224, 255,   8)
#define YELLOW_OFF     RGB(210, 235,   8)
#define BLUE           RGB(191, 226, 255)
#define BLUE_OFF       RGB(180, 210, 240)
#define LIGHT_BLUE     RGB(226, 255, 255)
#define LIGHT_BLUE_OFF RGB(205, 235, 235)

// The defintion of a particular cell
struct tCellDef
{
  // Position and dimenstions, in terms of the grid
  uint8_t _horzOffs;
  uint8_t _vertOffs;
  uint8_t _horzSpan;
  uint8_t _vertSpan;
  
  uint16_t _colour; // cell background colour
  uint16_t _colourOff;  // colour of off LCD regions
};

#ifdef CONFIG_LCD_OFF_SHOWN
// use colour and OFF colour. 
#define COLOURS(_clr) _clr, _clr##_OFF
#else
// don't use OFF colour
#define COLOURS(_clr) _clr, _clr
#endif

// Position, size and colour of cells, in terms of the grid
struct tCellDef pCellDefs[] =
{
// Order must match tCells enum
// X  Y  W  H
  {0, 0, 2, 2, COLOURS(LIGHT_BLUE)}, // TimeCell
  {2, 0, 1, 1, COLOURS(      BLUE)}, // TemperatureCell
  {2, 1, 1, 1, COLOURS(    YELLOW)}, // AlarmCell

  {0, 2, 3, 1, COLOURS(      BLUE)}, // WeatherCell
  {0, 3, 3, 1, COLOURS(    YELLOW)}, // DateCell  
};

// Handy macros for accessing members
#define CELL_X(_p) ((_p)->_horzOffs*HORIZONTAL_DIV + 1)
#define CELL_Y(_p) ((_p)->_vertOffs*VERTICAL_DIV + 1)
#define CELL_W(_p) ((_p)->_horzSpan*HORIZONTAL_DIV - 2)
#define CELL_H(_p) ((_p)->_vertSpan*VERTICAL_DIV - 2)

#if defined(LCD_SMALL) || defined(FAKE_SMALL)
#define GAP_Y 1
#else
#define GAP_Y 2
#endif

// The 5 icons on the weather cell
enum ForecastIcon {SunnyIcon, FairIcon, CloudyIcon, RainyIcon, StormyIcon,  Num_ForecastIcons};

void ShowDebug();  

void PaintCellBackgrounds()
{
  // Paint all the cell backgrounds
  tCellDef* pCellDef = pCellDefs;
  for (int iCell = TimeCell; iCell < Num_Cells; iCell++, pCellDef++)
    LCD_FILL_COLOUR(LCD_BEGIN_FILL(CELL_X(pCellDef), CELL_Y(pCellDef), CELL_W(pCellDef), CELL_H(pCellDef)), pCellDef->_colour);    
}

void PaintColon(int x, int y, int charGap, int charWidth, int charHeight, word colour, bool dot = false)
{
  // Draw a dot or a colon in the gap between chars of the given size
  int dy = dot?charHeight/2:charHeight/3;
  int size = dot?charWidth/4:charWidth/6;
  if ((charGap % 2) != (size % 2))
    size++; // make it even
  LCD_FILL_COLOUR(LCD_BEGIN_FILL(x + charGap/2 - size/2, y + dy - size/2, size, size), colour);
  if (!dot) // colon
    LCD_FILL_COLOUR(LCD_BEGIN_FILL(x + charGap/2 - size/2, y + 2*dy - size/2, size, size), colour);
}

int colonX = 0, colonY = 0, colonW = 10;
void UpdateColon(uint16_t colour)
{
  // Redraw (blinking) colon
  PaintColon(colonX, colonY, colonW, Graphics::LargeDigitWidth(), Graphics::LargeDigitHeight(), colour);
}

void PaintTime(const char* pTime, bool PM, uint8_t mask)
{
  // Paint the str as the time
  // pTime is "HH:MM", mask bits control what's shown as 'on'. MS bit is LH char
  tCellDef* pCellDef = pCellDefs + TimeCell;
  int colonWidth = Graphics::LargeDigitWidth()/3;
  int digitGap = colonWidth/4;
  int digitWidth = Graphics::LargeDigitWidth() + digitGap;
  int x = CELL_X(pCellDef) + (CELL_W(pCellDef) - 4*digitWidth - colonWidth)/2;
  int y = CELL_Y(pCellDef) + (CELL_H(pCellDef) - Graphics::LargeDigitHeight())/2;
  for (int ch = 0; ch < (int)strlen(pTime); ch++)
  {
    uint16_t colour = (mask & 0x80)?CONFIG_LCD_ON_COLOUR:pCellDef->_colourOff;
    mask <<= 1;
    if (pTime[ch] == ':')
    {
      colonX = x - digitGap;
      colonY = y;
      colonW = colonWidth + digitGap;
      UpdateColon(colour);
      x += colonWidth;
    }
    else
    {
      Graphics::LargeDigit(x, y, pTime[ch], colour, pCellDef->_colourOff);
      x += digitWidth;
    }
  }
  int w, h;
  Graphics::TextSize(Graphics::PMText, w, h);
  Graphics::Text(x - digitGap - w, y + Graphics::LargeDigitHeight() + h/2, Graphics::PMText, PM?CONFIG_LCD_ON_COLOUR:pCellDef->_colourOff, pCellDef->_colour);
}

void PaintDate(const char* pDate, uint16_t mask)
{
  // Paint the str as a date in the date cell
  // pDate is AAA NN.NN.NNNN, A's are letters, N's are digits
  // eg SAT 20. 9.2023
  tCellDef* pCellDef = pCellDefs + DateCell;
  int digitGap   = Graphics::SmallCharWidth()/12;
  int digitWidth = Graphics::SmallCharWidth() + digitGap;
  int x = CELL_X(pCellDef) + (CELL_W(pCellDef) - (strlen(pDate)*digitWidth - digitGap))/2;  
  int y = CELL_Y(pCellDef) + (CELL_H(pCellDef) - Graphics::SmallCharHeight())/2;
  for (int ch = 0; ch < (int)strlen(pDate); ch++)
  {
    uint16_t colour = (mask & 0x8000)?CONFIG_LCD_ON_COLOUR:pCellDef->_colourOff;
    mask <<= 1;
    if (pDate[ch] == '.') // the dots occupy the space of a character (vs the colons in the times, which are compressed)
      PaintColon(x, y, digitWidth - digitGap, Graphics::SmallCharWidth(), Graphics::SmallCharHeight(), colour, true);
    else if (pDate[ch] != '/')
    {
      if (::isalpha(pDate[ch]))
        Graphics::SmallChar(x, y, pDate[ch], colour,  pCellDef->_colourOff); 
      else
        Graphics::SmallDigit(x, y, pDate[ch], colour,  pCellDef->_colourOff); 
    }
    x += digitWidth;
  }  
}

void PaintMoon(uint8_t segments)
{
  // Paint the given moon segments
  #ifdef DEMO
  segments = 0x01;
  #endif  
  tCellDef* pCellDef = pCellDefs + WeatherCell;
  int w, h;
  Graphics::TextSize(Graphics::MoonText, w, h);
  Graphics::Moon(CELL_X(pCellDef) + CELL_W(pCellDef) - h - Graphics::MoonWidth(), CELL_Y(pCellDef) + (CELL_H(pCellDef) - Graphics::MoonHeight())/2 + 2*GAP_Y + 1, segments, CONFIG_LCD_ON_COLOUR, pCellDef->_colourOff);
}

const char pEmptyForecastStr[] PROGMEM = "";
const char pNoneForecastStr[] PROGMEM = "N/A";
void PaintForecast(char forecast, bool icons)
{
  // Paint the forecast letter as an icon or text
  // if forecast is '!', completely blanks the display
  // if forecast is '*', displays just the LCD shadows
  // '?' means N/A -- show all icons or "N/A"
#ifdef DEMO
  if (forecast == '?')
    forecast = 'D';
#endif  
  tCellDef* pCellDef = pCellDefs + WeatherCell;
  int w, h;
  Graphics::TextSize(Graphics::WeatherText, w, h);
  int digitGap = GAP_Y;
  int digitWidth = Graphics::VerySmallCharWidth() + digitGap;
  int x = CELL_X(pCellDef) + h;
  int y = CELL_Y(pCellDef) + h + 4*GAP_Y;
  uint16_t background = (forecast == '!')?pCellDef->_colour:pCellDef->_colourOff;
  if (forecast == '*')
  {
    forecast = '!';
    background = pCellDef->_colourOff;
  }
  if (icons)
  {
    ForecastIcon icon = Num_ForecastIcons;
    if (::isalpha(forecast)) // map the letter range A-Z to the icon range, Sunny-Stormy
      icon = (ForecastIcon)min((forecast - 'A')/(('Z' - 'A' + 1)/Num_ForecastIcons), Num_ForecastIcons - 1);
    for (int idx = 0; idx < Num_ForecastIcons; idx++)
      Graphics::Weather(CELL_X(pCellDef) + Graphics::WeatherWidth()*idx, CELL_Y(pCellDef) + (CELL_H(pCellDef) - Graphics::WeatherHeight())/2 + GAP_Y, idx, (idx == icon || forecast == '?')?CONFIG_LCD_ON_COLOUR:background, background);
  }
  else
  {
    bool progmemStr = true;
    char buff[10];
    const char* pForecastStr = pEmptyForecastStr;
    if (::isalpha(forecast))
      pForecastStr = Weather::GetForecastStr(forecast);
    else if (forecast == '?')
    {
      pForecastStr = pNoneForecastStr; // "N/A"
      float hPa = Weather::GetPressure();
      if (hPa != 0.0)
      {
        // show the current pressure!
        memset(buff, 0, sizeof(buff));
        char* pStr = buff;
        int hPa = Weather::GetPressure() + 0.5;
        Format(pStr, hPa + 0.5, 1000, ' ');
        Copy(pStr, " HPA");
        pForecastStr = buff;
        progmemStr = false; // read from buffer, not progmem
      }
    }
    const int kMinCol = 24; // where we wrap to 2nd line
    int x0 = x;
    for (int lines = 0; lines < 2; lines++)
    {
      int col = 0;
      char ch = progmemStr?pgm_read_byte_near(pForecastStr):*pForecastStr;
      if (ch)
        while (ch && ch != '\n')
        {
          Graphics::VerySmallChar(x, y, toupper(ch), CONFIG_LCD_ON_COLOUR, background);
          x += digitWidth;
          col++;
          ch = progmemStr?pgm_read_byte_near(++pForecastStr):*++pForecastStr;
        }
      if (ch)
        pForecastStr++;
      while (col < kMinCol)
      {
        Graphics::VerySmallChar(x, y, ' ', CONFIG_LCD_ON_COLOUR, background);
        x += digitWidth;
        col++;
      }
      x = x0;
      y += Graphics::VerySmallCharHeight() + 2*GAP_Y;
    }
  }
}

int MoonTextX = 0;
void PaintWeather(char forecast, uint8_t segments)
{
  // Paint the weather cell: forecast and moon phase
  tCellDef* pCellDef = pCellDefs + WeatherCell;
  int w, h;
  Graphics::TextSize(Graphics::WeatherText, w, h);
  Graphics::Text(CELL_X(pCellDef) + h, CELL_Y(pCellDef) + GAP_Y, Graphics::WeatherText, CONFIG_LCD_ON_COLOUR, pCellDef->_colour); 

  Graphics::TextSize(Graphics::MoonText, w, h);
  MoonTextX = CELL_X(pCellDef) + CELL_W(pCellDef) - w - h;
  Graphics::Text(MoonTextX, CELL_Y(pCellDef) + GAP_Y, Graphics::MoonText, CONFIG_LCD_ON_COLOUR, pCellDef->_colour); 

  PaintMoon(segments);
  
  PaintForecast(forecast, Config::ForecastIcons);
}

void PaintTemperature(const char* pTemp, bool Celcius)
{
  // Paint the str as temperature
  // pTemp is 3 characters
  tCellDef* pCellDef = pCellDefs + TemperatureCell;
  int colonWidth = Graphics::SmallDigitWidth()/3;
  int digitGap = colonWidth/4;
  int digitWidth = Graphics::SmallDigitWidth() + digitGap;
  int x = CELL_X(pCellDef) + (CELL_W(pCellDef) - 3*digitWidth)/2;
  int y = CELL_Y(pCellDef) + (CELL_H(pCellDef) - Graphics::SmallDigitHeight())/2;
  for (int digit = 0; digit < 3; digit++)
  {
    Graphics::SmallDigit(x, y, pTemp[digit], CONFIG_LCD_ON_COLOUR, pCellDef->_colourOff);
    x += digitWidth;
  }
  int w, h;
  Graphics::TextSize(Graphics::RoomText, w, h);
  Graphics::Text(CELL_X(pCellDef) + h/2, CELL_Y(pCellDef) + GAP_Y, Graphics::RoomText, CONFIG_LCD_ON_COLOUR, pCellDef->_colour); 

  Graphics::Degrees(x, y, Celcius, (*pTemp != '?')?CONFIG_LCD_ON_COLOUR:pCellDef->_colourOff, pCellDef->_colourOff);
}

void PaintAlarm(const char* pTime, bool PM, uint8_t mask)
{
  // Paint the time str in the Alarm cell, pTime is "HH:MM", LS bit of mask controls <bell>
  tCellDef* pCellDef = pCellDefs + AlarmCell;
  int colonWidth = Graphics::SmallDigitWidth()/3;
  int digitGap = colonWidth/4;
  int digitWidth = Graphics::SmallDigitWidth() + digitGap;
  int x = CELL_X(pCellDef) + (CELL_W(pCellDef) - 4*digitWidth - colonWidth)/2;
  int y = CELL_Y(pCellDef) + (CELL_H(pCellDef) - Graphics::SmallDigitHeight())/2;
  bool bell = mask & 1;
  PM = PM && (mask & 0x80);
  for (int ch = 0; ch < (int)strlen(pTime); ch++)
  {
    uint16_t colour = (mask & 0x80)?CONFIG_LCD_ON_COLOUR:pCellDef->_colourOff;
    mask <<= 1;
    if (pTime[ch] == ':')
    {
      PaintColon(x - digitGap, y, colonWidth + digitGap, Graphics::SmallDigitWidth(), Graphics::SmallDigitHeight(), colour);
      x += colonWidth;
    }
    else
    {
      Graphics::SmallDigit(x, y, pTime[ch], colour, pCellDef->_colourOff);
      x += digitWidth;
    }
  }
  int w, h;
  Graphics::TextSize(Graphics::AlarmText, w, h);
  Graphics::Text(CELL_X(pCellDef) + h/2, CELL_Y(pCellDef) + GAP_Y, Graphics::AlarmText, CONFIG_LCD_ON_COLOUR, pCellDef->_colour); 
  Graphics::Text(CELL_X(pCellDef) + w + h, CELL_Y(pCellDef) + GAP_Y, Graphics::BellText, bell?CONFIG_LCD_ON_COLOUR:pCellDef->_colourOff, pCellDef->_colour); 
  
  Graphics::TextSize(Graphics::PMText, w, h);
  Graphics::Text(x - digitGap - w, y + Graphics::SmallDigitHeight() + GAP_Y, Graphics::PMText, PM?CONFIG_LCD_ON_COLOUR:pCellDef->_colourOff, pCellDef->_colour);  
}

void Format(char*& pStr, int value, int MSD, char pad)
{
  // Format value into pStr, advancing it.  
  // MSD controls number of digits (100 = 3 digits).  Initial 0's padded with pad
  char prefix = pad;
  char* signAt = NULL;
  
  if (value < 0)
  {
    value = -value;
    prefix = '-';
  }
  while (MSD)
  {
    if (value < MSD && pad && !(value == 0 && MSD == 1))
    {
      signAt = pStr;
      *pStr++ = pad;
    }
    else
    {
      *pStr++ = '0' + (value / MSD);
      pad = 0;
    }
    value %= MSD;
    MSD /= 10;
  }
  if (signAt)
    *signAt = prefix;
}

void Copy(char*& pStr, const char* pValue)
{
  // Copy value to pStr, advancing pStr
  while (*pValue)
    *pStr++ = *pValue++;
}

void FormatTime(int Hour24, int Minute, char* pStr, bool& PM, bool active = true)
{
  // Format the time fields into str, "HH:MM", or blank if not active (eg alarm time), sets the PM flag
  PM = false;
  if (!active)
  {
    Copy(pStr, "  :  ");
    return;
  }
  int h = Hour24;
  #ifdef CONFIG_12_HOUR
    if (h > 12)
    {
      h -= 12;
      PM = true;
    }
    else if (h == 12)
      PM = true;
    if (h == 0)
      h = 12;
    Format(pStr, h, 10, ' ');
  #else
    Format(pStr, h, 10, '0');
  #endif
  *pStr++ = ':';
  Format(pStr, Minute, 10, '0');
}

void ShowTime(char* str, size_t sizeofStr, int Hour24, int Minute, byte Mask)
{
  // Format and paint the time fields
  bool PM = false;
  memset(str, 0, sizeofStr);
  char* pStr = str;
  Hour24 += Config::DLS;
  if (Hour24 > 23)
    Hour24 -= 24;
  FormatTime(Hour24, Minute, pStr, PM);
  PaintTime(str, PM, Mask);  
}

void ShowAlarm(char* str, size_t sizeofStr, int Hour24, int Minute, byte Mask)
{
  // Paint str as an Alarm time, str is "HH:MM"
  bool PM = false;
  memset(str, 0, sizeofStr);
  char* pStr = str;
  FormatTime(Hour24, Minute, pStr, PM);
  PaintAlarm(str, PM, Mask);  
}

const char* pDays = "---\0MON\0TUE\0WED\0THU\0FRI\0SAT\0SUN"; // no need for progmem, there's space
void ShowDate(char* str, size_t sizeofStr, int day, int date, int month, int year, word Mask)
{
  // format and paint the date fields
  char* pStr = str;
  memset(str, 0, sizeofStr);
  if (1 <= day && day <= 7)
    Copy(pStr, pDays + 4*(day));
  else
    Copy(pStr, pDays);      
  *pStr++ = '/';
  int first = date, second = month;
  #ifndef CONFIG_DATE_DD_MM_YYYY
    first = month;
    second = date;
  #endif
  Format(pStr, first, 10, ' ');
  *pStr++ = '.';
  Format(pStr, second, 10, ' ');
  *pStr++ = '.';
  Format(pStr, 2000 + year, 1000, '0');
  PaintDate(str, Mask);
}

void UpdateAlarm()
{
  // format and update the alarm panel
  char str[16];
#ifdef CONFIG_HIDE_DISABLED_ALARM  
  ShowAlarm(str, sizeof(str), Config::AlarmHour24, Config::AlarmMinute, AlarmActive()?0xFF:0x00);
#else
  ShowAlarm(str, sizeof(str), Config::AlarmHour24, Config::AlarmMinute, AlarmActive()?0xFF:0xFE);
#endif
}

void Splash()
{
  // Paint a mostly blank display but with "MEW 2024"
  PaintTime("  :  ", false, 0x00);
  PaintDate("MEW/  .  .2024", 
           0b1110000000111100);
  PaintWeather('*', 0);
  PaintTemperature("?  ", true);
  PaintAlarm("  :  ", false, 0x00);
}

unsigned long updateMS = 0; // update periodically
bool colonOn = true; // control colon blink
// track what's currently displayed, to see if it needs updating
int displayedMinute = -1;
int displayedDay = -1;
int displayedTemperature = -9999;
char displayedForecast = ' ';
uint8_t displayedSegments = 0xAA;
// Cleaner than ifdef's throughout
#ifdef CONFIG_CELCIUS
const bool displayCelcius = true;
#else
const bool displayCelcius = false;
#endif

void Init()
{
  // One-time initialisation
  PaintCellBackgrounds();
#ifndef DEBUG  
  Splash();
  delay(5000);
  PaintTime("  :  ", false, 0x00);  // compute position of the ':'
#else
  PaintTime("  :  ", false, 0x00);  // compute position of the ':'
#endif
  Weather::Init();
  Alarm::Init();
  PaintWeather(Weather::GetForecast(), Moon::Segments());
  UpdateAlarm();
  updateMS = millis();
  colonOn = true;
  displayedDay = displayedMinute = -1;
  displayedTemperature = -9999;  
}

void Loop()
{
  if (Alarm::Loop())
  {
    UpdateAlarm();
  }
  // The main loop, update measurements, check button presses, update the display
  unsigned long nowMS = millis();
  if ((displayedMinute == -1) || (nowMS - updateMS) > 1000)
  {
    updateMS = nowMS;

#ifdef CONFIG_BLINK_COLON
    // Blinking colon
    colonOn = !colonOn;
    UpdateColon(colonOn?CONFIG_LCD_ON_COLOUR:pCellDefs[TimeCell]._colourOff);
#endif    
    int minute = rtc.ReadMinute();
    if (minute != displayedMinute)
    {
      displayedMinute = minute;
      rtc.ReadTime(true);
      Alarm::CheckActivation(rtc.m_Hour24, rtc.m_Minute);
      char str[16];
      char* pStr = str;
      // *** The time
      ShowTime(str, sizeof(str), rtc.m_Hour24, rtc.m_Minute, 0xFF);

      // *** The date
      ShowDate(str, sizeof(str), rtc.m_DayOfWeek, rtc.m_DayOfMonth, rtc.m_Month, rtc.m_Year, 0xFFFF);

      Weather::Loop();
      // *** The temperature
      int T = Weather::GetTemperature();
      if (T != displayedTemperature)
      {
        displayedTemperature = T;
        memset(str, 0, sizeof(str));
        pStr = str;
        if (!displayCelcius)
          T = 9*T/5 + 32;
        Format(pStr, T, 100, ' ');
        PaintTemperature(str, displayCelcius);
      }
      // *** The forecast+moon
      char forecast = Weather::GetForecast();
      uint8_t segments = Moon::Segments();
      if (forecast != displayedForecast || segments != displayedSegments)
      {
        displayedForecast = forecast;
        displayedSegments = segments;
        PaintWeather(forecast, segments);
      }
      
#ifdef DEBUG  
      ShowDebug();
#endif      
    }
  }
}

void Reset()
{
  // Reset to refresh the display, eg after configuration
  UpdateAlarm();
  displayedMinute = -1;
}

void Face()
{
  // Toggle the clock face, just the forecast icon/textmode
  // erase
  PaintForecast('!', Config::ForecastIcons);
  // toggle
  Config::ForecastIcons = !Config::ForecastIcons;
  Config::Save();
  // repaint
  PaintForecast(displayedForecast, Config::ForecastIcons);
}

bool touchPrevReading = false, touchPrevState = false;
int touchX = 9999, touchY = 9999;
unsigned long touchTransitionTimeMS = 0;
#define TOUCH_THRESHOLD_PIXELS 50 // larger distance != a stable touch
#define TOUCH_TIME_MS 50 // touch held this long = stable touch
bool GetTouch(int& x, int& y)
{
  // debounced touch
  bool thisTouchReading = LCD_GET_TOUCH(x, y);
  if (thisTouchReading != touchPrevReading      || 
      abs(x -  touchX) > TOUCH_THRESHOLD_PIXELS || 
      abs(y -  touchY) > TOUCH_THRESHOLD_PIXELS)
  {
    // state has changed, reset
    touchPrevReading = thisTouchReading;
    touchX = x;
    touchY = y;
    touchTransitionTimeMS = millis();
  }
  else if (thisTouchReading != touchPrevState &&
           (millis() - touchTransitionTimeMS) >= TOUCH_TIME_MS) // a state other than the last one and held for long enough
    return (touchPrevState = thisTouchReading);
  return false;
}

void WaitForNoTouch()
{
  // waits until there is no touch detected
  int x, y;
  while (LCD_GET_TOUCH(x, y))
    ;
}

bool CheckTouch()
{
  // Check for, and process touches
  int x, y;
  if (GetTouch(x, y) && !Alarm::CheckDeActivation())
  {
    int iCell = InCell(x, y);
    if (iCell == TimeCell || iCell == AlarmCell)
    {
      // check for touch held -- toggle DLS
      int delayCounter = 5;  // 5x half a second
      bool touch = false;
      while (delayCounter)
      {
        if (!(touch = LCD_GET_TOUCH(x, y)))
          break;
        delayCounter--;
        delay(500);
      }
      if (!delayCounter && touch)
      {
        if (iCell == TimeCell)
        {
          Config::DLS = !Config::DLS;
          Config::Save();
          FlashDLS(Config::DLS);
        }
        else if (iCell == AlarmCell)
        {
          Alarm::SetEnabled(!Alarm::IsEnabled());
          UpdateAlarm();
        }
        WaitForNoTouch();
        Reset();
        return true;
      }
    }
    if (iCell == WeatherCell && x < MoonTextX)
    {
      // touch on weather - toggle icon/test
      Face();
      WaitForNoTouch();
      return true;   
    }
    else
    {
      Config::Edit(iCell);
    }
  }
  return false;
}

int InCell(int x, int y)
{
  // returns the cell x & y fall inside, or Num_Cells
  tCellDef* pCellDef = pCellDefs;
  for (int iCell = TimeCell; iCell < Num_Cells; iCell++, pCellDef++)
    if (CELL_X(pCellDef) < x && x < (CELL_X(pCellDef) + CELL_W(pCellDef)) && CELL_Y(pCellDef) < y && y < (CELL_Y(pCellDef) + CELL_H(pCellDef)))
      return iCell;
  return Num_Cells;
}

const char*    DLSOn    = "DLS/  .  .  0\xD4";    // 0xD4 = 'n'
const char*    DLSOff   = "DLS/  .  . 0\xF1\xF1"; // 0xF1 = 'F'
const uint16_t DLSmask = 0b1110000000111100;
void FlashDLS(bool On)
{
  // Flash the old, then new status of DLS
  PaintDate(On?DLSOff:DLSOn, DLSmask);
  delay(500);
  PaintDate(On?DLSOn:DLSOff, DLSmask);
  delay(1500);
}

bool AlarmActive()
{
  // true if the Alarm switch is on
  return Alarm::IsEnabled();
}

void ShowDebug()
{
  // [DBG:PPPPP F  PPPPP:PPPPP T] current P, forecast letter, old, new MSL P, trend
  int16_t currentP;
  int16_t currentAdjP;
  int16_t oldAdjP;
  char trend;
  char buff[16];
  char* pStr;
  int x = CELL_X(pCellDefs + WeatherCell) + CELL_W(pCellDefs + WeatherCell)/3;
  const int y = CELL_Y(pCellDefs + WeatherCell) + 8;    
  Weather::GetInfo(currentP, currentAdjP, oldAdjP, trend);      
  memset(buff, 0, sizeof(buff));
  pStr = buff;
  Copy(pStr, "DBG:");
  Format(pStr, currentP, 10000, ' ');
  *pStr++ = ' ';
  *pStr++ = displayedForecast;
  x = Graphics::PaintDebugStr(x, y, buff, 0, 0xFFFF) + 2*Graphics::DebugCharWidth;
  
  memset(buff, 0, sizeof(buff));
  pStr = buff;
  Format(pStr, oldAdjP, 10000, ' ');
  *pStr++ = ':';
  Format(pStr, currentAdjP, 10000, ' ');
  *pStr++ = ' ';
  *pStr++ = trend;
  x = Graphics::PaintDebugStr(x, y, buff, 0, 0xFFFF);
}

}
