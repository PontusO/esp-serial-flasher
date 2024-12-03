# Challenger board flash loader utility

## Overview

This program is used to flash an Espressif SoC (target) from an iLabs Challenger board (host) using `esp_serial_flasher`.
`uart1` is dedicated for communication with the Espressif SoC, whereas, either `uart0` or USB can be used for debug output.

The following steps are performed in order to re-program the targets memory:

1. Peripherals are initialized.
2. Host puts the slave device into boot mode and tries to connect by calling `esp_loader_connect()`.
3. Then `esp_loader_flash_start()` is called to enter the flashing mode and erase the amount of memory to be flashed.
4. The `esp_loader_flash_write()` function is called repeatedly until the whole binary image is transferred.

In addition to the steps mentioned above, `esp_loader_change_transmission_rate()` is called after connection is established in order to increase the flashing speed. The bootloader is also capable of detecting the baud rate during connection phase, however, it is recommended to start at lower speed and then use dedicated command to increase the baud rate. This does not apply for the ESP8266, as its bootloader does not support this command, therefore, baud rate can only be changed before the connection phase in this case.

## Hardware Required

* A Challenger board with an on board ESP32C3 or ESP32C6.
* The Challenger board needs to have at least 4MByte of flash or more.

## Hardware connection

This section describes how the different Challenger boards connect to the on board ESP32 module.

Challenger+ RP2350 WIFI6/BLE5

|  Challenger+   | ESP32 (slave) |
|:--------------:|:-------------:|
|       14       |      IO0      |
|       15       |      RST      |
|        5       |      RX0      |
|        4       |      TX0      |

## Build and flash

First, either export the SDK location in your shell:

```bash
export PICO_SDK_PATH=<sdk_location>
```

or set the `PICO_SDK_FETCH_FROM_GIT` CMake variable for the SDK to be pulled from GitHub.

Create and navigate to the example `build` directory:
s
```bash
mkdir build && cd build
```

Run cmake (with appropriate parameters) and build: 
```bash
cmake .. -DPICO_BOARD=ilabs_challenger_rp2350_wifi_ble -DPICO_PLATFORM=rp2350-arm-s -DPICO_STDIO_USB=1 && cmake --build .
```

> Note: CMake 3.13 or later is required.

Binaries to be flashed are placed in the `binaries.c` file for each possible target and converted to C arrays. Flash integrity verification is enabled by default.

For more details regarding `esp_serial_flasher` configuration and Raspberry Pi Pico support, please refer to the top level [README.md](../../README.md).

Finally, flash the board by connecting your Challenger board to your PC, with the `BOOTSEL` button pressed and copy over the `.uf2` file from the example `build` directory to the Mass Storage Device the Pico presents itself as.

To see the debug output, connect to the boards virtual com port with a serial monitor (e.g. using minicom):

```bash
minicom -b 115200 -o -D /dev/ttyACM0
```

Here is the example's console output:

```
iLabs ESP32-C2/C3/C6 flash utility V1.0
ESP-AT interpreter V4.0.0.0 for Challenger Boards.
Baudrate: 2000000 baud.
Connected to target
Transmission rate changed.
Writing bootloader... Erasing flash (this may take a while)...
DEBUG: Flash size detection failed, falling back to defaultStart programming
Progress: 100 %
Finished programming
Writing partition table... Erasing flash (this may take a while)...
Start programming
Progress: 100 %
Finished programming
Writing initial OTA data... Erasing flash (this may take a while)...
Start programming
Progress: 100 %
Finished programming
Writing AT custom data... Erasing flash (this may take a while)...
Start programming
Progress: 100 %
Finished programming
Writing manufacturer non volotile data... Erasing flash (this may take a while)...
Start programming
Progress: 100 %
Finished programming
Writing esp-at stack... Erasing flash (this may take a while)...
Start programming
Progress: 100 %
Finished programming
Done!
********************************************
*** Logs below are print from slave .... ***
********************************************
�ESP-ROM:esp32c6-20220919
Build:Sep 19 2022
rst:0x1 (POWERON),boot:0xc (SPI_FAST_FLASH_BOOT)
SPIWP:0xee
mode:DIO, clock div:2
load:0x4086c410,len:0xd2c
load:0x4086e610,len:0x2df0
load:0x40875728,len:0x17d8
entry 0x4086c410
I (23) boot: ESP-IDF v5.1.2-dirty 2nd stage bootloader
I (24) boot: compile time Dec  2 2024 14:36:23
I (24) boot: chip revision: v0.1
I (27) boot.esp32c6: SPI Speed      : 80MHz
I (32) boot.esp32c6: SPI Mode       : DIO
I (36) boot.esp32c6: SPI Flash Size : 4MB
I (41) boot: Enabling RNG early entropy source...
I (47) boot: Partition Table:
I (50) boot: ## Label            Usage          Type ST Offset   Length
I (57) boot:  0 otadata          OTA data         01 00 0000d000 00002000
I (65) boot:  1 phy_init         RF data          01 01 0000f000 00001000
I (72) boot:  2 nvs              WiFi data        01 02 00010000 0000e000
I (80) boot:  3 at_customize     unknown          40 00 0001e000 00042000
I (87) boot:  4 ota_0            OTA app          00 10 00060000 001d0000
I (95) boot:  5 ota_1            OTA app          00 11 00230000 001d0000
I (102) boot: End of partition table
I (107) boot: No factory image, trying OTA 0
I (111) esp_image: segment 0: paddr=00060020 vaddr=42150020 size=30530h (197936) map
I (161) esp_image: segment 1: paddr=00090558 vaddr=40800000 size=0fac0h ( 64192) load
I (176) esp_image: segment 2: paddr=000a0020 vaddr=42000020 size=147104h (1339652) map
I (453) esp_image: segment 3: paddr=001e712c vaddr=4080fac0 size=09f6ch ( 40812) load
I (463) esp_image: segment 4: paddr=001f10a0 vaddr=40819a30 size=03bcch ( 15308) load
I (468) esp_image: segment 5: paddr=001f4c74 vaddr=50000000 size=00068h (   104) load
I (475) boot: Loaded app from partition at offset 0x60000
I (511) boot: Set actual ota_seq=1 in otadata[0]
I (512) boot: Disabling RNG early entropy source...
no external 32k oscillator, disable it now.
at param mode: 1

ready
```

The loader will flash the on board LED during the process and stop as soon as the new firmware has been flashed. If something goes wrong during the programming the LED blinking will stop.