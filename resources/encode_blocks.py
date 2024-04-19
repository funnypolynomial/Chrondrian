#!/usr/bin/python
import sys
import os
from PIL import Image

# Encode multiple detailed blocks, coloured as regions
# Width, Height, data..
# data byte is bit sequence


def ByteStr(b):
    return "0x" + hex(256 + b)[3:].upper()

    
def Encode(block, colour):
    global bytes
    bytes = 0

    # find dims
    xMin = yMin = +9999
    xMax = yMax = -9999
    for y in range(bmp.height):
        for x in range(bmp.width):
            if bmp.getpixel((x, y)) == colour:
                xMin = min(xMin, x)
                yMin = min(yMin, y)
                xMax = max(xMax, x)
                yMax = max(yMax, y)
    if xMin == +9999:
        return
        
    # write it   
    sys.stdout.write("static const uint8_t " + name + str(block) + "[] PROGMEM =\n")
    sys.stdout.write('{\n')
    sys.stdout.write("  " + ByteStr(xMax-xMin+1))
    sys.stdout.write(", ")
    sys.stdout.write(ByteStr(yMax-yMin+1))
    sys.stdout.write(", \n  ")
    bytes += 2

    # encode body
    byte = 0
    bit = 1
    for y in range(yMin, yMax + 1):
        for x in range(xMin, xMax + 1):
            if bmp.getpixel((x, y)) == colour:
                byte |= bit
            bit *= 2
            if bit == 256:
                sys.stdout.write(ByteStr(byte) +", ")
                bytes += 1
                byte = 0
                bit = 1
                if bytes % 16 == 0:
                    sys.stdout.write("\n  ")
    if bit != 1:
        sys.stdout.write(ByteStr(byte) +", ")
        bytes += 1
    sys.stdout.write("\n};  // ")
    sys.stdout.write(str(bytes))
    sys.stdout.write(" bytes \n\n")

if len(sys.argv) != 3:
    sys.stdout.write("parameters: <segments name> <input image file>\n")
    exit()
name = sys.argv[1]
bmp = Image.open(sys.argv[2])
colours  = (
  (255, 0, 0),       #  0
  (0, 255, 0),       #  1
  (0, 0, 255),       #  2
                        
  (200, 0, 0),       #  3
  (0, 200, 0),       #  4
  (0, 0, 200),       #  5
                        
  (150, 0, 0),       #  6
  (0, 150, 0),       #  7
  (0, 0, 150),       #  8
                                    
  (255, 255,   0),   #  9
  (0,   255, 255),   # 10
  (255,  0,  255),   # 11
  
  (200, 200,   0),   # 12
  (0,   200, 200),   # 13
  (200,   0, 200),   # 14
  
  (175,   0,   0),   # 15
  )
bytes = 0
total_bytes = 0
sys.stdout.write("// ---------------------------------\n")
for block in range(len(colours)):  
    Encode(block, colours[block])
    total_bytes += bytes
sys.stdout.write("// (")
sys.stdout.write(str(total_bytes))
sys.stdout.write(" bytes total)\n\n")    

