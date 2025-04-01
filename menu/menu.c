#include "SGlib.h"
#include "font.h"
#include "sega_logo.h"
/* define PSGPort (SDCC z80 syntax) */
__sfr __at 0x7F PSGPort;

/* macros for SEGA and SDSC headers */
#define SMS_BYTE_TO_BCD(n) (((n)/10)*16+((n)%10))

#define SMS_EMBED_SEGA_ROM_HEADER_16KB_REGION_CODE  0x4B
#define SMS_EMBED_SEGA_ROM_HEADER_16KB(productCode,revision)                                   \
 const __at (0x3ff0) unsigned char __SMS__SEGA_signature[16]={'T','M','R',' ','S','E','G','A', \
                                                                          0xFF,0xFF,0xFF,0xFF, \
                  SMS_BYTE_TO_BCD((productCode)%100),SMS_BYTE_TO_BCD(((productCode)/100)%100), \
      (((productCode)/10000)<<4)|((revision)&0x0f),SMS_EMBED_SEGA_ROM_HEADER_16KB_REGION_CODE}


volatile unsigned char __at(0xfff) selected; // Write ROM index here to launch it from RP2040

#define ITEMS_PER_PAGE 15
#define ROM_NUMBER 18*2
#define ROM_NAME_LENGTH 31
// ROM-stored Menu data:
// 0xfff -- selected game index, write here to launch
// 0x4000 -- total menu items
// 0x4001 -- 31byte for each menu item with \0
const __at(0x4000) unsigned char menu_item_num = ROM_NUMBER;
#if 1
const __at(0x4001) unsigned char menu_items[16384] = { 0 };
#else
const char menu_items[] = {
    "1213456789012345678901234567 1\0"
    "1213456789012345678901234567 2\0"
    "1213456789012345678901234567 3\0"
    "1213456789012345678901234567 4\0"
    "1213456789012345678901234567 5\0"
    "1213456789012345678901234567 6\0"
    "1213456789012345678901234567 7\0"
    "1213456789012345678901234567 8\0"
    "1213456789012345678901234567 9\0"
    "121345678901234567890123456 10\0"
    "121345678901234567890123456 11\0"
    "121345678901234567890123456 12\0"
    "121345678901234567890123456 13\0"
    "121345678901234567890123456 14\0"
    "121345678901234567890123456 15\0"
    "121345678901234567890123456 16\0"
    "121345678901234567890123456 17\0"
    "121345678901234567890123456 18\0"
    "! 13456789012345678901234567 1\0"
    "! 13456789012345678901234567 2\0"
    "! 13456789012345678901234567 3\0"
    "! 13456789012345678901234567 4\0"
    "! 13456789012345678901234567 5\0"
    "! 13456789012345678901234567 6\0"
    "! 13456789012345678901234567 7\0"
    "! 13456789012345678901234567 8\0"
    "! 13456789012345678901234567 9\0"
    "! 1345678901234567890123456 10\0"
    "! 1345678901234567890123456 11\0"
    "! 1345678901234567890123456 12\0"
    "! 1345678901234567890123456 13\0"
    "! 1345678901234567890123456 14\0"
    "! 1345678901234567890123456 15\0"
    "! 1345678901234567890123456 16\0"
    "! 1345678901234567890123456 17\0"
    "! 1345678901234567890123456 18\0",
};
#endif
const unsigned char sprite_pointer[16] = {
    0b00001000, //     *
    0b00001100, //     **
    0b00001110, //     ***
    0b11111111, //   ******
    0b00001110, //     ***
    0b00001100, //     **
    0b00001000, //     *
    0b00000000 //
};

void SG_print(const unsigned char *str) {
    while (*str) {
        SG_setTile(*str++);
    }
}

#define SG_printatXY(x,y, str) do{SG_setNextTileatXY(x,y); SG_print(str);}while(0)

static void draw_pointer(const unsigned char y) {
    SG_initSprites();
    SG_addSprite(4, (8 * 4) + y * 8, 0, SG_COLOR_CYAN);
    SG_finalizeSprites();
    SG_copySpritestoSAT();
}

void draw_page(const unsigned char page) {
    // clear items from 5 row
    SG_VRAMmemset(0x1800 + (4 << 5), 0, 32 * ITEMS_PER_PAGE);

    const unsigned char start = page * ITEMS_PER_PAGE;
    unsigned char end = (page + 1) * ITEMS_PER_PAGE;
    if (end > menu_item_num) end = menu_item_num;

    for (unsigned char i = start; i < end; i++) {
        SG_printatXY(2, 4 + (i - start), &menu_items[i * ROM_NAME_LENGTH]);
    }
}

static void setup(void) {
//    SG_VRAMmemsetW(0, 0x0000, 8192);
    SG_setBackdropColor(SG_COLOR_BLACK);

    SG_loadTilePatterns(sms_sg1000_cart_logo, 1, 192);

    SG_loadTilePatterns(devkitSMS_font__tiles__1bpp, 32, sizeof(devkitSMS_font__tiles__1bpp));
    SG_loadTilePatterns(devkitSMS_font__tiles__1bpp, 32 + 0x100, sizeof(devkitSMS_font__tiles__1bpp));
    SG_loadTilePatterns(devkitSMS_font__tiles__1bpp, 32 + 0x200, sizeof(devkitSMS_font__tiles__1bpp));

    // SG_loadTileColours but not uses rom
    // sega logo
    SG_VRAMmemset(0x2000, SG_COLOR_LIGHT_BLUE << 4, 200);
    /// rest of screen
    SG_VRAMmemset(0x2000 + 200, 0xF1, (1920 * 3) - 200);

    SG_loadSpritePatterns(sprite_pointer, 0, 8);


    draw_page(0);
    draw_pointer(0);

    // Draw SEGA logo
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 8; x++) {
            SG_setTileatXY(x + 1, y, y * 8 + (x + 1));
        }
    }

    SG_printatXY(10, 0, "MARK III/SMS/SG-1000");
    SG_printatXY(15, 1, "multicart");
    SG_printatXY(16, 2, "by xrip");

    SG_printatXY(3, 23, "<< prev page $ next page >> ");

    SG_displayOn();

}

static void play_jingle(void) {
    const unsigned char jingle[][3] = {
    {0b10000000 | (0x1ac & 0xf), (0x1ac >> 4), 1},  
    {0b10100000 | (0x153 & 0xf), (0x153 >> 4), 1},  
    {0b11100000 | (0x11d & 0xf), (0x11d >> 4), 60},  

    {0b10000000 | (0x0d6 & 0xf), (0x0d6 >> 4), 1},  
    {0b10100000 | (0x0aa & 0xf), (0x0aa >> 4), 1},  
    {0b11100000 | (0x08f & 0xf), (0x08f >> 4), 255},  

    {0b11100000 | (0x08f & 0xf), (0x08f >> 4), 255},  
    {0b11100000 | (0x08f & 0xf), (0x08f >> 4), 255},  
    };

    PSGPort = 0x90;  // Channel 1 Volume (Max)
    SG_waitForVBlank();
    PSGPort = 0xB0;  // Channel 2 Volume (Max)
    SG_waitForVBlank();
    PSGPort = 0xD0;  // Channel 3 Volume (Max)

    // Short delay to let PSG process volume settings
    SG_waitForVBlank();


    // Play each note
    for (unsigned char i = 0; i < 8; i++) {
        PSGPort = jingle[i][0];  // Send low byte of tone
        PSGPort = jingle[i][1];  // Send high byte of tone

        // Wait for the note duration
        for (unsigned char j = 0; j < jingle[i][2]; j++) {
            SG_waitForVBlank();
        }
    }


    // Silence PSG at the end
    PSGPort = 0x9F;  // Mute Channel 1
    PSGPort = 0xBF;  // Mute Channel 2
    PSGPort = 0xDF;  // Mute Channel 3
    PSGPort = 0xFF;  // Mute Channel 3
}


void main(void) {
    setup();

    unsigned int keys, previous_keys = 0;
    unsigned int selected_menu_item = 0;
    unsigned char current_page = 0;
    unsigned int key_hold_frame_counter = 0;

#define repeat_delay 10
    play_jingle();
    while (1) {
        keys = SG_getKeysStatus();

        const unsigned int items_in_page = (menu_item_num - current_page * ITEMS_PER_PAGE) > ITEMS_PER_PAGE ? ITEMS_PER_PAGE : (menu_item_num - current_page * ITEMS_PER_PAGE);
        if (selected_menu_item >= items_in_page) {
            selected_menu_item = items_in_page - 1;
        }

        // Up button pressed
        if ((keys & PORT_A_KEY_UP) && (key_hold_frame_counter % repeat_delay == 0 || !(previous_keys & PORT_A_KEY_UP))) {
            if (selected_menu_item == 0)
                selected_menu_item = items_in_page - 1;
            else
                selected_menu_item--;

            draw_pointer(selected_menu_item);
        }

        // Down button pressed
        if ((keys & PORT_A_KEY_DOWN) && (key_hold_frame_counter % repeat_delay == 0 || !(previous_keys & PORT_A_KEY_DOWN))) {
            selected_menu_item = (selected_menu_item + 1) % items_in_page;

            draw_pointer(selected_menu_item);
        }

        // Right button pressed to go to the next page
        if ((keys & PORT_A_KEY_RIGHT) && (key_hold_frame_counter % repeat_delay == 0 || !(previous_keys & PORT_A_KEY_RIGHT))) {
            if ((current_page + 1) * ITEMS_PER_PAGE < menu_item_num) {
                current_page++;
                selected_menu_item = 0;  // Reset selection to top of new page
                draw_page(current_page);
                draw_pointer(selected_menu_item);
            }
        }

        // Left button pressed to go to the previous page
        if ((keys & PORT_A_KEY_LEFT) && (key_hold_frame_counter % repeat_delay == 0 || !(previous_keys & PORT_A_KEY_LEFT))) {
            if (current_page > 0) {
                current_page--;
                selected_menu_item = 0;  // Reset selection to top of new page
                draw_page(current_page);
                draw_pointer(selected_menu_item);
            }
        }

        if ((keys & PORT_A_KEY_START) && !(previous_keys & PORT_A_KEY_START)) {
            selected = current_page * ITEMS_PER_PAGE + selected_menu_item;
        }
        previous_keys = keys;

        // Increment vsync_counter only if a key is being held
        if (keys & (PORT_A_KEY_UP | PORT_A_KEY_DOWN | PORT_A_KEY_LEFT | PORT_A_KEY_RIGHT)) {
            key_hold_frame_counter++;
        } else {
            key_hold_frame_counter = 0;
        }

        SG_waitForVBlank();

    }
}

SMS_EMBED_SEGA_ROM_HEADER_16KB(9999, 0);
