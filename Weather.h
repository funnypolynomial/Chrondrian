#pragma once

namespace Weather
{
  // FOR ENTERTAINMENT ONLY!
  void Init();
  void Loop();
  char GetForecast();
  int GetTemperature();
  float GetPressure(); // hPa
  const char* GetForecastStr(char letter);
  void GetInfo(int16_t& currentRawP, int16_t& currentAdjP, int16_t& oldAdjP, char& trend);
};
