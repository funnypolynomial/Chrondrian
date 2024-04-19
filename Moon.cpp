#include <Arduino.h>
#include "Clock.h"
#include "RTC.h"
#include "Config.h"
#include "Moon.h"

// Moon calc's Adapted from my ArDSKYLite project


namespace Time {
int DaysInMonth(int Month, int Year)  // 1..12, 2001..2099
{
  // (Days-28) encoded as 2bits/Month 0b0000DD..FFJJ00
  int Increment = (0x03BBEECCUL >> (2*Month) & 0x03);
  if ((Year % 4) || Increment)    // (simple leap year test)
    return 28 + Increment;
  else
    return 29;  // Leap year AND Feb
}

long DayOfCentury(int Date,  // 1..31
                        int Month, // 1..12
                        int Year)  // 2001..2099 (Leap year test is basic)
{
  // years into the "century"
  Year -= 2000;
  // days in whole years
  long Day = Date + 365L*Year;
  // leap days (except in 2000)
  Day += (Year - 1)/4L;

  // days in this year
  while (--Month > 0) 
    Day += DaysInMonth(Month, Year);
  return Day;
}

unsigned long MakeSeconds(int Date, int Month, int Year, int Hour, int Minute)
{
  // mktime-like thing, seconds since 1/1/2000 00:00
  unsigned long Minutes = DayOfCentury(Date, Month, Year)*24UL*60UL;
  Minutes += Hour*60UL + Minute;
  return Minutes*60UL;
}
}

namespace Moon {
void CalcAge(unsigned long referenceNewMoonSeconds, int Day, int Month, int Year, int Hour, int Minute,
                   int& AgeDays, int& Angle, int& DaysUntilPlus1,
                   int& InDays, int& AtHour24, int& AtMinute)
{
  // Angle is 0..180. 0=new 45=1st Q, 90=Full, 135=3rd Q (which seems weird now)
  // DaysUntilPlus1 is +ve if until Full, else until new
  // In/At specify the next new moon, as would be entered by the user to set the reference
  const unsigned long moonPeriodSeconds = 2551443UL;  // ~29.53059 (vs ~29.530587981) days
  const unsigned long dayLengthSeconds = 24UL*60UL*60UL;
  // NOT adjusted for DLS, it's only an hour!
  unsigned long CalcMoonSeconds = Time::MakeSeconds(Day, Month, Year, Hour, Minute);
  unsigned long diffSeconds = CalcMoonSeconds - referenceNewMoonSeconds;

  if (CalcMoonSeconds < referenceNewMoonSeconds)
    diffSeconds = moonPeriodSeconds - (referenceNewMoonSeconds - CalcMoonSeconds);
  unsigned long AgeSeconds = diffSeconds % moonPeriodSeconds;
  AgeDays = AgeSeconds/dayLengthSeconds;
  Angle = AgeSeconds*180UL/moonPeriodSeconds;
  
  // Calc days to next Full etc, this can be out by quite a lot since the Full may not be half-way between New's
  if (Angle < 90)
  {
    DaysUntilPlus1 = (moonPeriodSeconds/2UL - AgeSeconds)/dayLengthSeconds;
    if (((moonPeriodSeconds/2UL - AgeSeconds) % dayLengthSeconds) > dayLengthSeconds/2UL)
      DaysUntilPlus1++; // round up
    DaysUntilPlus1++;
  }
  else
  {
    DaysUntilPlus1 = (moonPeriodSeconds - AgeSeconds)/dayLengthSeconds;
    if (((moonPeriodSeconds - AgeSeconds)/dayLengthSeconds) > dayLengthSeconds/2UL)
      DaysUntilPlus1++; // round up
    DaysUntilPlus1++;
    DaysUntilPlus1 = -DaysUntilPlus1;
  }

  long SecondsToNew = moonPeriodSeconds - AgeSeconds;
  SecondsToNew -= (dayLengthSeconds - (Hour*60L + Minute)*60L);  // from midnight tonight
  long DaysToNew = SecondsToNew / dayLengthSeconds;  // New moon in this many days
  DaysToNew++;
  long TimeToNewSeconds = SecondsToNew % dayLengthSeconds;  // New moon this many seconds into the day
  long TimeToNewHour24 = TimeToNewSeconds / 3600L;
  long TimeToNewMinute = (TimeToNewSeconds - TimeToNewHour24*3600L)/60L;
  InDays = (int)DaysToNew;
  AtHour24 = (int)TimeToNewHour24;
  AtMinute = (int)TimeToNewMinute;
}

uint8_t GetSegments(int fromAngle, int toAngle)
{
  uint8_t segs = 0xFF; // all on (dark) new moon
  for (int s = 0; s < 4; s++)
  {
    int midAngle = (45 + s*90)/2; // angle at the middle of the segment
    if (fromAngle <= midAngle && midAngle <= toAngle)
    {
      segs &= ~(1 << s); // illuminated part straddles segment, light it (turn it off)
    }
  }
  return segs;
}

uint8_t Segments()
{
  // return a bitset of on segments for the current moon phase
  rtc.ReadTime(true);
  int ageDays, phaseAngle, daysUntilPlus1;
  int InDays, AtHour24, AtMinute;
  CalcAge(Config::RefNewMoonSeconds, rtc.m_DayOfMonth, rtc.m_Month, rtc.m_Year + 2000, rtc.m_Hour24, rtc.m_Minute,
          ageDays, phaseAngle, daysUntilPlus1,
          InDays, AtHour24, AtMinute);
#ifdef DEBUG  
//  if (daysUntilPlus1 > 0)
//  {
//    Serial.print("Full in ~");Serial.println(daysUntilPlus1-1);
//  }
//  else
//  {
//    Serial.print("New in ~");Serial.println((-daysUntilPlus1)-1);
//  }
#endif
  #ifdef CONFIG_SOUTHERN_HEMISPHERE
    if (phaseAngle <= 90)
      return GetSegments(0, phaseAngle*2);
    else
      return GetSegments((phaseAngle - 90)*2, 180);
  #else
    if (phaseAngle <= 90)
      return GetSegments(180 - phaseAngle*2, 180);
    else
      return GetSegments(0, 180 - (phaseAngle - 90)*2);
  #endif
}

void NextNewMoon(int& InDays, int& AtHour24, int& AtMinute)
{
  rtc.ReadTime(true);
  int ageDays, phaseAngle, daysUntilPlus1;
  CalcAge(Config::RefNewMoonSeconds, rtc.m_DayOfMonth, rtc.m_Month, rtc.m_Year + 2000, rtc.m_Hour24, rtc.m_Minute,
          ageDays, phaseAngle, daysUntilPlus1,
          InDays, AtHour24, AtMinute);
}
}
