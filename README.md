## FPDoom: Doom for feature phones

Currently only for feature phones based on the SC6531(E/DA) chipset.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, USE AT YOUR OWN RISK!

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

### List of tested models

| # | Name            |   Chip   |      LCD       | Boot Key |
|---|-----------------|----------|----------------|----------|
| 1 | F+ F256         | SC6531E  | GC9306 240x320 | *        |
| 2 | Digma Linx B241 | SC6531E  | ST7789 240x320 | center   |
| 3 | F+ Ezzy 4       | SC6531E  | GC9106 128x160 | 1        |
| 4 | Joy's S21       | SC6531DA | GC9108 128x160 | 0        |
| 5 | Vertex M115     | SC6531DA | ST7735 128x128 | up       |
| 6 | Vertex С323     | SC6531DA | GC9106 128x160 | 0        |
| 7 | Nobby 170B      | SC6531E  | GC9106 128x160 | #        |
| 8 | SW DZ09         | SC6531DA | GC9307 240x240 |          |
| 9 | Nokia TA-1174   | SC6531E  | ???    128x160 | 7        |

* Vertex M115, Nobby 170B, Nokia TA-1174: need keymap file
* Nobby 170B: use `--spi 1 --mac 0xa8` without `--rotate`
* Smart Watch DZ09: use `--spi 0 --lcd 0x9307`, no controls - you can only watch replays, no boot key - use boot cable
* Nokia TA-1174: use `--spi 1 --spi_mode 1 --lcd 0x9316`, also add `enddata 0` command for `spd_dump`, before `fdl` commands

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

* D-pad and keys 1-9 are rotated with the `--rotate` option (with screen or separately).
* For phones without D-pad Center key (Nobby 170B) - up/down keys are used for fire.
* If you don't like the controls - you can change it in [keytrn.c](fpdoom/keytrn.c).

