# Chrondrian
My recreation of a colourful clock with date, time, temperature, moonphase, forecast and alarm.

![DSC06071](https://github.com/funnypolynomial/Chrondrian/assets/5882685/e592bd60-b2d9-4640-8945-7163bda00755)

I saw a similar product online and liked the look of the colored areas and the way the non-active LCD segments were visible. I created my own take on it, with different form factor, using a custom PCB, Arduino Nano, 480x320 LCD, SPL06 sensor, RTC etc.

The graphics are done by creating coloured BMP files representing, for example, a 7-segment digit. Each segment has a different colour and some Python code converts them into run-length-encoded data arrays which are included in the sketch.  See the resources sub-directory.

![LargeDigit](https://github.com/funnypolynomial/Chrondrian/assets/5882685/b1f59f6e-bdc4-450a-84f8-2e59ae1eb8ae)
  (Colour variations within the segments indicate areas for block compression vs line compression.)

The weather "forecast" is done using the [Zambretti Forecaster](https://en.wikipedia.org/wiki/Zambretti_Forecaster) algorithm, displayed as either an icon or the Zambretti text.

![chron01](https://github.com/funnypolynomial/Chrondrian/assets/5882685/02dabd2c-81c1-4637-9b7c-885040665394)
![chron02](https://github.com/funnypolynomial/Chrondrian/assets/5882685/4ca5747a-da41-4b77-8f0b-abc350fbca8b)

The assembly is a stack of two identical boards, one is used as an adaptor connecting a Nano to the LCD shield and has the RTC and buzzer.  The other has the sensor, buttons etc.  An Acrylic sheet protects the LCD. The boards are separated with brass stand-offs and connected by a header/socket strip.

![20240120_134238](https://github.com/funnypolynomial/Chrondrian/assets/5882685/76cec86b-d9dc-4b4e-837b-7b7d6d57254e)
![DSC06068](https://github.com/funnypolynomial/Chrondrian/assets/5882685/e380781c-11be-429d-80ce-9860e72d1614)

The board is [here](https://oshwlab.com/funnypolynomial/chrondrian-final). Pins.h has an ASCII schematic.

The code can also run on a 320x240 LCD and use touch instead of physical buttons.

![chron04](https://github.com/funnypolynomial/Chrondrian/assets/5882685/29ea543e-d7e9-4f0b-bbb5-f447474b32ef)
![chron03](https://github.com/funnypolynomial/Chrondrian/assets/5882685/6343eee9-6f38-4401-8094-364569efdb3c)


