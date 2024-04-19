del ..\Data_Large.h
echo #pragma once >> ..\Data_Large.h
python encode_regions.py LargeDigit .\480x320\LargeDigit.bmp >> ..\Data_Large.h
python encode_regions.py SmallDigit .\480x320\SmallDigit.bmp >> ..\Data_Large.h
python encode_regions.py SmallChar  .\480x320\SmallChar.bmp  >> ..\Data_Large.h
python encode_regions.py VerySmallChar  .\480x320\VerySmallChar.bmp  >> ..\Data_Large.h
python encode_regions.py Moon  .\480x320\Moon.bmp  >> ..\Data_Large.h
python encode_regions.py Degrees  .\480x320\Degrees.bmp  >> ..\Data_Large.h
python encode_regions.py Sun  .\480x320\Sun.bmp  >> ..\Data_Large.h
python encode_regions.py Cloud  .\480x320\Cloud.bmp  >> ..\Data_Large.h
python encode_regions.py Storm  .\480x320\Storm.bmp  >> ..\Data_Large.h
python encode_blocks.py Text  .\480x320\Text.bmp  >> ..\Data_Large.h