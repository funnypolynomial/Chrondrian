#include <Arduino.h>
// External libraries:
#include <SoftwareI2C.h>  // see RTC.h & SPL06_I2C.h

#include "Clock.h"
#include "Alarm.h"
#include "Config.h"
#include "Pins.h"
#include "BTN.h"
#include "RTC.h"

// Chrondrian: an LCD clock with time, date, temperature, "weather", moon phase and alarm.
// Inspired by something seen online, rectangles of subdued colour, visible "off" regions of 7-segment digits etc.
//
// +--------------------+-------+
// |                    | Temp  |
// |     Time           +-------+
// |                    | Alarm |
// +--------------------+-------+
// | Weather              Moon  |
// +----------------------------+
// | Date                       |
// +----------------------------+

// === Configuration ===
// == Buttons ==
// At runtime, press either the SET or the ADJ button.
// SET:
// Pressing SET enters config mode which sets the time, date etc. Initially the Time will blink.
// Pressing SET will advance to setting the Alarm (if enabled), the Date and the Moon.
// Press ADJ to actually set the blinking item.
// For the time, the hour will blink, ADJ increments it, SET moves on to the minutes.
// Similarly for the alarm.
// For the date, the order is year, month, date and day.
// For the moon, the setting is the date and time of the next new moon.  This is done as two values:
//   how many days to the new moon, this shows as "New in NNd", with NN blinking
//   what time, on that day, the new moon occurs, this shows as "New at HHMM", with HH then MM blinking (24-hour)
//
// *Holding* SET down toggles Daylight Savings off and on.
//
// ADJ:
// Pressing ADJ while the clock is running will toggle the Weather display between icons and text.
// In config mode, ADJ increments the blinking field (and SET accepts the blinking value and advances to the next field)
//
// == Touch ==
// Tapping the time, alarm, moon or date edits the corresponding value(s) (the alarm, only if enabled).
// Tapping on the weather toggles between icons and text.
// Touching and holding on the Time toggles DLS. On the Alarm, enables or disables it
// When editing, tapping within the cell that has the blinking field increments it.  Tapping anywhere else advances to the next field (or finishes)
//
// There are a number of defines in Config.h which alter the clock at *compile* time, for example 12/24-hour mode, temperature units, etc.
// Colours, layout/placement is defined at the top of Clock.cpp
//
// === Weather ===
// The "forecast" is computed using the Zambretti Forecaster algorithm. See Weather.cpp
//
// === Moon ===
// The moon phase is computed from the reference new moon, using a period of ~29.53059 days.  So it WILL drift slowly. See Moon.cpp

// === LCD ===
// Switch between large and small LCDs using #define LCD_SMALL in Clock.h
// The interface to the LCD implementations is via the macros defined in Clock_Large.h or Clock_Small.h, LCD_INIT, LCD_BEGIN_FILL, LCD_FILL_COLOUR etc
// For Touch, implement the LCD_GET_TOUCH macro and define LCD_HAS_TOUCH

// Mark Wilson, April 2024

void setup() 
{
#ifdef DEBUG
  Serial.begin(38400);
  Serial.println("Chrondrian");
#endif  
#ifdef SERIALIZE
  Serial.begin(38400);
#endif  
  btn1Set.Init(-PIN_BTN_SET); // these are analog
  btn2Adj.Init(-PIN_BTN_ADJ);
  SERIALISE_ON(true);
  LCD_INIT();
  LCD_FILL_BYTE(LCD_BEGIN_FILL(0, 0, LCD_WIDTH, LCD_HEIGHT), 0x00);
  rtc.setup();
  Config::Load();
  Clock::Init();
}

void loop() 
{
  if (btn1Set.CheckButtonPress() && !Alarm::CheckDeActivation())
    Config::Set();
  else if (btn2Adj.CheckButtonPress() && !Alarm::CheckDeActivation())
    Clock::Face();
  else if (!Clock::CheckTouch())
  {
    Clock::Loop();
    SERIALISE_ON(false);
  }
  else
    Alarm::CheckDeActivation();
}
