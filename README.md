# shell_cat

A terminal ASCII pet cat with interactive states, persistent profile, and a small fishing minigame.

## Overview

`shell_cat` runs a small ASCII cat in your terminal. The cat lives its own life — standing, sitting, crouching, sleeping, blinking, and wagging its tail. You can interact with it, manage its needs, and jump into a quick fishing minigame to earn coins and level up.

## Features

- **Persistent profile**: name, creation date, day count, feed count, pet count, hunger, mood, energy, cleanliness, coins, level, best combo, fish caught
- **Living stats**: hunger rises, energy falls, cleanliness drops, and mood follows the cat's needs
- **Random states**: Standing, Sitting, Crouching, Sleeping, Playful, Grumpy, Happy, Eating — with idle animations
- **Fishing minigame**: catch fish with lane timing to earn coins, combo bonuses, and progression
- **Interactive controls**:
  - `P` — Pet the cat (enters Happy state, +1 pet count)
  - `F` — Feed the cat (enters Eating state, +1 feed count)
  - `B` — Brush the cat and raise cleanliness
  - `S` — Let the cat sleep and recover energy
  - `G` — Start the fishing minigame
  - `N` — Rename the cat
  - `R` — Trigger a random action
  - `Q` — Quit
  - `Ctrl+C` — Also quits and restores the cursor
- **2D ASCII layout**:
  - Top-left: the cat
  - Right side: info panel with stats and progression
  - Bottom-left: control hints

## Build

### Using Make (recommended for release)

```bash
make
```

This compiles an optimized release binary to `build/shell_cat_cli`.

To clean and rebuild:

```bash
make clean && make
```

### Using CMake

```bash
mkdir -p build
cd build
cmake -DCMAKE_EXE_LINKER_FLAGS="-static" ..
cmake --build .
```

> Use `-DCMAKE_EXE_LINKER_FLAGS="-static"` only if your environment has dynamic-linking issues.

## Run

```bash
./build/shell_cat_cli
```

On first launch you will be asked to name your cat. After that, the profile is saved to `.shell_cat_profile` in the current working directory.

## Project structure

- `include/shell_cat/` — headers
  - `renderer.hpp` — terminal rendering (2D text, sprites, boxes)
  - `cat.hpp` — cat state machine and ASCII sprites
  - `profile.hpp` — persistent profile data
- `src/` — sources
  - `main.cpp` — main loop, input handling, layout
  - `renderer.cpp`, `cat.cpp`, `profile.cpp`
- `tests/` — build verification

## License

MIT
