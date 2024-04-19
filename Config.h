#pragma once

// Display 12-hour vs 24
#define CONFIG_12_HOUR

// Display Celcius vs Fahrenheit
#define CONFIG_CELCIUS

// Display date as Day, Month, Year vs Month, Day, Year
#define CONFIG_DATE_DD_MM_YYYY

// Display LCD segments which are off in a fainter colour, vs hide them
#define CONFIG_LCD_OFF_SHOWN

// Colour used for LCD segments that are on
#define CONFIG_LCD_ON_COLOUR 0

// If defined, bell AND time are hidden if the alarm is disabled, othewise only the bell is hidden/shown
#define CONFIG_HIDE_DISABLED_ALARM

// If defined, main time colon blinks
// Display pulses slightly when blinking colon on non-USB power
//#define CONFIG_BLINK_COLON

// Display for Southern hemisphere, vs Northern
// Also used to determine the season for the weather forecast
#define CONFIG_SOUTHERN_HEMISPHERE

// Weather "forecast" constants etc. FOR ENTERTAINMENT ONLY!
// Height above MSL in meters. If defined, used to adjust the air pressure reading to sea level
#define CONFIG_ALTITUDE_METERS    20
// Local pressure range, at sea level, in hPa
// If defined, pressures in the range are mapped to the Zambretti Algorithm range of 947-1050
// This may make no sense!
//#define CONFIG_MIN_PRESSURE_HPA  992
//#define CONFIG_MAX_PRESSURE_HPA 1025 

namespace Config
{
  void Load();
  void Save();

  extern byte AlarmHour24;
  extern byte AlarmMinute;
  extern bool AlarmEnabled;
  extern byte DLS; 
  extern unsigned long RefNewMoonSeconds;
  extern bool ForecastIcons;

  void Set();
  void Edit(int cell);
};
