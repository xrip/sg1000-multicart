#include <hardware/gpio.h>
#include <hardware/clocks.h>

// Pico pin usage definitions

#define A0_PIN    0
#define A1_PIN    1
#define A2_PIN    2
#define A3_PIN    3
#define A4_PIN    4
#define A5_PIN    5
#define A6_PIN    6
#define A7_PIN    7
#define A8_PIN    8
#define A9_PIN    9
#define A10_PIN  10
#define A11_PIN  11
#define A12_PIN  12
#define A13_PIN  13
#define A14_PIN  14
#define A15_PIN  15
#define D0_PIN   16
#define D1_PIN   17
#define D2_PIN   18
#define D3_PIN   19
#define D4_PIN   20
#define D5_PIN   21
#define D6_PIN   22
#define D7_PIN   23
#define MEMR_PIN  24
#define MEMW_PIN  25
#define MREQ_PIN  26
#define CEROM2_PIN  27
#define DSRAM_PIN  28
#define IOR_PIN  29

// Pico pin usage masks

#define A0_PIN_MASK     0x00000001L
#define A1_PIN_MASK     0x00000002L
#define A2_PIN_MASK     0x00000004L
#define A3_PIN_MASK     0x00000008L
#define A4_PIN_MASK     0x00000010L
#define A5_PIN_MASK     0x00000020L
#define A6_PIN_MASK     0x00000040L
#define A7_PIN_MASK     0x00000080L
#define A8_PIN_MASK     0x00000100L
#define A9_PIN_MASK     0x00000200L
#define A10_PIN_MASK    0x00000400L
#define A11_PIN_MASK    0x00000800L
#define A12_PIN_MASK    0x00001000L
#define A13_PIN_MASK    0x00002000L
#define A14_PIN_MASK    0x00004000L
#define A15_PIN_MASK    0x00008000L
#define D0_PIN_MASK     0x00010000L
#define D1_PIN_MASK     0x00020000L
#define D2_PIN_MASK     0x00040000L
#define D3_PIN_MASK     0x00080000L
#define D4_PIN_MASK     0x00100000L
#define D5_PIN_MASK     0x00200000L  // gpio 21
#define D6_PIN_MASK     0x00400000L
#define D7_PIN_MASK     0x00800000L

#define MEMR_PIN_MASK   0x01000000L //gpio 24
#define MEMW_PIN_MASK   0x02000000L
#define MREQ_PIN_MASK   0x04000000L  //gpio 26
#define CEROM2_PIN_MASK 0x08000000L
#define DSRAM_PIN_MASK  0x10000000L
#define IOR_PIN_MASK    0x20000000L

// Aggregate Pico pin usage masks
#define ALL_GPIO_MASK  	0x3FFFFFFFL
#define BUS_PIN_MASK    0x0000FFFFL
#define DATA_PIN_MASK   0x00FF0000L
#define FLAG_MASK       0x2F000000L
#define ROM_MASK ( MREQ_PIN_MASK  )
#define ALWAYS_IN_MASK  (BUS_PIN_MASK | FLAG_MASK)
#define ALWAYS_OUT_MASK (DATA_PIN_MASK | DOUTE_PIN_MASK)

#define SET_DATA_MODE_OUT   gpio_set_dir_out_masked(DATA_PIN_MASK)
#define SET_DATA_MODE_IN    gpio_set_dir_in_masked(DATA_PIN_MASK)


#include <stdlib.h>
#include <string.h>

#include "menu_rom.h"
#include "roms/roms.h"
#include "hardware/timer.h"
#include "hardware/structs/vreg_and_chip_reset.h"

volatile uint8_t __uninitialized_ram(rom_index);
uint8_t * ROM;
volatile uint8_t *rom_slot1;
volatile uint8_t *rom_slot2;
volatile uint8_t *rom_slot3;

static void reset_sega() {
    rom_slot1 = ROM;
    rom_slot2 = ROM;
    rom_slot3 = ROM;

    for (int i = 0; i < 5; i++) {
        while (!(gpio_get_all() & MEMR_PIN_MASK));
        SET_DATA_MODE_OUT;
        gpio_put_masked(DATA_PIN_MASK, 0xc7 << 16);
        while (!(gpio_get_all() & MEMR_PIN_MASK));
        SET_DATA_MODE_IN;
    }
}

void __not_in_flash_func(run)() {
    while (1) {
        while (gpio_get_all() & MREQ_PIN_MASK); //memr = b5 mreq=b10
        const uint32_t pins = gpio_get_all(); // re-read for SG-1000;
        const uint16_t address = pins & BUS_PIN_MASK;
        if (!(pins & MEMR_PIN_MASK)) {
            uint8_t value;

            if (address <= 1024) {
                value = ROM[address];
            } else if (address < 0x4000) {
                value = rom_slot1[address];
            } else if (address < 0x8000) {
                value = rom_slot2[address];
            } else if (address < 0xC000) {
                value = rom_slot3[address];
            } else {
                continue;
            }
            SET_DATA_MODE_OUT;
            gpio_put_masked(DATA_PIN_MASK, value << 16);
            SET_DATA_MODE_IN;

        }
        else if (!(pins & MEMW_PIN_MASK)) {
            const uint8_t value = (gpio_get_all() & DATA_PIN_MASK) >> 16;
            // ROM[address] = (gpio_get_all() & DATA_PIN_MASK) >> 16;
            const uint8_t page = value & 0x1f; // todo check rom size
            switch (address) {
                // Rom select from our menu
                case 0xFFF:
                    rom_index = value;
                    memcpy(ROM, roms[value].data, roms[value].size);
                    reset_sega();
                    break;
                case 0xFFFD:
                    rom_slot1 = ROM + page * 0x4000;
                break;
                case 0xFFFE:
                    rom_slot2 = ROM + page * 0x4000 - 0x4000;
                break;
                case 0xFFFF:
                    rom_slot3 = ROM + page * 0x4000 - 0x8000;
                break;
            }
        }
    }
}


void main() {
    hw_set_bits(&vreg_and_chip_reset_hw->vreg, VREG_AND_CHIP_RESET_VREG_VSEL_BITS);
    busy_wait_us(33);
    set_sys_clock_khz(400 * 1000, true);

    ROM = (uint8_t *) malloc(256 << 10);
    memcpy(ROM, menu_rom, 16 << 10);

    // recalc menu_rom checksum
    unsigned int checksum = 0;
    for (int i = 0; i<0x3ff0; i ++ ) {
        checksum += menu_rom[i];
    }
    ROM[0x3ff0+10] = checksum & 0x00FF;
    ROM[0x3ff0+11] = checksum >> 8;

    // Update game list in menu_rom
    #define ROM_NAME_LENGTH 31
    ROM[0x4000] = rom_count;
    for (int i = 0; i< rom_count; i++) {
        memcpy(&ROM[0x4001 + i * ROM_NAME_LENGTH], roms[i].name, ROM_NAME_LENGTH);
    }

    gpio_init_mask(ALL_GPIO_MASK);
    gpio_set_dir_in_masked(ALWAYS_IN_MASK);

    reset_sega();
    run();
}

