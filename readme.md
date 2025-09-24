# ESP32 AT host bridge arduino software for RP2040
this repository contains arduino software for RP2040 that bridges USB CDC commands to ESP32 with AT firmware in SPI or SDIO mode.
## but why?
while ESP32 AT is commonly known, SPI or SDIO AT is not popular option because there are not many examples even though SPI and SDIO can achieve amazing throughput.
### but why arduino?
to make SPI and SDIO AT popular, i needed to make them more easy to use. advanced users probably are able to transform these codes for toolchain they want.
### but why RP2040?
RP2040 is cheap, and is a good gateway to switching to diversity of microcontrollers, lets say, SAMD51, or STM32.
# AT support table
|MCU|Interface|status|note|
|---|---|---|---|
|ESP32|SPI|N||
|ESP32|SDIO|Y|[1]|
|ESP32-C2|SPI|Y|[2]|
|ESP32-C2|SDIO|N||
|ESP32-C3|SPI|Y|[1]|
|ESP32-C3|SDIO|N||
|ESP32-C5|SPI|N|[3]|
|ESP32-C5|SDIO|N|[3]|
|ESP32-C6|SPI|Y|[2]|
|ESP32-C6|SDIO|Y|[2]|
|ESP32-S2|SPI|N||
|ESP32-S2|SDIO|N||

[1] get daily [AT builds](https://github.com/espressif/esp-at/actions)\
[2] you will need to use custom AT build or [prebuilt AT by me](https://github.com/wb1016/esp-at-autobuild/actions)\
[3] development in progress

# Arduino sketch support status
## Single-SPI
Arduino sketch: Supported
Tested MCU: ESP32-C2 ESP32-C3
## Dual-SPI
Arduino sketch: Upcoming
Tested MCU:
## Quad-SPI
Arduino sketch: Upcoming
Tested MCU:
## SDIO
Arduino sketch: Upcoming
Tested MCU:

# Pin assignments
## SPI
|Name|RP2040|ESP32-C2|ESP32-C3|ESP32-C6|
|---|---|---|---|---|
|MOSI/D0|IO3|IO7|IO7|IO18|
|MISO/D1|IO4|IO2|IO2|IO20|
|D2[1]|IO6|IO8|IO8|IO22|
|D3[1]|IO7|IO9|IO9|IO2|
|SCK|IO2|IO6|IO6|IO19|
|CS|IO5|IO10|IO10|IO23|
|HANDSHAKE|IO8|IO3|IO3|IO21|

[1] required for only in QSPI mode

## SDIO
|Name|RP2040|ESP32|ESP32-C6|
|---|---|---|---|
|CLK|IO16|IO14|IO19|
|CMD|IO17|IO15|IO18|
|DAT0|IO18|IO2|IO20|
|DAT1|IO19|IO4|IO21|
|DAT2[1]|IO20|IO12|IO22|
|DAT3[1]|IO21|IO13|IO23|

[1] required for only 4-bit mode
