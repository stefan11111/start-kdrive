# start-kdrive

Usage:
```
start-kdrive [xinit args]
```

This executes
```
xinit -mouse evdev,,device=/dev/input/eventxx -keybd device=/dev/input/eventyy [xinit args]
```

Where xx and yy are replaced with the appropriate numbers
