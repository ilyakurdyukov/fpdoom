## Snes9x port

Port of Snes9x v1.43. This older version is more lightweight and has 8-bit rendering mode.

### Build

1. Download specific sources and apply patches:  
`$ make -f helper.make all patch`

2. Use the same instructions as for `fpdoom`. The `USE_16BIT={0|1}` option selects 8 or 16-bit rendering mode. 8-bit mode is faster, but doesn't handle various graphical effects (that many games don't use). 16-bit mode is slower but provides accurate rendering.

### Game controls

| Key(s)         | Action             |
|----------------|--------------------|
| Left Soft Key  | select             |
| Right Soft Key | start              |
| Direction Pad  | movement           |
| D-pad Center   | B                  |
| Dial Key       | A                  |
| 2, 4, 5, 6     | up/left/down/right |
| 1, 3           | TL, TR             |
| *, 0, #        | A, B, A            |
| 7, 8, 9        | X, Y, X            |
| Power + LSoft  | reset              |
| Power + Dial   | exit               |
| Power + Up     | reset              |
| Power + Down   | exit               |
| Power + Left   | select             |
| Power + Right  | start              |
| Power + Center | B                  |
| Power + 0      | force save SRAM    |

* The reset/exit keys must be held for 1 second to activate. This is to prevent accidental pressing.

### Issues

* SC6531 with 312MHz can emulate regular games at 1x speed. SC6530/SC6531E with 208MHz will skip frames.

* SA1 - disabled.

* SuperFX works, but very slow.

* C4/DSP1/DSP4 - disabled, because emulation uses floating point operations that are not supported by the phone's CPU. This can be solved by rewriting to integer calculations or using floating point emulation.

* Some game cartridges have SRAM for saves, this memory is saved on exit or if you force save SRAM. If the device is turned off, the SRAM changes will be lost.

* The FAT32 file system driver can't create files with long names, if you want to save SRAM the ROM file name must be in DOS 8.3 format or you need to create an empty file with the ROM name and the `.srm` extension in advance.

