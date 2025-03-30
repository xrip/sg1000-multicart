cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(SD1000_cart C CXX ASM)

set(CMAKE_BUILD_TYPE "MinSizeRel")

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()



add_executable(${PROJECT_NAME})

# Build roms
add_custom_target(rom_list COMMAND python make.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/roms
)

# Build menu rom first
add_custom_target(menu_rom COMMAND make clean && make && python bin2c.py menu.sg ../menu_rom.h
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/menu
)
add_dependencies(${PROJECT_NAME} rom_list menu_rom)

pico_define_boot_stage2(slower_boot ${PICO_DEFAULT_BOOT_STAGE2_FILE})
target_compile_definitions(slower_boot PRIVATE PICO_FLASH_SPI_CLKDIV=4)
pico_set_boot_stage2(${PROJECT_NAME} slower_boot)

# 16megs of flash on purple pico clones
target_compile_definitions(${PROJECT_NAME} PRIVATE
        PICO_PRINTF_ALWAYS_INCLUDED=0
        PICO_STACK_SIZE=1024
        PICO_HEAP_SIZE=1024
        PICO_PANIC_FUNCTION=
)

pico_set_float_implementation(${PROJECT_NAME} none) # size optimizations
pico_set_double_implementation(${PROJECT_NAME} none) # size optimizations
pico_set_printf_implementation(${PROJECT_NAME} none)

add_compile_options(-flto -fwhole-program -ffunction-sections -fdata-sections -O3) # size optimizations

target_sources(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/sega_cart.c
)

target_include_directories(${PROJECT_NAME} PUBLIC
)

target_link_libraries(${PROJECT_NAME} PUBLIC
        pico_runtime
        -Wl,--wrap=atexit # size optimizations
)

# use custom linker script
pico_set_linker_script(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/memmap_sd1000.ld)


# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(${PROJECT_NAME})
target_link_options(${PROJECT_NAME} PRIVATE -Xlinker --print-memory-usage --data-sections --function-sections)

