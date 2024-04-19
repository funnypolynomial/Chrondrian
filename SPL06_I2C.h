#pragma once

#define SPL06_I2C_ADDR 0x76

namespace SPL06_I2C  // SPL06-00x for Arduino, SoftwareI2C
{
  // Does not init SoftwareI2C.  Temperature & Pressure 8x oversampling
  void Init();
  double GetTemperatureC();
  double GetPressurePa();
};
