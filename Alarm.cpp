#include <Arduino.h>
#include "Clock.h"
#include "Config.h"
#include "Pins.h"
#include "BTN.h"
#include "Alarm.h"

namespace Alarm
{
// check the enabled switch, trigger/stop the alarm buzz
  
BTN btnAlarm;
bool alarmIsEnabled;  // alarm switch is on
bool buzzerIsActive;  // buzzer is sounding
bool alarmTriggered;  // have been triggered by alarm time, but only once

void Init()
{
  // Init the toggle/slider, and the state
  pinMode(PIN_PWM_BUZZER, OUTPUT);
  digitalWrite(PIN_PWM_BUZZER, LOW);
#ifdef LCD_HAS_TOUCH
  alarmIsEnabled = Config::AlarmEnabled;
#else
  btnAlarm.Init(PIN_SW_ALARM); // +ve means digital. Pin 13 has an LED and a resistor
  alarmIsEnabled = btnAlarm.IsDown();
#endif  
  buzzerIsActive = alarmTriggered = false;
}

bool Loop()
{
#ifndef LCD_HAS_TOUCH
  // Check enabled state, returns true if changed
  bool enabled = !btnAlarm.IsDown(); // reverse the sense, just so alarm enabled lights the built-in LED
  if (!enabled && buzzerIsActive)
    Buzzer(false);  // make sure buzzer is off is alarm disabled
  if (enabled != alarmIsEnabled)
  {
    alarmIsEnabled = enabled;
    return true;
  }
#endif  
  return false;
}

void Buzzer(bool On)
{
  digitalWrite(PIN_PWM_BUZZER, On); // Buzzer is active type
  buzzerIsActive = On;
}

bool IsEnabled()
{
  return alarmIsEnabled;
}

void SetEnabled(bool enabled)
{
  alarmIsEnabled = Config::AlarmEnabled = enabled;
  Config::Save();
}

void CheckActivation(byte Hour24, byte Minute)
{
  // check if it's time to turn the buzzer on
  if (alarmIsEnabled && Hour24 == Config::AlarmHour24 && Minute == Config::AlarmMinute && !alarmTriggered)
  {
    alarmTriggered = true;
    Buzzer(true);
  }
  else
    alarmTriggered = false;
}

bool CheckDeActivation()
{
  // Used with button press/touch, returns true if press absorbed and the buzzer killed
  if (buzzerIsActive)
  {
    Buzzer(false);
    return true;
  }
  return false;
}

}
