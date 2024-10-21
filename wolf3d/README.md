## Wolfenstein 3D port

Port of the [Wolf4SDL](https://github.com/KS-Presto/Wolf4SDL).

### Build

1. Download specific sources and apply patches:  
`$ make -f helper.make all patch`

2. Use the same instructions as for `fpdoom`. Use `GAMEVER` option to select the game version, for example `GAMEVER="CARMACIZED UPLOAD"` for shareware v1.4, read [Wolf4SDL/version.h](https://github.com/KS-Presto/Wolf4SDL/blob/master/version.h) for a description of all options.

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
| Power + Dial   | map                |
| Power + 4,5,6  | G, Y, M            |

* Cheats: G + Y toggles God Mode, M + Y gives all items.

### Issues

* Each version of the game requires its own binary.

* Saved games and config are not compatible with other ports.

* For compatibility with Linux, the USB build will try to open file names in uppercase if opening in lowercase fails.

