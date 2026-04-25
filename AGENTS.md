# shell_cat — Agent Notes

## Build

- **Preferred**: `make` produces `build/shell_cat_cli`
- **CMake** (alternative): `mkdir -p build && cd build && cmake .. && cmake --build .`
- Clean: `make clean` (removes `build/`)
- C++17, no external dependencies

## Tests

- The test suite is minimal. `tests/test_dummy.cpp` is a no-op `main()`.
- CMake builds it as `test_dummy`; run with `ctest` inside the build directory.
- `test_minimal.cpp` in the repo root is a standalone debug file **not wired into any build system**.

## Runtime Behavior

- The binary is fully interactive; it does not accept CLI arguments. Launching it starts a terminal UI that waits for keyboard input.
- It manipulates terminal state: hides the cursor, disables canonical mode and echo, and switches stdin to non-blocking.
- If the process crashes or is killed without graceful shutdown, the terminal may be left with **hidden cursor and no echo**. Run `reset` to recover.
- On first run it prompts for a cat name; after that it reads/writes `.shell_cat_profile` in the **current working directory**.

## Repo Gotchas

- Prebuilt binaries `shell_cat_cli_asan` and `test_minimal` exist in the repo root. Do not commit them; they are not tracked by `.gitignore` (only `build/` and patterns like `*.o` are ignored).
- `make` uses `-static -pthread` linking by default. CMake does **not** set `-static` unless you pass `-DCMAKE_EXE_LINKER_FLAGS="-static"` explicitly.

## Source Layout

- `include/shell_cat/` — headers (`renderer.hpp`, `cat.hpp`, `profile.hpp`)
- `src/` — implementation + `main.cpp`
- `tests/` — CMake-registered dummy test
