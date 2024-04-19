# Chrondrian
My recreation of a colourful clock with date, time, temperature, moonphase, forecast and alarm.

![DSC06071](https://github.com/funnypolynomial/Chrondrian/assets/5882685/e592bd60-b2d9-4640-8945-7163bda00755)

I saw a similar product online and liked the look of the colored areas and the way the non-active LCD segments were visible. I created my own take on it, with different form factor, using a custom PCB, Arduino Nano, 480x320 LCD, SPL06 sensor, RTC etc.

The graphics are done by creating coloured BMP files represeting, for example, a 7-segment digit. Each segment has a different colour and some Python code converts them into run-length-encoded data arrays which are included in the sketch.

![LargeDigit](https://github.com/funnypolynomial/Chrondrian/assets/5882685/b1f59f6e-bdc4-450a-84f8-2e59ae1eb8ae)

The weather "forecast" is done using the Zambretti Forecaster algorithm, displayed as either an icon or the Zambretti text.

The assembly is a stack of two identical boards, one is used as an adaptor connecting a Nano to the LCD shield.  The other has RTC, sensor, buzzer and buttons.  An Acrylic sheet protects the LCD. The boards are separated with brass stand-offs.

![20240120_134238](https://github.com/funnypolynomial/Chrondrian/assets/5882685/76cec86b-d9dc-4b4e-837b-7b7d6d57254e)
![DSC06068](https://github.com/funnypolynomial/Chrondrian/assets/5882685/e380781c-11be-429d-80ce-9860e72d1614)

The board is [here](https://oshwlab.com/funnypolynomial/chrondrian-final). Pins.h has an ASCII schematic.

