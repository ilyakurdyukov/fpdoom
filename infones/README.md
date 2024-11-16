## InfoNES port

Port of the [InfoNES](https://github.com/jay-kumogata/InfoNES).

I hope that the author of InfoNES will pay attention to the huge number of bugs in his code, such as writing element 0x4015 to an array of size 0x18. Also that "NES Emulator in C++" is complete nonsense, the code will not become written in C++ if you change the extension of the C source files to .cpp.

### Build

1. Download specific sources and apply patches:  
`$ make -f helper.make all patch`

2. Use the same instructions as for `fpdoom`.

### Game controls

| Key(s)         | Action             |
|----------------|--------------------|
| Left Soft Key  | select             |
| Right Soft Key | start              |
| Direction Pad  | movement           |
| D-pad Center   | A                  |
| Dial Key       | A                  |
| 2, 4, 5, 6     | up/left/down/right |
| *, 0, #        | A                  |
| 1, 3, 7, 8, 9  | B                  |
| Power + LSoft  | start              |
| Power + Dial   | exit               |
| Power + Up     | reset              |
| Power + Down   | exit               |
| Power + Left   | select             |
| Power + Right  | start              |
| Power + Center | B                  |
| Power + 0      | force save SRAM    |

* The reset/exit keys must be held for 1 second to activate. This is to prevent accidental pressing.

### Issues

* InfoNES is far from the best emulator, half of the games don't work or are too glitchy.

* Most games that use iNES Mapper 004 are unplayable due to serious graphical glitches.

* This emulator only supports NTSC 60Hz mode.

* Some game cartridges have SRAM for saves, this memory is saved on exit or if you force save SRAM. If the device is turned off, the SRAM changes will be lost.

* The FAT32 file system driver can't create files with long names, if you want to save SRAM the ROM file name must be in DOS 8.3 format or you need to create an empty file with the ROM name and the `.srm` extension in advance.

