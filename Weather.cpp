#include <Arduino.h>

// BMP280 or SPL06-00x pressure and temperature sensor
// * * N O T E * *   
// BMP280 not supported because we can't use Wire, using SoftwareI2C instead

//#define SENSOR_BMP
#ifdef SENSOR_BMP
#include <BMP280_DEV.h> 
#else
#include "SPL06_I2C.h"
#endif

#include "Clock.h"
#include "Config.h"
#include "RTC.h"
#include "Weather.h"

namespace Weather// FOR ENTERTAINMENT ONLY!
{

// This code emulates the Zambretti Forecaster (https://en.wikipedia.org/wiki/Zambretti_Forecaster)  
// It's not going to be accurate, consider it entertainment; don't use it to guide planting your crops.
// The forecaster is a physical, analog, device consisting of three disks. 
// Dialing in wind direction and barometric pressure at sea level reveals letters in three windows, Rising, Falling and Steady.
// These relate to the pressure trend over the last three hours; Steady is less than 1.6hPa change.
// The Rising and Falling windows have arrows marked W and S for Winter and Summer.
// The "forecast" is found by reading the letter in the appropriate window, at the appropriate arrow, if applicable.
// On the back of the device is a table of forecasts corresponding to letters, for example, A is "Settled fine".
//
// This is implemented below as follows:
// The pressure read from the sensor is first transformed:
//  * to sea level, if CONFIG_ALTITUDE_METERS is defined
//  * to local range if CONFIG_MIN_PRESSURE_HPA & CONFIG_MAX_PRESSURE_HPA are defined
//    Note that the Forecaster is based on UK conditions, the above step (unused!) is perhaps able to compensate slightly.
// The result is used as the current pressure, and the trend is calculated from the reading 3 hours ago.
// Readings are taken on the hour and half hour.
// The conversion of the current pressure to a letter, for a given trend, is done via the p<Trend>PressureTable[]'s below.
// Summer/Winter is applied as an extra step forward or backward in those tables.
// Wind is ignored.
// The forecast is shown by the clock as either the text, or an icon.
// For the icon, the assumption is that they go from left to right from good to bad, and so do the letters A-Z
// All pressures are in deca Pascals, dPa = hPa*10  In other words, 1 DP of pressure in hPa. Temperatures are in C

typedef int16_t tPressure;  // dPa

// I2C pressure and temperature sensor
#ifdef SENSOR_BMP
BMP280_DEV bmp280;
#endif

const tPressure kNullPressure = 0;  // Don't have a value
const tPressure kPressureTrendThreshold = 16; // i.e. 1.6hPa
// The algorithm works with pressures in this range
const tPressure ZambrettiMinPressure =  9470;
const tPressure ZambrettiMaxPressure = 10500;

int loopMinute = -9999;
int currentTemperature = 0;
tPressure currentPressure = kNullPressure;  // unadjusted
tPressure adjustedPressure = kNullPressure;  // adjusted for MSL and range. Used in forecast
char currentForecastLetter = '?';
char currentTrendLetter = '?';

#define NUM_READINGS 6  // 3 hours worth, every half hour
tPressure pressureReadings[NUM_READINGS];  // [0] is oldest. Adjusted
uint32_t kReadingTimeoutMS = 50UL;

// A '\n' denotes a line break
//            123456789012345678901234 (24 chars per line)
#define FORECASTS \
    FCAST(A, "Settled fine") \
    FCAST(B, "Fine weather") \
    FCAST(C, "Becoming fine") \
    FCAST(D, "Fine\n" \
             "becoming less settled") \
    FCAST(E, "Fine\n" \
             "possible showers") \
    FCAST(F, "Fairly fine\n" \
             "improving") \
    FCAST(G, "Fairly fine\n" \
             "possible showers early") \
    FCAST(H, "Fairly fine\n" \
             "showery later") \
    FCAST(I, "Showery early\n" \
             "improving") \
    FCAST(J, "Changeable\n" \
             "mending") \
    FCAST(K, "Fairly fine\n" \
             "showers likely") \
    FCAST(L, "Rather unsettled\n" \
             "clearing later") \
    FCAST(M, "Unsettled\n" \
             "probably improving") \
    FCAST(N, "Showery\n" \
             "bright intervals") \
    FCAST(O, "Showery\n" \
             "becoming more unsettled") \
    FCAST(P, "Changeable\n" \
             "some rain") \
    FCAST(Q, "Unsettled\n" \
             "short fine intervals") \
    FCAST(R, "Unsettled\n" \
             "rain later") \
    FCAST(S, "Unsettled\n" \
             "rain at times") \
    FCAST(T, "Very unsettled\n" \
             "finer at times") \
    FCAST(U, "Rain at times\n" \
             "worse later") \
    FCAST(V, "Rain at times\n" \
             "becoming very unsettled") \
    FCAST(W, "Rain\n" \
             "at frequent intervals") \
    FCAST(X, "Very unsettled\n" \
             "rain") \
    FCAST(Y, "Stormy\n" \
             "possibly improving") \
    FCAST(Z, "Stormy\n" \
             "much rain")
// Multi-str. Single string const with multiple strings, 0 delimited. double 0 at the end
#define FCAST(_letter, _str) _str "\0"
const char pForecastMStr[] PROGMEM = FORECASTS;

const char* GetForecastStr(char letter)
{
  // returns the forecast corresponding to the letter, from the mstr above, or null
  if (::isalpha(letter))
  {
    const char* pStr = pForecastMStr;
    while (letter-- != 'A')
      while (pgm_read_byte_near(pStr++))
        ;
    return pStr;
  }
  return nullptr;
}

bool ReadPressure(tPressure& P)
{
  // read raw pressure. BMP BLOCKS
  float p;
#ifdef SENSOR_BMP
  bmp280.startForcedConversion();
  uint32_t startMS = millis();
  while (!bmp280.getPressure(p))
  {
    if ((millis() - startMS) > kReadingTimeoutMS)
    {
      bmp280.stopConversion();
      return false;
    }
  }  
#else
  p = SPL06_I2C::GetPressurePa()/100.0;  // returns mB == hPa
#endif  
  P = 10.0*p + 0.5;
  return true;
}

bool ReadTemperature(int& T)
{
  // read raw temperature. BMP BLOCKS
  float t;
#ifdef SENSOR_BMP
  bmp280.startForcedConversion();
  uint32_t startMS = millis();
  while (!bmp280.getTemperature(t))
  {
    if ((millis() - startMS) > kReadingTimeoutMS)
    {
      bmp280.stopConversion();
      return false;
    }
  }
#else
  t = SPL06_I2C::GetTemperatureC();
#endif
  T = t + 0.5;
  return true;
}

tPressure AdjustedPressure(tPressure pressure)
{
  // Adjust to sea level
#ifdef CONFIG_ALTITUDE_METERS  
  float pressure_hPa = pressure/10.0;
  return (tPressure)(0.5 + 10*pressure_hPa * pow(1.0 - (0.0065 * CONFIG_ALTITUDE_METERS) / (currentTemperature + (0.0065 * CONFIG_ALTITUDE_METERS) + 273.15), -5.257));
#else
  return pressure;
#endif  
}

// 
#define PRESSURE_OFFSET 947 // tables below store the difference from this, in a byte, hPa, at MSL
#define P(_p) ((char)((_p) - PRESSURE_OFFSET))

// Rising pressure
const char pRisingPressureTable[]  PROGMEM = {    'A',     'B',     'C',     'F',     'G',     'I',     'J',     'L',     'M',     'Q',     'T',     'Y',     'Z',     '\0',
                                              P(1030), P(1022), P(1012), P(1007), P(1000), P( 995), P( 990), P( 984), P( 978), P( 970), P( 965), P( 959), P( 947)};
// Falling pressure
const char pFallingPressureTable[] PROGMEM = {    'A',     'B',     'D',     'H',     'O',     'R',     'U',     'V',     'X',     '\0',
                                              P(1050), P(1040), P(1024), P(1018), P(1010), P(1004), P( 998), P( 991), P( 985)};
// Steady pressure
const char pSteadyPressureTable[]  PROGMEM = {    'A',     'B',     'E',     'K',     'N',     'P',     'S',     'W',     'X',     'Z',     '\0',
                                              P(1033), P(1023), P(1014), P(1008), P(1000), P( 994), P( 989), P( 981), P( 974), P( 960)};

// Seasons
//                    DNOSAJJMAMFJ- 
#define JUN_JUL_AUG 0b0000111000000
#define DEC_JAN_FEB 0b1000000000110
#ifdef CONFIG_NORTHERN_HEMISPHERE
bool IsWinter() { return DEC_JAN_FEB & (1 << rtc.m_Month); }
bool IsSummer() { return JUN_JUL_AUG & (1 << rtc.m_Month); }
#else
bool IsWinter() { return JUN_JUL_AUG & (1 << rtc.m_Month); }
bool IsSummer() { return DEC_JAN_FEB & (1 << rtc.m_Month); }
#endif

tPressure ReadTablePressure(const char* pPressures)
{
  // read a pressure from one of the tables above
  return 10*(pgm_read_byte_near(pPressures) + PRESSURE_OFFSET);
}

char LookupForecast(tPressure currentPressureMSL, const char* pTable, int8_t seasonAdjustment)
{
  // pTable is <letters><NUL><offset pressures>
  // Returns the letter corresponding to the table pressure closest to currentPressure
  // seasonAdjustment is +1/0/-1
  const char* pLetters = pTable;
  const char* pPressures = pTable + strlen_P(pTable) + 1;
  tPressure tablePressure = ReadTablePressure(pPressures++);
  if (currentPressureMSL > tablePressure)
    return ' '; // outside range
  const char* pClosestLetter = pLetters++;
  tPressure minDiff = abs(currentPressureMSL - tablePressure);

  while (pgm_read_byte_near(pLetters))
  {
    tablePressure = ReadTablePressure(pPressures);
    tPressure diff = abs(currentPressureMSL - tablePressure);
    if (diff < minDiff)
    {
      minDiff = diff;
      pClosestLetter = pLetters;
    }
    pLetters++;
    pPressures++;
  }
  if (currentPressureMSL < tablePressure)
    return ' ';  // outside range
    
  if (seasonAdjustment == +1 && pgm_read_byte_near(pClosestLetter + 1))
    pClosestLetter++;
  else if (seasonAdjustment == -1 && pClosestLetter != pTable)
    pClosestLetter--;

  return pgm_read_byte_near(pClosestLetter);
}

void Init()
{
  // N/A values
  for (int i = 0; i < NUM_READINGS; i++)
    pressureReadings[i] = kNullPressure;
#ifdef SENSOR_BMP
  bmp280.begin(BMP280_I2C_ALT_ADDR);
#else
  SPL06_I2C::Init();
#endif  
  ReadTemperature(currentTemperature);
  ReadPressure(currentPressure);
}

void Loop()
{
  // The full time must be read before calling, called once per minute
  // Takes a pressure readings every half hour: on the hour and half past
  // Pressure is adjusted
  //   to MSL
  //   for range
  // Updates the forecast based on the current adjusted pressure and the trend from the most recent reading and one 3 hours ago (adjusted)
  // Based on
  // https://communities.sas.com/t5/Streaming-Analytics/Zambretti-Algorithm-for-Weather-Forecasting/td-p/679487
  // or https://integritext.net/DrKFS/zambretti.htm
  
  if (loopMinute != rtc.m_Minute)
  {
    loopMinute = rtc.m_Minute;
    ReadTemperature(currentTemperature);
    if (loopMinute == 0 || loopMinute == 30)
    {
      ReadPressure(currentPressure);
      adjustedPressure = AdjustedPressure(currentPressure);
      
#if defined(CONFIG_MIN_PRESSURE_HPA) && defined(CONFIG_MAX_PRESSURE_HPA)
      // Map pressure to Zambretti range
      adjustedPressure = map(adjustedPressure, CONFIG_MIN_PRESSURE_HPA*10, CONFIG_MAX_PRESSURE_HPA*10, ZambrettiMinPressure, ZambrettiMaxPressure);
#endif  
      
      for (int i = 1; i < NUM_READINGS; i++)
        pressureReadings[i-1] = pressureReadings[i];
      pressureReadings[NUM_READINGS - 1] = adjustedPressure;
      tPressure oldPressure = pressureReadings[0];
      
      currentForecastLetter = '?';
      if (oldPressure != kNullPressure)
      {
        if ((adjustedPressure - oldPressure) >= +kPressureTrendThreshold)
        {
          currentTrendLetter = 'R';
          currentForecastLetter = LookupForecast(adjustedPressure, pRisingPressureTable, IsSummer()?+1:0);
        }
        else if ((adjustedPressure - oldPressure) <= -kPressureTrendThreshold)
        {
          currentTrendLetter = 'F';
          currentForecastLetter = LookupForecast(adjustedPressure, pFallingPressureTable, IsWinter()?-1:0);
        }
        else
        {
          currentTrendLetter = 'S';
          currentForecastLetter = LookupForecast(adjustedPressure, pSteadyPressureTable, 0);
        }
      }
    }
  } 
}

char GetForecast()
{
  // '?' means N/A
  return currentForecastLetter;
}

int GetTemperature()
{
  return currentTemperature;
}

float GetPressure()
{
  return currentPressure/10.0;
}

void GetInfo(int16_t& currentRawP, int16_t& currentAdjP, int16_t& oldAdjP, char& trend)
{
  currentRawP = currentPressure;
  currentAdjP = adjustedPressure;
  oldAdjP = pressureReadings[0];
  trend =  currentTrendLetter;
}

}
