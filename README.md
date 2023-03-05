# AHT20
used https://github.com/Seeed-Studio/Seeed_Arduino_AHT20 library for sensor
other library: 
- https://github.com/cbm80amiga/Arduino_ST7789_Fast
- https://github.com/cbm80amiga/RREFont

 ST7789 240x240 IPS (without CS pin) connections (only 6 wires required): 
 #01 GND -> GND
 #02 VCC -> VCC (3.3V only!)
 #03 SCL  --|==2k2==|--> D13/PA5/SCK
 #04 SDA ---|==2k2==|--> D11/PA7/MOSI
 #05 RES ---|==2k2==|--> D9 /PA0 or any digital
 #06 DC  ---|==2k2==|--> D10/PA1 or any digital
 #07 BLK -> NC
