#pragma once

#define PIN_RTC_SDA     12
#define PIN_RTC_SCL     11

#define PIN_BTN_ADJ     A6
#define PIN_BTN_SET     A7

#define PIN_SW_ALARM    13
#define PIN_PWM_BUZZER  10  // Note: Buzzer is actually *active*, PWM is N/A

//   *** Schematic ***
//   
//                  LCD_RST etc                                +-DS3221-+
//   +--------------+++++-------+++-----------------+          |  RTC   |
//   |              |||||       ||| [GND],[5V],RST* |   [SDA]--+        |
//   |   [SET][ADJ] |||||       |||                 |   [SCL]--+        |
//   |  +--+----+---+++++-------+++-----+           |   [GND]--+        |
//   |  | A7   A6   A4-A0*              |           |   [5V]---+        |
//   |  |                      NANO     |   LCD     |          |        |
//   |  |                               |           |          +--------+
//   |  | D2-D9*     D10  D11  D12  D13 |           |
//   |  +-++++++-++---+----+----+----+--+           |          +-SPL06--+
//   |    |||||| || [BZR][SCL][SDA][ALA]            |          | Baro/  |
//   |    |||||| ||                                 |   [SDA]--+ Temp   |
//   +----++++++-++---------------------------------+   [SCL]--+        |
//        LCD_D2 etc                                    [GND]--+        |
//                                                      [5V]---+        |
//                                                             |        |
//                                                             +--------+
//
//    [BZR]----{Active Piezo}---------[GND]
//    
//    [ALA]----{Slide switch}---------[5V]
//             
//    [SET]------------------+   Set$
//                           |   ---
//             [5V]--{40kR}--+---   --[GND]
//    
//    [ADJ]------------------+   Adj$
//                           |   ---
//             [5V]--{40kR}--+---   --[GND]
//    
//                                                             
//   *:direct connection from Nano to sockets for pins on LCD shield
//   [XXX]'s are connected.
//   $:Set/Adj momentary on push-buttons
//  
