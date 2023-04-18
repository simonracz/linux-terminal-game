# Linux Terminal Game
Code created during recording the [Linux Terminal Game from Scratch](https://www.youtube.com/playlist?list=PLRWoC1qZt_WeCwCUEWxsSIpqfgg2qd9wD) video series.

# Videos
[Linux Terminal Game from Scratch I - Termios, ANSI Escape Codes](https://youtu.be/WvSOSyi5lWY)

[Linux Terminal Game from Scratch II - Game Loop, Rendering, Level Loading](https://youtu.be/D9SppXPZLU0)

[Linux Terminal Game from Scratch III - Gameplay](https://youtu.be/z-yqFVX_j_U)

[Linux Terminal Game from Scratch IV - Tests, Rendering, Colors, Animations](https://youtu.be/qMbB2wVVw-I)


# Requirements
Just a C compiler :)

# Build
```
gcc -std=gnu17 -Wall -Wextra -O2 ./game.c -o game
./game
```

# General Game Requirements
## Display Graphics
- Low resolution
- ASCII characters (subpixels, textures)
- Colors (foreground, background)
- Move the cursor's position freely

## Input Handling
- Canonical Mode - line based
- Non-Canonical Input Mode - More control over input
- RAW Input Mode
- Turn off Echo

## Sound and Music:
- :(
- Terminal Bell - NO

# Toolbox
- man 3 termios
- ANSII Escape Codes - https://en.m.wikipedia.org/wiki/ANSI_escape_code

# ANSI Escape Codes
## FG Color
Paremeters: R G B
```
printf "\x1B[:38;2;R;G;Bm"
```

## BG Color
Parameters: R G B
```
printf "\x1B[48;2;R;G;Bm"
```

## Moving the cursor
Move Cursor To (x, y) - Upper Left Corner is (1, 1)
Paremeters: X Y
```
printf "\x1B[Y;XH"
```

## Clear Screen
```
printf "\x1B[2J"
```

## Hide Cursor
```
printf "\x1B[?25l"
```

## Show Cursor
```
printf "\x1B[?25h"
```

## Reset Colors
```
printf "\x1B[m"
```

# Implementation Stages
## Stage 1
- Non-canonical input mode
- Turning off echo
- Proper reset on exit (both for normal exit and abnormal exit)
- Rudimentary game loop
- Grab the arrow keys
- Print them out (e.g. RIGHT)

## Stage 2
- Empty level with borders
- Player can walk around within the borders

## Stage 3
- Optimize rendering
- Optimize game loop

## Stage 4
- Load up level from a file

## Stage 5
- Gameplay

## Stage 6
- Tests

## Stage 7
- Rendering
- Colors
- Animations?

# Gameplay

## Game Elements
```
@ - Player Character
X - Wall
$ - Gems / Gold
O - Rock
. - Earth
  - Space/Empty
E - Exit
```

## Rules
- Player can dig a tunnel through Earth
- Rocks/Gems fall down if there is space below them
- Rocks/Gems falling on Player ends the game -> Lose Scenario
- Player entering Exit ends the game -> Win scenario
- Player can't go through Wall, Rock
- Player can collect Gems

## Composite Rules
This is stable
```
.O.
.O.
```
This is unstable -> Top rock rolls to the left
```
. O..
. O..
```
This is unstable -> Top rock rolls to the right
```
..O .
..O .
```
However, this is stable
```
.O ...
XX XXX
```

Rocks and Gems behave similarly in most of the time.
For example, a Gem can kill the player if it falls on its head.

Player can push a rock horizontally if there is empty Space behind the Rock.
In the scenario below, the Player can push the rocks right or left.
```
.. O@O ..
.........
```
