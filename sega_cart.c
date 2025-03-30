#include <hardware/gpio.h>
#include <hardware/clocks.h>

// Pico pin usage masks
#define MEMR_PIN_MASK   0x01000000L //gpio 24
#define MEMW_PIN_MASK   0x02000000L
#define MREQ_PIN_MASK   0x04000000L  //gpio 26

// Aggregate Pico pin usage masks
#define ALL_GPIO_MASK  	0x3FFFFFFFL
#define BUS_PIN_MASK    0x0000FFFFL
#define DATA_PIN_MASK   0x00FF0000L
#define FLAG_MASK       0x2F000000L
#define ALWAYS_IN_MASK  (BUS_PIN_MASK | FLAG_MASK)

#define SET_DATA_MODE_OUT   gpio_set_dir_out_masked(DATA_PIN_MASK)
#define SET_DATA_MODE_IN    gpio_set_dir_in_masked(DATA_PIN_MASK)


#include <stdlib.h>
#include <string.h>

#include "menu_rom.h"
#include "roms/roms.h"
#include "hardware/timer.h"
#include "hardware/structs/vreg_and_chip_reset.h"

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

            const uint8_t page = value & 0x1f; // todo check rom size
            switch (address) {
                // Rom select from our menu
                case 0xFFF:
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

