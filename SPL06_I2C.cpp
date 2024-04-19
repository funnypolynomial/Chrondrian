#include <Arduino.h>
#include "RTC.h"
#include "SPL06_I2C.h"

// A simple implementation of SPL06-007 pressure/temperature reading
// Based on https://github.com/rv701/SPL06-007, which at time of writing (Nov '23) is broken.
// Uses SoftwareI2C because the LCD shield uses SDA & SCL

namespace SPL06_I2C
{
enum RegisterIndices {PSR_B2, PSR_B1, PSR_B0,  
                      TMP_B2, TMP_B1, TMP_B0,   
                      PRS_CFG, TMP_CFG, 
                      MEAS_CFG, CFG_REG,   
                      COEF = 0x10};

// Oversampling
//  bits    value      scale    scale hex    
//  0b0000:    1x     524288   0x00080000
//  0b0001:    2x    1572864   0x00180000
//  0b0010:    4x    3670016   0x00380000
//  0b0011:    8x    7864320   0x00780000
//  Higher values use FIFO, untested

// 8x from above
#define OVERSAMPLE_BITS   0b0011
#define OVERSAMPLE_SCALE 7864320.0
  
void Write(uint8_t idx, uint8_t value)
{
  // Write the value at the given register/byte index
  softWire.beginTransmission(SPL06_I2C_ADDR);
  softWire.write(idx);
  softWire.write(value);
  softWire.endTransmission();  
}

uint8_t Read(uint8_t idx)
{
  // Return the value of the given register/byte index
  softWire.beginTransmission(SPL06_I2C_ADDR);
  softWire.write(idx); 
  softWire.endTransmission(/*false*/);
  
  softWire.requestFrom((uint8_t)SPL06_I2C_ADDR, (uint8_t)1);
  return softWire.read();
}

int32_t ReadValue(uint8_t I, uint8_t N, uint8_t M = 0)
{
  // Read a 24 or 16 bit value, MSB first, starting with the I'th register byte
  // Then, starting at bit M, extract N bits as a 2's complement value
  // Convert that to 32 bit 2's complement value

  // Deals with the general case of COEF values spread across multiple bytes
  // and also the simpler case of multi-byte values
  
  uint32_t val = 0UL;
  uint8_t P = (N > 16)?3:2;
  for (int idx = 0; idx < P; idx++)
    val = (val << 8) | Read(I + idx);
     
  uint32_t hiBit = 1UL << N;
  uint32_t mask  = hiBit - 1UL;
  hiBit >>= 1;
  
  // Shift down so M'th is least significant:
  val >>= M;
  // Mask off bits above N'th:
  val &= mask;
  
  // Sign-extend if -ve:
  if (val & hiBit)
    val |= 0xFFFFFFFFL & ~mask;
    
  return (int32_t)val;
}

void Init()
{
  // Init the device
  Write(PRS_CFG, OVERSAMPLE_BITS);               // 8x oversample, rate is N/A
  Write(TMP_CFG, 0b10000000 | OVERSAMPLE_BITS);  // External sensor, 8x oversample, rate is N/A
  Write(MEAS_CFG, 0b111);                        // continuous pressure and temperature reading
  Write(CFG_REG, 0x00);                          // no FIFO
}

// Note that we DON'T check *_RDY flags in MEAS_CFG
double GetTemperatureC()
{
  // temperature in Celcius
  double c0 = ReadValue(COEF + 0, 12, 4);
  double c1 = ReadValue(COEF + 1, 12);
  
  double scaledT = ReadValue(TMP_B2, 24)/OVERSAMPLE_SCALE;

  return ((c0/2.0) + (c1*scaledT));
}

double GetPressurePa()
{
  // pressure in Pascals
  double c00 = ReadValue(COEF +  3, 20, 4);
  double c10 = ReadValue(COEF +  5, 20);  
  double c01 = ReadValue(COEF +  8, 16);
  double c11 = ReadValue(COEF + 10, 16); 
  double c20 = ReadValue(COEF + 12, 16);
  double c21 = ReadValue(COEF + 14, 16);
  double c30 = ReadValue(COEF + 16, 16);
  
  double scaledT  = ReadValue(TMP_B2, 24)/OVERSAMPLE_SCALE;
  double scaledP  = ReadValue(PSR_B2, 24)/OVERSAMPLE_SCALE;
  
  return c00 + scaledP*(c10 + scaledP*(c20 + scaledP*c30)) + scaledT*(c01 + scaledP*(c11 + scaledP*c21));
}
  
};
