## libc server

A helper application for running binaries in USB mode. Allows code running on the phone to read/write files from the working directory where `libc_server` is running on the PC.

### Usage

```
$ cd workdir
$ ../libc_server <libc_server args...> -- <system args...> <app name> <app args...>
```

`libc_server args...`: like `--verbose 2` to print all USB traffic.  
`--`: marks the end of `libc_server` args.
`system args...`: like `--bright 50 --rotate 3`, which help configure the device and display.  
`app name`: like `fpdoom`, this is `argv[0]` for `main()`.  
`app args...`: like `-playdemo demo1` for Doom.  
