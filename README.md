# Sega SG-1000 / MARK III / PAL MASTER SYSTEM multicart
Project is heavely based on 
https://github.com/aotta/SD-1000 and uses it's PCB

A no-frills multicart for Sega SG-1000, Mark III, and Master System consoles using only an RP2040 microcontroller - no additional chips required!

## Features

- Works with Sega SG-1000, Mark III, and PAL Master System consoles
- Supports multiple ROM files up to 256KB total
- Built-in game selection menu
- ROM banking support for larger games
- Bare-minimum hardware design - just an RP2040 and passive components
- No EEPROM, CPLD, or additional logic ICs needed

## How It Works

This implementation uses the RP2040's GPIO pins to directly interface with the console's address and data buses. The RP2040 monitors the console's memory requests and responds with the appropriate ROM data in real-time.

- Address lines A0-A15 are connected to GPIO pins 0-15
- Data lines D0-D7 are connected to GPIO pins 16-23
- Control signals (MEMR, MEMW, MREQ, etc.) connected to GPIO pins 24-29
- The RP2040 runs at 400MHz to ensure adequate performance for bus timing

The software includes a menu ROM that allows you to select which game to play. When a selection is made, the appropriate ROM is loaded into memory and bank switching is configured as needed.

## Adding ROM Files

The project includes a Python script that automatically processes ROM files and generates the necessary C header file. Follow these steps to add your own games:

1. Place your Sega ROM files (`.sg` for SG-1000 or `.sms` for Master System ROMs) in the `roms/` directory
2. Run the provided Python script from the `roms/` directory:
   ```
   cd roms
   python make.py
   ```
3. The script will:
    - Scan for all `.sg` and `.sms` files in the directory
    - Generate a `roms.h` header file containing all ROM data
    - Format filenames to fit in the menu
    - Calculate the appropriate sizes for each ROM

### Script Details

The script automatically:
- Formats each filename to fit the 30-character limit in the menu
- Converts ROM binary data to C arrays
- Creates a structured array of all ROMs with their metadata
- Sets the ROM count for the menu system

## Hardware Requirements

- PURPLE chinese RP2040 clone that have pins 23,25 and 29 available, offical green or other black chinese RP2040s wont work in case of different pinout
- Diode
- A button
- SD-1000 pcb from https://github.com/aotta/SD-1000

## Pin Connections
You can use and breadboard PCB with 2.54 connector with following wiring:

| Signal | RP2040 GPIO |
|--------|-------------|
| A0-A15 | GPIO 0-15   |
| D0-D7  | GPIO 16-23  |
| MEMR   | GPIO 24     |
| MEMW   | GPIO 25     |
| MREQ   | GPIO 26     |
| CEROM2 | GPIO 27     |
| DSRAM  | GPIO 28     |
| IOR    | GPIO 29     |

## Building the Project

1. Clone this repository
2. Add your ROM files to the `roms/` directory
3. Update `roms.h` using the script as described above
4. Build using the Raspberry Pi Pico SDK:
   ```
   mkdir build
   cd build
   cmake ..
   make
   ```
5. Flash the resulting `.uf2` file to your RP2040 board

## Using the Multicart

1. Insert the multicart into your Sega console
2. Power on the console
3. Use the directional pad and buttons to navigate the menu
4. Select a game to play
5. Reset the console to return to the menu

## Limitations

- ROM size is limited to 256KB
- Only SEGA mappers supported
- No save game support (no SRAM emulation)

## ROM Banking

Currently only max 256Kb roms with SEGA mappers are supported.
For ROMs larger than 16KB, the cart uses bank switching at the following addresses:
- `0xFFFD`: Select 16KB bank for 0x0000-0x3FFF (slot 1)
- `0xFFFE`: Select 16KB bank for 0x4000-0x7FFF (slot 2)
- `0xFFFF`: Select 16KB bank for 0x8000-0xBFFF (slot 3)

## Acknowledgments

This project builds on the knowledge and work of the Sega console homebrew community. Special thanks to those who documented the Sega cartridge interfaces and memory mapping.

## License

This project is provided for educational and personal use only. All game ROMs are the property of their respective owners.

---

Feel free to contribute to this project or report issues in the GitHub repository!