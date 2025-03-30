import os
import glob

HEADER_FILE = "roms.h"
FILENAME_LENGTH = 30  # Fixed length for filename in RomEntry


def format_filename(filename):
    """Trims or pads the filename to exactly 31 characters."""
    if len(filename) > FILENAME_LENGTH:
        return filename[:FILENAME_LENGTH]  # Trim if too long
    return filename.ljust(FILENAME_LENGTH)  # Pad with spaces if too short


def process_files():
    files = sorted(glob.glob("*.sg") + glob.glob("*.sms"))
    rom_entries = []

    with open(HEADER_FILE, "w") as header:
        header.write("#ifndef ROMS_H\n#define ROMS_H\n\n")
        header.write('#include <stdint.h>\n\n')

        for idx, file in enumerate(files):
            var_name = f"flash_rom{idx:03d}"
            formatted_name = format_filename(file)

            with open(file, "rb") as f:
                data = f.read()

            # Generate C array
            c_array = ", ".join(f"0x{byte:02X}" for byte in data)
            rom_entries.append(f"{{ {len(data)}, \"{formatted_name}\\0\", {var_name} }}")

            # Write individual file data to header
            header.write(f"const unsigned char __attribute__((section(\".sd1000_rom\"))) {var_name}[] = {{\n    {c_array}\n}};\n\n")

        # Generate ROM list
        header.write(f"typedef struct {{ int size; char name[{FILENAME_LENGTH + 1}]; const unsigned char *data; }} RomEntry;\n\n")
        header.write(f"RomEntry roms[] = {{\n    {',\n    '.join(rom_entries)}\n}};\n")
        header.write(f"const int rom_count = {len(files)};\n\n")

        header.write("#endif // ROMS_H\n")


if __name__ == "__main__":
    process_files()
    print(f"{HEADER_FILE} generated successfully.")
