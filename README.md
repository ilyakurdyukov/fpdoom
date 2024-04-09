## FPDoom: Doom for feature phones

Currently only for feature phones based on the SC6530/SC6531(E/DA) chipset.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, USE AT YOUR OWN RISK!

### Other ports

Port of some games on Build Engine [here](fpbuild). Reuses code written for FPDoom.

### Build

1. `libc_server`: use the host compiler, based on a [spd_dump](https://github.com/ilyakurdyukov/spreadtrum_flash) tool, so build it with the same options.
2. `pack_reloc`: use the host compiler, the tool to extract and compress relocations from an Elf binary.
3. Download the [Doom](https://github.com/id-Software/DOOM) sources, and copy the `linuxdoom-1.10` directory as `doom_src`.
Apply the patch: `$ patch -d doom_src -p1 -i ../doom.patch`.
4. `fpdoom`: use compiler from Android NDK, build instructions are the same as for [custom_fdl](https://github.com/ilyakurdyukov/spreadtrum_flash/custom_fdl) from `spd_dump`.

* You can find prebuilt `fpdoom.bin`, and host binaries for Windows in [Releases](https://github.com/ilyakurdyukov/fpdoom/releases).

### How to run FPDoom

1. Figure out how to run `spd_dump` on your feature phone.
2. Create a working directory `workdir` and place the Doom resource file here, for example [doom1.wad](http://distro.ibiblio.org/pub/linux/distributions/slitaz/sources/packages/d/doom1.wad) from the shareware version of Doom 1.
3. Put these commands in a script, run it and then connect your phone:
```
$ ./spd_dump --wait 300 fdl nor_fdl1.bin 0x40004000 fdl fpdoom.bin ram
$ cd workdir && ../libc_server -- --bright 50 --rotate 3 doom
```

* `--bright X` is the phone screen brightness (X = 0..100).
* `--rotate S[,K]` is the screen/keypad rotation in 90 degress units (-1 or 3 = -90, 1 = +90, etc.)
All feature phone LCDs I've seen are vertical, if you have a phone with a horizontal display - that means it's a vertical LCD placed horizontally, so you need to use different S and K values.
* You can add extra options for Doom, for example `doom -timedemo demo1`.

### Possible problems

1. Message `!!! unknown LCD`, and before that `LCD: id = 0x000000` - SPI mode is used to connect the LCD. I can't handle it yet.
2. Message `!!! unknown LCD`, but some number shown in `LCD: id = 0x`. I can add support for this LCD if you send me a firmware dump from your phone.
3. Message `!!! keymap not found`, means the firmware scan algorithm can't find the keymap, so you can't use the keypad, only watch replays. I can extract the keymap if you send me a firmware dump. I can also add code that asks you to press the requested keys to create the keymap if this becomes a common problem.
4. Message `Error: W_InitFiles: no files found`, means that no `.wad` files were found. On Linux files must be named in lower case.

### List of tested models

|  # | Name              |   Chip   |      LCD       | Boot Key   |
|----|-------------------|----------|----------------|------------|
|  1 | F+ F256           | SC6531E  | GC9306 240x320 | *          |
|  2 | Digma Linx B241   | SC6531E  | ST7789 240x320 | center     |
|  3 | F+ Ezzy 4         | SC6531E  | GC9106 128x160 | 1          |
|  4 | Joy's S21         | SC6531DA | GC9108 128x160 | 0          |
|  5 | Vertex M115       | SC6531DA | ST7735 128x128 | up         |
|  6 | Vertex С323       | SC6531DA | GC9106 128x160 | 0          |
|  7 | Nobby 170B        | SC6531E  | GC9106 128x160 | #          |
|  8 | SW DZ09           | SC6531DA | GC9307 240x240 | none       |
|  9 | Nokia TA-1174     | SC6531E  | ST7735 128x160 | 7          |
| 10 | BQ 3586           | SC6531H  | R61529 320x480 | #          |
| 11 | Samsung B310E     | SC6530C  | ST7735 128x160 | center     |
| 12 | Fontel FP200      | SC6531DA | GC9106 128x160 | left soft  |
| 13 | Vertex D514       | SC6531E  | ???    240x320 | center     |
| 14 | Fly TS114         | SC6531E  | GC9306 240x320 | right soft |
| 15 | Energizer E12     | SC6531E  | GC9106 128x160 | 1          |
| 16 | Itel it5626       | SC6531DA | GC9307 240x320 | left soft  |
| 17 | Sunwind C2401     | SC6531E  | ST7789 240x320 | *          |
| 18 | DEXP SD2810       | SC6531E  | ???    240x320 | 2          |
| 19 | Nokia TA-1400     | SC6531F  | ???    240x320 | right soft |
| 20 | YX Q5 Kids Camera | SC6531DA | NV3023 128x160 | shutter    |
| 21 | Nomi i184         | SC6531DA | GC9102 128x160 | *          |
| 22 | Sigma IO67        | SC6531DA | GC9305 240x320 | dial       |

* Vertex M115, Nobby 170B, Nokia TA-1174, BQ 3586, Energizer E12, Sunwind C2401, DEXP SD2810, YX Q5 Kids Camera, Nomi i184, Sigma IO67: need keymap file
* Nobby 170B: use `--spi 1 --mac 0xa8` without `--rotate`
* Smart Watch DZ09: use `--spi 0 --lcd 0x80009307`, no controls - you can only watch replays, no boot key - use boot cable
* Nokia TA-1174: use `--spi 1 --spi_mode 1`, also add `end_data 0` command for `spd_dump`, before `fdl` commands
* BQ 3586: use `--bl_gpio 19` for backlight to work properly
* Fly TS114: use `--spi 1`
* Nokia TA-1400: use `--spi 1`, also add `end_data 0` command for `spd_dump`, before `fdl` commands
* YX Q5 Kids Camera: `--mac 0xa8` without `--rotate`

### Game controls

| Key(s)         | Action             |
|----------------|--------------------|
| Left Soft Key  | use                |
| Right Soft Key | open/close menu    |
| Direction Pad  | movement           |
| D-pad Center   | fire               |
| Dial Key       | run on/off         |
| 2, 4, 5, 6     | up/left/down/right |
| 1, 3           | strafe left/right  |
| 7, 9           | prev/next weapon   |
| Power + Center | use                |
| Power + Up     | run on/off         |
| Power + Down   | open/close menu    |
| Power + L/R    | prev/next weapon   |
| Power + LSoft  | zoom in            |
| Power + RSoft  | zoom out           |
| Power + Dial   | map                |

* D-pad and keys 1-9 are rotated with the `--rotate` option (with screen or separately).
* For phones without D-pad Center key (Nobby 170B) - up/down keys are used for fire.
* If you don't like the controls - you can change it in [keytrn.c](fpdoom/keytrn.c).
* Combinations with the power button are for the Children's Camera, which has only six keys. Don't hold down the power button for too long (7 seconds), as this may trigger the device to power off.

### Tweaks

* Use `--scaler 3` option to `libc_server` on 128x160 screens to scale down by 1.5x instead of 2x, this will use the entire screen area but decrease the horizontal viewing angle (-25% total).
* Use `--charger 2` option to `libc_server` so that the battery can be removed for the SC6531DA after the game is loaded, the game will continue to run on USB power.

