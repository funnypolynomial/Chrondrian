del ..\Data_Small.h
echo #pragma once >> ..\Data_Small.h
python encode_regions.py LargeDigit .\320x240\LargeDigit.bmp >> ..\Data_Small.h
python encode_regions.py SmallDigit .\320x240\SmallDigit.bmp >> ..\Data_Small.h
python encode_regions.py SmallChar  .\320x240\SmallChar.bmp  >> ..\Data_Small.h
python encode_regions.py VerySmallChar  .\320x240\VerySmallChar.bmp  >> ..\Data_Small.h
python encode_regions.py Moon  .\320x240\Moon.bmp  >> ..\Data_Small.h
python encode_regions.py Degrees  .\320x240\Degrees.bmp  >> ..\Data_Small.h
python encode_regions.py Sun  .\320x240\Sun.bmp  >> ..\Data_Small.h
python encode_regions.py Cloud  .\320x240\Cloud.bmp  >> ..\Data_Small.h
python encode_regions.py Storm  .\320x240\Storm.bmp  >> ..\Data_Small.h
python encode_blocks.py Text  .\320x240\Text.bmp  >> ..\Data_Small.h