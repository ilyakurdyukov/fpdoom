## gnuboy port

Port of [gnuboy](https://github.com/rofl0r/gnuboy).

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

* Monochrome screens are not supported yet.

* Some game cartridges have SRAM for saves, this memory is saved on exit or if you force save SRAM. If the device is turned off, the SRAM changes will be lost.

* The FAT32 file system driver can't create files with long names, if you want to save SRAM the ROM file name must be in DOS 8.3 format or you need to create an empty file with the ROM name and the `.sav` extension in advance.

