#include <Arduino.h>
#include <EEPROM.h>
#include "Config.h"
#include "Clock.h"
#include "Moon.h"
#include "RTC.h"
#include "BTN.h"

namespace Config
{
// Defaults
byte DLS = 0;
byte AlarmHour24 = 7;
byte AlarmMinute = 0;
bool AlarmEnabled = false;  // only used on a touch screen, with no toggle switch
bool ForecastIcons = true;
unsigned long RefNewMoonSeconds = 750668100UL; // 15 October 2023, 6:55 a.m

#define EEPROM_OFFSET 32 // don't overlap other projects
void Load()
{
  // Read settings from EEPROM
  int idx = EEPROM_OFFSET;
  if (EEPROM.read(idx++) == 'C')
  {
    DLS = EEPROM.read(idx++)?1:0;
    AlarmHour24 = EEPROM.read(idx++) % 24;
    AlarmMinute = EEPROM.read(idx++) % 60;
    RefNewMoonSeconds = 0UL;
    for (int b = 0; b < 4; b++) // Big endian
    {
      RefNewMoonSeconds <<= 8;
      RefNewMoonSeconds |= EEPROM.read(idx++);
    }
    ForecastIcons = EEPROM.read(idx++);
    AlarmEnabled = EEPROM.read(idx++);
  }
  else
    Save();
}

void Save()
{
  // Write settings to EEPROM
  int idx = EEPROM_OFFSET;
  EEPROM.write(idx++, 'C');
  EEPROM.write(idx++, DLS?1:0);
  EEPROM.write(idx++, AlarmHour24);
  EEPROM.write(idx++, AlarmMinute);
  for (int b = 0; b < 4; b++)
    EEPROM.write(idx++, RefNewMoonSeconds >> 8*(3-b));
  EEPROM.write(idx++, ForecastIcons?1:0);
  EEPROM.write(idx++, AlarmEnabled?1:0);
}

// B a s e
class Editor
{
  // blinks the current setting (Time, Date etc)
  // Pressing Set goes to the next setting
  // Pressing Adj starts editing the setting (the first field within the setting (Hour, Year etc)
  // In that mode, Adj increments, Set goes to the next field
  public:
    Editor(int cellIdx)
    {
      blinkTimerMS = idleTimerMS = millis();
      cell = cellIdx;
    }
    
    void DirectEdit()
    {
      // start with the first field
      OnNextField(field = 0, offMask);
      Edit();
    }
    
    bool Edit()
    {
      // The main edit loop
      while (!exit)
      {
        if (update)
        {
          OnUpdate(blink?onMask:offMask);
          update = false;
        }
        unsigned long nowMS = millis();
        if ((nowMS - blinkTimerMS) > 500UL)  // toggle blink
        {
          blinkTimerMS = nowMS;
          update = true;
          blink = !blink;
        }
        if ((nowMS - idleTimerMS) > 30000UL)  // idle
        {
          field = 0;
          exit = true;
        }
        bool Set, Adj;
        CheckButtons(Set, Adj);        
        if (Set) // next field
        {
          if (field == -1)
            exit = true;  // next setting
          else
            save = exit = OnNextField(++field, offMask);
          idleTimerMS = nowMS;
        }
        else if (Adj)  // increment field
        {
          if (field == -1)
            OnNextField(field = 0, offMask); // start editing
          else
            OnNextValue(field);
          blink = update = true;
          idleTimerMS = nowMS;
        }    
      }
      OnExit(save);
      return field != -1;
    }

  // Repaint the fields, with the given mask (on/off)
  virtual void OnUpdate(word mask) = 0;
  
  // Advance to the next field, return true if done (i.e. beyond last field)
  virtual bool OnNextField(char field, word& offMask) = 0;
  
  // Advance the field's value
  virtual void OnNextValue(char field) = 0;
  
  // Editing is done, save is true if accepted
  virtual void OnExit(bool save) = 0;
  
  protected:
    void CheckButtons(bool& Set, bool& Adj)
    {
      int x, y;
      if (Clock::GetTouch(x, y))
      {
        Adj = Clock::InCell(x, y) == cell;
        Set = !Adj;
      }
      else if ((Set = btn1Set.CheckButtonPress()))
        Adj = false;
      else
        Adj = btn2Adj.CheckButtonPress();
    }
    
    static char str[16];

    unsigned long blinkTimerMS = 0;
    unsigned long idleTimerMS = 0;
    word offMask  = 0;  // start blinking everything
    word onMask  = 0xFFFF;  // start blinking everything
    bool update = true;
    bool exit = false;
    bool save = false;
    bool blink = true;
    char field = -1;  // -1 means BEFORE the first field (field 0), "all fields" blink, for the case of selecting what to change, using buttons
    int cell = -1;
};

char Editor::str[16]; // shared buffer

// T i m e
class TimeEditor:public Editor
{
  public:
    TimeEditor():Editor(Clock::TimeCell) {hour24 = rtc.m_Hour24; minute = rtc.m_Minute; }

    void OnUpdate(word mask) override
    { 
      Clock::ShowTime(str, sizeof(str), hour24, minute, mask);
    }
    
    bool OnNextField(char field, word& offMask) override
    {
      if (field == 0)
        //          hh:MM
        offMask = 0b00111111; // Hour
      else if (field == 1)
        offMask = 0b11100111; // Minute
      else
        return true;   // done
      return false;
    }
    
    void OnNextValue(char field) override
    {
     if (field == 0)
        hour24++;
      else if (field == 1)
        minute++;
      hour24 %= 24;
      minute %= 60;
    }

    void OnExit(bool save) override
    {
      if (save)
      {
        rtc.ReadTime(true);
        rtc.m_Hour24 = hour24; 
        rtc.m_Minute = minute;
        rtc.m_Second = 0; 
        rtc.WriteTime();
      }
      else
       OnUpdate(0xFFFF);
    }
  protected:
    int hour24;
    int minute;
};

// A l a r m
class AlarmEditor:public TimeEditor
{
  public:
    AlarmEditor() {hour24 = Config::AlarmHour24; minute = Config::AlarmMinute; cell = Clock::AlarmCell; }

    void OnUpdate(word mask) override
    { 
      Clock::ShowAlarm(str, sizeof(str), hour24, minute, mask);
    }
    
    void OnExit(bool save) override
    {
      if (save)
      {
         Config::AlarmHour24 = hour24; 
         Config::AlarmMinute = minute;
         Config::Save();
      }
      else
        OnUpdate(0xFFFF);
    }
};

// D a t e
class DateEditor:public Editor
{
  public:
    DateEditor():Editor(Clock::DateCell) {day = rtc.m_DayOfWeek - 1; date = rtc.m_DayOfMonth - 1; month = rtc.m_Month - 1; year = rtc.m_Year - 23; }

    void OnUpdate(word mask) override
    { 
      Clock::ShowDate(str, sizeof(str), day + 1, date + 1, month + 1, year + 23, mask);
    }
    
    bool OnNextField(char field, word& offMask) override
    {
      if (field == 0)
        //          www dd.mm.yyyy
        offMask = 0b1111111111000011; // Year
// Swap month/date        
#ifdef CONFIG_DATE_DD_MM_YYYY
      else if (field == 1)
      {
        offMask = 0b1111111001111111; // Month
        date  %= Time::DaysInMonth(month + 1, year + 2023);
      }
      else if (field == 2)
        offMask = 0b1111001111111111; // Date
#else
      else if (field == 1)
        offMask = 0b0111001111111111; // Date
      else if (field == 2)
      {
        offMask = 0b1111111001111111; // Month
        date  %= Time::DaysInMonth(month + 1, year + 2023);
      }
#endif        
      else if (field == 3)
        offMask = 0b0001111111111111; // Day
      else
        return true;   // done
      return false;
    }
    
    void OnNextValue(char field) override
    {
      if (field == 0)
        year++;
      else if (field == 1)
        month++;
      else if (field == 2)
        date++;
      else if (field == 3)
        day++;
      year  %= 20;
      month %= 12;
      date  %= Time::DaysInMonth(month + 1, year + 2023);
      day   %= 7;
    }

    void OnExit(bool save) override
    {
      if (save)
      {
        rtc.ReadTime(true);
        rtc.m_DayOfWeek = day + 1;
        rtc.m_DayOfMonth = date + 1; 
        rtc.m_Month = month + 1; 
        rtc.m_Year = year + 23;
        rtc.WriteTime();      
      }
      else
       OnUpdate(0xFFFF);
    }
  private:
    int day, date, month, year;
};


// M o o n
class MoonEditor:public Editor
{
  public:
    MoonEditor():Editor(Clock::DateCell)
    {
      Moon::NextNewMoon(inDays, atHour24, atMinute);
      //         www dd.mm.yyyy
      onMask = 0b1111110110111111;
      segments = Moon::Segments();
    }

    void FormatStr()
    {
      memset(str, 0, sizeof(str));
      char* pStr = str;
      if (field == 0)
      {
        Clock::Copy(pStr, "NEW/\x86\xD4.  ."); // NEW In   XXXd"
        Clock::Format(pStr, inDays, 100, ' ');
        Clock::Copy(pStr, "\xDE"); // 'd'
      }
      else
      {
        Clock::Copy(pStr, "NEW/\xF7\xF8.  ."); // NEW At   HHMM"
        Clock::Format(pStr, atHour24, 10, '0');
        Clock::Format(pStr, atMinute, 10, '0');
      }
    }
    
    void OnUpdate(word mask) override
    {
      if (field == -1)
        Clock::PaintMoon(mask?segments:~segments);  // blink the moon phase
      else
        Clock::PaintDate(str, mask);
    }
    
    bool OnNextField(char field, word& offMask) override
    {
      FormatStr();
      if (field == 0)
      {
        //          NEW in.  .XXXd
        offMask = 0b1111110110000111; // days
        Clock::PaintMoon(segments); // restore the moon phase
      }
      else if (field == 1)
        offMask = 0b1111110110001111; // hour
      else if (field == 2)
        offMask = 0b1111110110110011; // minute
      else
        return true;   // done
      return false;
    }
    
    void OnNextValue(char field) override
    {
      if (field == 0)
        inDays++;
      else if (field == 1)
        atHour24++;
      else if (field == 2)
        atMinute++;
      inDays  %= 29;
      atHour24 %= 24;
      atMinute %= 60;
      FormatStr();
    }

    void OnExit(bool save) override
    {
      if (save)
      {
        rtc.ReadTime(true);
        Config::RefNewMoonSeconds = Time::MakeSeconds(rtc.m_DayOfMonth, rtc.m_Month, rtc.m_Year + 2000, inDays*24 + atHour24, atMinute);
        Config::Save();
      }
      else
      {
        OnUpdate(segments);
      }
    }
  private:
    uint8_t segments;
    int inDays, atHour24, atMinute;
};

bool CheckDLSToggle()
{
  // check for SET held -- toggle DLS, returns true if was toggled
  int delayCounter = 5;  // 5x half a second
  while (delayCounter)
  {
    if (!btn1Set.IsDown())
    {
      break;
    }
    delayCounter--;
    delay(500);
  }
  if (!delayCounter && btn1Set.IsDown())
  {
    // held
    DLS = !DLS;
    Save();
    Clock::FlashDLS(DLS);
    Clock::Reset();
    return true;
  }
  return false;
}

void Set()
{
  // choose to set the time, (alarm), date, or moon reference
  if (CheckDLSToggle())
    return;
  TimeEditor time;
  if (!time.Edit())
  {
    AlarmEditor alarm;
    if (!Clock::AlarmActive() || !alarm.Edit()) // only edit alarm time if active
    {
      DateEditor date;
      if (!date.Edit())
      {
        MoonEditor moon;
        moon.Edit();
      }
    }
  }
  Clock::Reset();
}

void Edit(int cell)
{
  switch (cell)
  {
    case Clock::TimeCell:
    {
      TimeEditor time;
      time.DirectEdit();
      break;
    }
    case Clock::AlarmCell:
    {
      if (Clock::AlarmActive())
      {      
        AlarmEditor alarm;
        alarm.DirectEdit();
        break;
      }
      return;
    }
    case Clock::DateCell:
    {
      DateEditor date;
      date.DirectEdit();
      break;
    }
    case Clock::WeatherCell:
    {
      MoonEditor moon;
      moon.DirectEdit();
      break;
    }
    default:
      return;   
  }
  Clock::Reset();
}
};
