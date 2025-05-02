#include <hardware/gpio.h>
#include <hardware/clocks.h>
#include <tusb.h>
#include "fatfs_disk.h"

// Pico pin usage masks
#define MEMR_PIN_MASK   0x01000000L //gpio 24
#define MEMW_PIN_MASK   0x02000000L
#define MREQ_PIN_MASK   0x04000000L  //gpio 26

#define CEROM2_PIN  27

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

// #include "menu_rom.h"
// #include "roms/roms.h"
#include "ff.h"
#include "hardware/timer.h"
#include "hardware/structs/vreg_and_chip_reset.h"
// #include "rom.h"

// extern uint8_t ROM[];

static void reset_sega() {
    for (int i = 0; i < 5; i++) {
        while (!(gpio_get_all() & MEMR_PIN_MASK));
        SET_DATA_MODE_OUT;
        gpio_put_masked(DATA_PIN_MASK, 0xc7 << 16);
        while (!(gpio_get_all() & MEMR_PIN_MASK));
        SET_DATA_MODE_IN;
    }
}
extern unsigned long flash_fs_get_FAT_sector(uint16_t fat_sector);
[[noreturn]]  void __time_critical_func(run)() {
    volatile uint8_t *banks[48];

    FATFS fs;
    FIL file;
    FRESULT res;
    if (!fatfs_is_mounted()) {
        mount_fatfs_disk();
    }

    const char pathname[256] = "spy.sms";

    f_mount(&fs, "", 1);  // Mount default drive
    FILINFO fileinfo;
    f_stat(pathname, &fileinfo);

    f_open(&file, pathname, FA_READ);


    uint8_t * ROM = (uint8_t *) flash_fs_get_FAT_sector(get_file_start_sector(&file));

    const uint32_t rom_mask  = (fileinfo.fsize / 1024) - 1;
    for (int i = 0; i < 48; i++) {
        banks[i] = ROM  + __fast_mul(i & rom_mask, 1024);
    }

    while (1) {
        while (gpio_get_all() & MREQ_PIN_MASK); //memr = b5 mreq=b10
        const uint32_t pins = gpio_get_all(); // re-read for SG-1000;
        const uint16_t address = (uint16_t) pins;

        if (__builtin_expect(!(pins & MEMR_PIN_MASK),1)) {
                SET_DATA_MODE_OUT;
                const uint8_t bank = address >> 10;
                if (bank < 48) {
                    gpio_put_masked(DATA_PIN_MASK, banks[bank][address & 1023] << 16);
                }
                SET_DATA_MODE_IN;
        } else if (__builtin_expect(!(pins & MEMW_PIN_MASK) && address >= 0xFFFD, 0)) {
                SET_DATA_MODE_IN;
                volatile const uint8_t value = (uint8_t)(gpio_get_all() >> 16) ;
                uint8_t  *bank_offset = ROM + ((value & 0x1f) << 14);
                switch (address) {
                    case 0xFFFD: {
#pragma GCC unroll(16)
                        for (uint8_t i = 1; i < 16; i++) {
                            banks[i] = bank_offset + __fast_mul(i, 1024);
                        }
                        break;
                    }
                    case 0xFFFE: {
#pragma GCC unroll(16)
                        for (uint8_t i = 0; i < 16; i++) {
                            banks[16 + i] = bank_offset + __fast_mul(i, 1024);
                        }
                        break;
                    }
                    case 0xFFFF: {
#pragma GCC unroll(16)
                        for (uint8_t i = 0; i < 16; i++) {
                            banks[32 + i] = bank_offset + __fast_mul(i, 1024);
                        }
                        break;
                }
            }
        }
    }
}


//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+
static inline void cdc_task(void)
{
    // connected() check for DTR bit
    // Most but not all terminal client set this when making connection
    // if ( tud_cdc_connected() )
    {
        // connected and there are data available
        if ( tud_cdc_available() )
        {
            // read data
            char buf[64];
            const size_t count = tud_cdc_read(buf, sizeof(buf));

            // Echo back
            // Note: Skip echo by commenting out write() and write_flush()
            // for throughput test e.g
            //    $ dd if=/dev/zero of=/dev/ttyACM0 count=10000
            tud_cdc_write(buf, count);
            tud_cdc_write_flush();
        }
    }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {
    if (!mount_fatfs_disk())
        create_fatfs_disk();
}

void main() {
    set_sys_clock_khz(266 * 1000, true);

    gpio_init_mask(ALL_GPIO_MASK);
    gpio_set_dir_in_masked(ALWAYS_IN_MASK);

    gpio_set_dir(CEROM2_PIN, GPIO_IN);

    while (to_ms_since_boot(get_absolute_time()) < 200) {
        if (gpio_get(CEROM2_PIN)) {
            reset_sega();
            run();
        }
    }

    tusb_init();
    while (1) {
        tud_task(); // tinyusb device task
        cdc_task();
    }
}

