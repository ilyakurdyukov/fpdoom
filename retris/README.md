## Retris port

Port of [Retris](https://github.com/ilyakurdyukov/retris).

### Build

1. Download specific sources and apply patches:  
`$ make -f helper.make all patch`

2. Use the same instructions as for `fpdoom`.

### Game controls

| Key(s)          | Action             |
|-----------------|--------------------|
| LSoft, 1        | rotate left        |
| RSoft, Up, 2, 3 | rotate right       |
| Left, 4, 7      | move left          |
| Right, 6, 9     | move right         |
| Down, 5         | move down          |
| Center, 0       | drop               |
| Power + Up      | restart            |
| Power + Down    | quit               |
| Power + Left    | rewind             |
| Power + Right   | pause              |
| Power + Center  | pause              |

* The restart/quit keys must be held for 1 second to activate. This is to prevent accidental pressing.

* The `--key_rep N` option sets key repeat rate in ms (default 100).
