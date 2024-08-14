## SD card boot

This is a small section that is added to the free space in the firmware. The code is executed when the phone is powered/rebooted, if one of the keys at positions (0,1), (1,0) or (1,1) is pressed and a FAT32 formatted SD card is inserted, then the binary `fpbin/fpmain.bin` is executed. The command line to the application is read from the file `fpbin/config.txt`. This application can be either one of the games or a menu for selecting games (`fpmenu`).

* The key for booting from USB is (0,0). The key positions are the intersections of traces on the PCB, not the physical buttons you see.

### How to install

First you need to dump the phone firmware using the [spreadtrum_flash](https://github.com/ilyakurdyukov/spreadtrum_flash) tool. The most common flash memory size is 4MB, but expensive models can have 8 and 16MB. So far, the flash size cannot be determined automatically, so specify 16MB, if the size is less, the dump will simply be repeated in the file several times.

`./spd_dump fdl nor_fdl1.bin 0x40004000 read_flash 0x80000003 0 16M flash.bin`

Then use the `fphelper` utility from the `spreadtrum_flash` set:

`./fphelper flash.bin scan`

This will scan the firmware and among various information will tell you what the phone's boot key is, as well as the keys for booting from the SD card. For example:

```
0x71b0c: keymap, bootkey = 0x08 (LSOFT), sdboot keys = 0x2a 0x01 0x30 (STAR, DIAL, 0)
```

Then run:

`./fphelper flash.bin sdboot`

This will print instructions on how to flash `sdboot` to your phone.

### Build

Prebuilt binaries can be found in [Releases](https://github.com/ilyakurdyukov/spreadtrum_flash/releases).

Use the same instructions as for `fpdoom`. Add the `CHIP=N` option to make, where N is the chip type (1 = SC6531E, 2 = SC6531DA, 3 = SC6530), as a result you will get a binary `sdbootN.bin`, which you can use in the instructions that `fphelper` writes.

To run from SD card all applications must be compiled with the `LIBC_SDIO=3` option (by default USB mode is set, it is `LIBC_SDIO=0`).

* The chip independent version (`CHIP=0`) doesn't fit in 4KB, do not try to use it, it's unfinished.

### Issues

* To ensure fast reading of resources from the SD card, FAT32 cluster chains are cached, which takes 4 bytes per cluster. For example, if a file takes 40MB and the cluster size is 4KB, then 40KB of memory will be needed to store the cluster chain. For games on the Build engine, this can be significant on a phone with 4MB of RAM. If the clusters are arranged sequentially, then no memory is spent on storing the cluster chain. Defragment the file system to straighten the cluster chains.

