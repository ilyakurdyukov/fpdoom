## SD card boot (UMS9117)

This is a bootloader that needs to be written to the beginning of the SD card. The code is executed when the phone is powered/rebooted, if the keys at positions (0,0), (0,1) and (1,0) is hold down. If a FAT32 formatted SD card is inserted, then the binary `fpbin_t117/fpmain.bin` is executed. The command-line arguments for the application is read from the file `fpbin_t117/config.txt`. This application can be either one of the games or a menu for selecting games (`fpmenu`).

* The key for booting from USB is (0,0). The key positions are the intersections of traces on the PCB, not the physical buttons you see.

* The keys (0,1) and (1,0) are the `bl_update` keys, `fphelper_t117` will print them in the log when analyzing the firmware kernel.

* The bootloader must be signed for phones with Secure Boot enabled, for now there no way to boot from an SD card for such devices.

### How to install

When writing the bootloader to the SD card, you need to preserve the MBR partitions. I don't know how to do the same on Windows.

```sh
# the SD card reader device
# for USB card readers, it is /dev/sdX
# for internal laptop card readers, it might be /dev/mmcblkX
dev=/dev/mmcblk0
# backup the original MBR
sudo dd if=$dev bs=512 count=64 > mbr_orig.bin

tmp=mbr_new.bin
cp sdboot_t117.bin $tmp
# copy the MBR partitions
dd if=mbr_orig.bin of=$tmp seek=440 skip=440 bs=1 count=72 conv=notrunc

# write the result to the SD card
sudo dd if=$tmp of=$dev bs=512
```

The `pinmap.bin` and `keymap.bin` extracted from the phone firmware must be copied to the `fpbin_t117` directory.

