# feedr

An offline desktop application for viewing website archives (.zim files). 

The ZIM catalogue includes Wikipedia, Stack Exchange, Project Gutenberg, medical manuals, etc

## What it is

feedr opens [ZIM](https://wiki.openzim.org/wiki/OpenZIM) files with libzim and renders articles.
Drop a `.zim` on the window and it becomes a tab.

Dark mode default, in settings.

## Features

- Home page with the library of added archives and previous files
- One tab per ZIM
- File explorer + `.zim`-filtered directory tree below
- Drag and drop + Add to library
- Shortcuts: random article, search, back, forward, archive home, jump to top
- Settings: Light mode toggle, search limit, font size
- CLI: `feedr recursive_self_improvement.zim`

## Build

Qt 6 (Core, Gui, Widgets) and libzim 7 or newer. pkg-config `libzim` is used when it is present; otherwise the link line falls back to `-lzim`.

```bash
cmake -S . -B build -G Ninja
cmake --build build --parallel
./build/bin/feedr
```

Intel oneAPI is optional. If you want `icpx`:

```bash
source /opt/intel/oneapi/setvars.sh
export CXX=icpx CC=icx
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER="$CC" -DCMAKE_CXX_COMPILER="$CXX"
cmake --build build --parallel
```

## Keybinds

| Key | Action |
|---|---|
| Ctrl+O | Open a ZIM in a new tab |
| Ctrl+Shift+O | Add to the library |
| Ctrl+H | Home |
| Ctrl+W | Close the current tab |
| Ctrl+Tab | Next tab |
| Ctrl+R | Random article |
| Ctrl+F / Ctrl+S | Search the current archive |
| Ctrl+T | Article top |
| Alt+Left | Back |
| Ctrl+, | Settings |
| Ctrl+Q | Quit |
| F1 | Shortcut list |

