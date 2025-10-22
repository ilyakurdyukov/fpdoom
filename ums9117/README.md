## FPDoom instructions for the UMS9117 chipset

Here are the differences from phones on the SC6531 chipset.

There is no support for reading NAND flash yet, but the binaries require the `pinmap` and `keymap` configs from the original firmware.

So, first you need to dump the kernel part of the firmware from the phone (hints [here](https://github.com/ilyakurdyukov/spreadtrum_flash?tab=readme-ov-file#instructions-4g-feature-phones)):

```
./spd_dump \
	keep_charge 1  fdl fdl1.bin 0x6200 \
	blk_size 0x1000  fdl fdl2.bin 0x801M \
	read_flash 0x80000003 0 auto kernel.bin
```

* `fdl1.bin` and `fdl2.bin` here are the files from the firmware update file.

Then process the kernel dump using the `fphelper_t117` tool (sources are [here](https://github.com/ilyakurdyukov/spreadtrum_flash/tree/main/fphelper_t117)).

```
./fphelper_t117 kernel.bin extract
```

This should extract the `pinmap.bin` and `keymap.bin` files, which must be placed in `workdir` for the example below.

### How to run

Use the [spreadtrum_flash](https://github.com/ilyakurdyukov/spreadtrum_flash) tool with the `t117_fdl1.bin` built from the same [repository](https://github.com/ilyakurdyukov/spreadtrum_flash/tree/main/t117_fdl). Note that the FDL load addresses are different.

```
$ ./spd_dump fdl t117_fdl1.bin 0x6200 fdl fpdoom.bin 0x801M
$ cd workdir && ../libc_server -- --bright 50 --rotate 3 doom
```

### Build

When building binaries for UMS9117, add the `T117=1` option to make.
