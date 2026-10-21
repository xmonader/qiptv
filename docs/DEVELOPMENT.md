# Development

## Prerequisites

- C++17 compiler (GCC/Clang)
- CMake ≥ 3.20
- Qt 6.5+ (Core, Gui, Widgets, Network, Test, OpenGLWidgets)
- libmpv (`mpv` pkg-config module)

On Debian/Ubuntu:

```bash
sudo apt install build-essential cmake qt6-base-dev libmpv-dev
```

## Workflow

```bash
make build   # cmake configure + build into build/
make test    # ctest
make run     # run build/bin/kiptv
make lint    # clang-tidy over the compile database
make clean
```

`make build` writes `build/compile_commands.json` for clang-tidy and editors.

## Conventions

- Non-UI logic goes in `kiptv_core` so it can be unit-tested; UI stays in the
  executable.
- Every model parser is defensive — never trust Xtream field types; route
  through `core/parse`.
- New behaviour gets a test. Logic tests link `kiptv_core`; widget tests run
  offscreen (`QT_QPA_PLATFORM=offscreen`).
- **moc caveat:** do not put `//` inside `R"(...)"` raw-string literals in a
  file moc processes — this moc version mis-lexes it as a comment. Use
  scheme-less fixture URLs or plain `"..."` strings.

## Adding a test

Add the `.cpp` under `tests/`, then register it in `tests/CMakeLists.txt`:

```cmake
add_executable(test_foo test_foo.cpp)
target_link_libraries(test_foo PRIVATE kiptv_core Qt6::Test)
add_test(NAME foo COMMAND test_foo)
```
