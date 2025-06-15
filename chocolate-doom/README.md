## Chocolate Doom port

Port of [Chocolate Doom](https://github.com/chocolate-doom/chocolate-doom).

### Build

1. Download specific sources and apply patches:  
`$ make -f helper.make all patch`

2. Use the same instructions as for `fpdoom`. To select a game to build: use `GAME=name`, where name is `doom`, `heretic`, `hexen` or `strife`.

* The Strife port is not ready yet.

### Chocolate Doom options

* `-iwad <filename>` - specify wad file.

* `-mb N.NN` - set the cache size to N megabytes.

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
| #              | fly up/jump        |
| *              | fly down           |
| 0              | next item          |
| 8              | use item           |
| Power + Center | use                |
| Power + Up     | run on/off         |
| Power + Down   | open/close menu    |
| Power + L/R    | prev/next weapon   |
| Power + LSoft  | zoom in            |
| Power + RSoft  | zoom out           |
| Power + Dial   | map                |
| Power + 0      | gamma (0..4)       |
| Power + 1,2,3  | I, D, Q            |
| Power + 4,5,6  | K, F, A            |

* Heretic and Hexen are modified so that IDDQD/IDKFA cheats work like in Doom, IDKFA also gives all items.

