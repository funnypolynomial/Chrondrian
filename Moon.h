#pragma once


namespace Time
{
  int DaysInMonth(int Month, int Year);  // 1..12, 2001..2099
  unsigned long MakeSeconds(int Date, int Month, int Year, int Hour, int Minute);
};

namespace Moon
{
  uint8_t Segments();
  void NextNewMoon(int& InDays, int& AtHour24, int& AtMinute);
};
