## FPBuild: Build Engine for feature phones

Port of games on the Build Engine. Using [jfbuild](https://github.com/jonof/jfbuild) code by Jonathon Fowler.

### Build

1. Download specific sources and apply patches:  
`$ make -f helper.make all patch`

2. Use the same instructions as for `fpdoom`. To select a game to build: use `GAME=duke3d` (Duke Nukem 3D), `GAME=sw` (Shadow Warrior) or `GAME=blood` (Blood).

* The patch for NBlood is based on [NBlood-Amiga](https://github.com/BSzili/NBlood-Amiga).

### Build Engine options

* `-cachesize N`, set the cache size to N kilobytes. Shadow Warrior and Blood run with freezes on phones with 4MB memory due to frequent loading of textures. The default cache size leaves some memory for allocations, you can try increasing it until you encounter out of memory errors.

* `-playanm 1`, to enable "video" playback (requires a lot of cache, you need an 8 MB phone).

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
| #              | jump               |
| *              | crouch             |
| 0              | next item          |
| 8              | use item           |
| Power + Center | use                |
| Power + Up     | run on/off         |
| Power + Down   | open/close menu    |
| Power + L/R    | prev/next weapon   |
| Power + LSoft  | zoom in            |
| Power + RSoft  | zoom out           |
| Power + Dial   | map                |
| Power + 0      | gamma (0..7)       |

### Issues

* Make sure the `.grp` filename is in all lowercase or uppercase, otherwise it will not work with `libc_server` running on Linux.

* Very low frame rate for a few seconds at the beginning of the level.

* Saved games depend on specific compile-time constants, which are reduced to fit the game in the small RAM of a feature phone. So saved games are incompatible with other versions of the game.

* Previews of saved games often glitches, most likely unloaded from memory due to lack of free cache.

* The fifth episode of "Duke Nukem 3D 20th Anniversary Edition" is not supported, `jfduke3d` does not support it. However, `duke3d.grp` from "20th Anniversary Edition" is supported, but works like "Atomic Edition" with four episodes. Just don't copy ".con" scripts to the working directory.

