# STM32-based Voltage Meter Firmware for Multiple Hardware Platforms

This repository contains firmware for STM32 MCU-based voltage meters,
which can be easily expanded to different hardware platforms.

## General Build Instructions

You will need the following packages (Debian/Ubuntu):

* build-essential
* gcc-arm-none-eabi
* git
* dialog

Depending on how you will program the firmware into the MCU, you will need:

* openocd
* stlink-tools
* dfu-util
* gdb-multiarch

This project uses `libopencm3`. It will be automatically checked out during the building process.
To build the firmware, run `make select` to select a target board, then type `make`.
To flash it, connect the board to a debugger, or UART (for ISP), or USB (for DFU), and then run `make flash`.
For ISP/DFU, remember to set BOOT0 (and BOOT1 if available) on the boards properly.

Under the `boards` folder there are Makefiles for each target board.
These files determine how the master Makefile will flash the MCU.

## Special Information for the CPT Field Probe

These probes are compact battery-powered voltage meters designed to
measure wireless power transfer at microwatt- to sub-watt-level (depending on the load).
Their form factor reduces stray coupling and falsely high readings.
The probe also reserved space for a full-/half-wave rectifier,
an adjustable load, and the CPT RX matching network.
The voltage measurement can be trimmed and calibrated against an accurate instrument.
The maximum input voltage is around 50 V with the 2M/100k resistive divider,
depending on the load, filtering capacitor rating, and the battery voltage
(as the VDDA is connected to battery without LDO -- the VDDA is calibrated against STM32's internal bandgap).

The average current consumption of these probes is under 150 microampere (see `boards/cpt-fieldprobe.md`).
A typical CR2032 battery has around 220 mAh, thus one battery can support 2 months of continuous, uninterrupted operation.

The hardware design files are located in `pcb/cpt-fieldprobe`.
To flash the probe, connect a SWD debugger (ST-Link) to the test points on the back of the PCB via enameled wires.

The probe was part of the following work. Please kindly consider citing the paper as shown if you find it useful:

```
@inproceedings{Capttery,
 author = {Zhang, Chi and Kumar, Sidharth and Bharadia, Dinesh},
 title = {{Capttery: Scalable Battery-like Room-level Wireless Power}},
 booktitle = {Proc. of ACM MobiSys},
 year = {2019},
}
```
