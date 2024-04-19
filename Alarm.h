#pragma once
namespace Alarm
{
    void Init();
    bool Loop();  // true if status changed
    void Buzzer(bool On);
    bool IsEnabled();
    void SetEnabled(bool enabled);
    void CheckActivation(byte Hour24, byte Minute);
    bool CheckDeActivation();
};
