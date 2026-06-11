# Quake Engine Port

A multi-platform port of the **Quake** engine (WinQuake / GLQuake lineage) targeting Linux, macOS, Windows, and **STM32F769I-Discovery** with a software renderer and game data loaded from a microSD card (custom FAT32 reader).

All builds are driven by GNU Make from `GNU_MAKE/`. The default target is `quakeSTM32`.

This README applies to both **GitHub** and **GitLab** mirrors of the project.

## Build targets

Targets are selected via `TARGET` in `GNU_MAKE/Makefile` or on the command line (`make TARGET=quakeX11`).

### Main targets (Quake)

| TARGET | Runs on | Renderer / mode | Notes |
|--------|---------|-----------------|-------|
| `quakeSTM32` | STM32F769I-Discovery | Software | Default target; firmware for the dev board |
| `quakeX11` | Linux / macOS | Software (X11) | 32-bit build on Linux x86_64 by default |
| `quakeGLX11` | Linux | OpenGL (GLX) | OpenGL renderer over X11 |
| `quakeWin` | Windows (MinGW) | Software | Win32 port |

As in the Makefile:

```makefile
# TARGET ?= quakeX11
# TARGET ?= quakeGLX11
# TARGET ?= quakeWin
TARGET ?= quakeSTM32
```

### Workbench targets (bring-up / debug)

These are side targets for platform bring-up and debugging. They stay **commented out** in the Makefile and are not part of the main Quake deliverables:

```makefile
# TARGET ?= STM32_LCD
# TARGET ?= testSTM32
# TARGET ?= testSDL2
# TARGET ?= testX11GL
```

| TARGET | Runs on | Purpose |
|--------|---------|---------|
| `testX11GL` | Linux | X11 + OpenGL smoke test |
| `testSDL2` | Linux | SDL2 + GLES2 smoke test |
| `STM32_LCD` | STM32 | LCD / display bring-up |
| `testSTM32` | STM32 | Minimal MCU sanity check |

## Project layout

```
quake/
├── src/                    # Engine and platform layers
│   ├── engine/             # Host, client, server, shared
│   ├── game/               # Game logic (QuakeC VM, etc.)
│   └── platform/           # POSIX, Windows, MCU (STM32, FAT32, LCD)
├── MCU_src/                # STM32 HAL, BSP, FreeRTOS, startup, linker script
├── ID_src/                 # Git submodule — original id Software sources (reference only)
├── GNU_MAKE/               # Build system
│   ├── Makefile
│   ├── make_targets/       # trg_<TARGET>.mk — per-target source lists and flags
│   ├── features/           # Engine, renderer, HAL, MCU platform modules
│   ├── GCC_tools_PC.mk     # Host GCC toolchain / platform compatibility
│   ├── GCC_tools_STM32.mk  # ARM GCC toolchain / MCU platform compatibility
│   ├── prog_ST-Link.mk     # STM flash / OpenOCD helpers
│   ├── OUTPUT/<TARGET>/    # Build artifacts
│   ├── objs/<TARGET>/      # Object files
│   └── run_env/            # Host runtime directory (symlinks + id1/)
├── Documentations/         # Schematics, datasheets, architecture notes
├── NOTES.MD                # “Madman's notes” — working scraps and script fragments
└── ISSUES.md               # Known bugs and reproduction notes
```

`ID_src/` is a **git submodule** (see `.gitmodules`). It is **not compiled** — only kept as a side-by-side reference to the original id Software tree. Initialise after clone:

```bash
git submodule update --init ID_src
```

`GCC_tools_PC.mk` and `GCC_tools_STM32.mk` are the **GCC compatibility configurators**: they pick compiler paths, arch flags (`-m32`, `arm-none-eabi`, etc.), and baseline warning/optimisation flags. Most platform-specific tuning for host vs MCU builds lives here; `make_targets/` and `features/` layer target and subsystem options on top.

### Build artifacts

```
GNU_MAKE/
├── OUTPUT/
│   └── <TARGET>/
│       ├── <TARGET>.elf
│       ├── <TARGET>.hex    # STM32 — generated from .elf (objcopy)
│       ├── <TARGET>.bin    # STM32 — optional
│       └── <TARGET>.map    # STM32 — linker map
└── objs/
    └── <TARGET>/           # .o, .d, .su
```

`objs/`, `OUTPUT/`, and most of `run_env/` are listed in `.gitignore` and are produced by the build.

## Prerequisites

### Common

- `make`, `git`
- A C/C++ toolchain appropriate for the chosen `TARGET` (see below)

### Linux — host targets (`quakeX11`, `quakeGLX11`)

```bash
sudo apt install make gcc g++ build-essential libx11-dev libxext-dev
```

32-bit build on x86_64 (default for `quakeX11`):

```bash
sudo dpkg --add-architecture i386
sudo apt update
sudo apt install gcc-multilib g++-multilib libc6-dev-i386 \
    libx11-dev:i386 libxext-dev:i386
```

OpenGL target additionally:

```bash
sudo apt install mesa-utils libgl1-mesa-dev
```

**Xephyr** (strongly recommended for host runs):

```bash
sudo apt install xserver-xephyr
```

The legacy X11 video code expects a specific colour depth / pixel layout. On a normal desktop X session you often get garbage on screen; Xephyr lets you pin the format explicitly (e.g. `-screen 320x200x16`) so the renderer sees the layout it was written for.

### Linux — STM32 (`quakeSTM32`)

**ARM GNU Toolchain** (`arm-none-eabi-gcc`) — install an official ARM release and point the build at it via `GCC_tools_STM32.mk`.

The makefile selects toolchain version through `GCC_VER` (default `13.3.rel1`; older lines are commented in the file for quick switching). Override `GCC_BASE_PATH` if your install lives elsewhere. **Auto-download / auto-install is not implemented yet** — if the expected directory is missing, `make` stops with an error that includes a **direct download URL** for the matching `arm-gnu-toolchain-<GCC_VER>-<arch>-arm-none-eabi` package (usually correct for your host OS/arch).

Example manual install:

```bash
# use the URL printed by make if the path below does not match your GCC_VER / arch
mkdir -p ~/arm-gnu-toolchain-13.3.rel1-$(uname -m)-arm-none-eabi
cd ~/arm-gnu-toolchain-13.3.rel1-$(uname -m)-arm-none-eabi
curl -LO '<url-from-make-error>'
tar -xf arm-gnu-toolchain-*.tar.xz --strip-components=1
```

**Flash / debug tools:**

```bash
sudo apt install stlink-tools openocd gdb-multiarch
```

On ARM hosts (e.g. flashing/debugging from a Raspberry Pi) distro packages may be missing or outdated — `stlink-tools` can be built from [source](https://github.com/stlink-org/stlink) instead.

**Board:** [STM32F769I-Discovery](https://www.st.com/en/evaluation-tools/32f769idiscovery.html)

### macOS — host targets

- [XQuartz](https://www.xquartz.org/)
- X11 headers via Homebrew

### Windows — host targets

- MSYS2 / MinGW-w64 for `quakeWin`
- [Xming](https://sourceforge.net/projects/xming/) (or another X server) when running X11 builds

## Building

All commands run from `GNU_MAKE/`:

```bash
cd GNU_MAKE
```

### STM32 (default)

```bash
make
make TARGET=quakeSTM32 fresh    # clean + rebuild
```

| Variable | Default | Description |
|----------|---------|-------------|
| `DEBUG` | `1` | `1` → `-O0 -g`; `0` → `-O2 -DNDEBUG` |
| `OPT_LVL` | `0` | GCC optimisation level for STM32 |
| `NO_ASM` | `1` | Disable legacy ASM paths |

### Host (Linux / macOS)

```bash
make TARGET=quakeX11
make TARGET=quakeGLX11 fresh
```

### Windows

```bash
make TARGET=quakeWin
```

### General Make goals

| Goal | Description |
|------|-------------|
| `all` / `build` | Build the current `TARGET` |
| `clean` | Remove `objs/` and the output binary for the current `TARGET` |
| `fresh` | `clean` + `build` |
| `run` | Symlink + run (host only, see below) |
| `rund` | Run under `gdbserver :1234` (host) |
| `vars` | Print compiler flags and source list |

### STM programming / debug goals (`st*`)

Separate from the main build; they operate on the **last built** firmware for the current `TARGET`:

| Goal | Description |
|------|-------------|
| `stupload` | Flash the existing `.hex` via the on-board ST-Link (**does not rebuild**) |
| `streset` | Reset the MCU from the shell (ST-Link CLI) |
| `stdebug` | Start OpenOCD (ST-Link SWD, binds to `0.0.0.0`, GDB port **3333**) |

**Important:** `make stupload` only flashes the last `.hex` produced for the current `TARGET`. Build first:

```bash
make TARGET=quakeSTM32
make stupload
```

The `.hex` is generated from the existing `.elf` via `objcopy` when `stupload` runs; it will not pick up source changes unless you rebuild.

## Running on the host (Linux / macOS / Windows)

### Game data

For host runs, place `pak0.pak` in `GNU_MAKE/run_env/id1/`:

```
GNU_MAKE/run_env/id1/pak0.pak
```

A shareware `pak0.pak` is included in the repository for convenience (unmodified; full-version ordering information inside the archive is intact). See [Game data & licensing](#game-data--licensing).

### `make run`

`make run` creates a symlink in `run_env/` pointing at the binary for the **current** `TARGET`:

```
run_env/<TARGET>  →  ../OUTPUT/<TARGET>/<TARGET>.elf
```

Then it executes that binary from `run_env/` (where `id1/` lives). Example:

```bash
cd GNU_MAKE
make TARGET=quakeX11
make run TARGET=quakeX11
```

Both Linux and macOS targets use **`DISPLAY=:1`** by default (set in the renderer feature makefiles).

### X server before `make run`

Start a virtual X display **before** running the binary. Without it the old X11 renderer typically shows corrupted output — it relies on a fixed colour mode, not whatever your desktop happens to use.

**Linux / macOS** — background Xephyr (shell alias used in development):

```bash
alias X='Xephyr :1 -screen 320x200x16 -ac -retro'
X &          # start display :1 in the background
make run TARGET=quakeX11
```

`-screen 320x200x16` sets resolution and 16-bit colour; adjust if you need a different test geometry. A second display (`:2`) can be started the same way with `X2 &` if needed.

**Windows** — start [Xming](https://sourceforge.net/projects/xming/) (or another X server), then point `DISPLAY` at it (typically `localhost:0`) before running or debugging.

Useful flags when testing on a virtual display:

```bash
./quakeX11 -nosound -noshm -nodga
```

### Host debugging (VS Code)

Launch configurations in `.vscode/launch.json`:

- **Launch local quakeX11** / **Launch local quakeX11:1** — run under GDB/LLDB from `run_env/`
- **Attach: 32/64 gdbserver** — attach to `localhost:1234` after `make rund` (with `DISPLAY=:1`)

## Running on STM32F769I-Discovery

### Hardware used

- On-board **ST-Link/V2-1** (SWD) for flash and debug
- Same USB connection exposes a **virtual COM port** (UART stdio retarget)

Recommended VS Code extensions:

- [Cortex-Debug](https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug) — attach to OpenOCD on `localhost:3333`
- [Serial Monitor](https://marketplace.visualstudio.com/items?itemName=ms-vscode.vscode-serial-monitor) — UART log from the ST-Link VCP

### Flash workflow

```bash
cd GNU_MAKE
make TARGET=quakeSTM32    # build .elf (+ .map in OUTPUT/)
make stupload             # flash .hex, reset
```

### MCU debugging (VS Code)

1. Terminal: `make stdebug` (OpenOCD + ST-Link, GDB on port **3333**)
2. VS Code: run **Remote Debug STM32F7** from `launch.json` — attaches to `127.0.0.1:3333`, symbols from `OUTPUT/quakeSTM32/quakeSTM32.elf`

UART output is available on the ST-Link virtual serial port while debugging.

### microSD — game data layout

The MCU port uses a **home-grown FAT32 parser** — it is not a general-purpose library and **does not guarantee** compatibility with arbitrary cards or `mkfs` defaults. It works with the geometry below; extending it for other layouts is welcome (see `src/platform/MCU/FileSystem/`).

Expected path on the card (names are uppercased by the driver):

```
QUAKE/
└── ID1/
    └── PAK0.PAK
```

#### Reference card geometry

FAT32 layout the current parser is tuned for (`fsck.vfat` on a working card):

```
fsck.fat 4.2 (2021-01-31)
Checking we can access the last sector of the filesystem
Boot sector contents:
System ID "mkfs.fat"
Media byte 0xf8 (hard disk)
       512 bytes per logical sector
       512 bytes per cluster
      4110 reserved sectors
First FAT starts at byte 2104320 (sector 4110)
         2 FATs, 32 bit entries
    500224 bytes per FAT (= 977 sectors)
Root directory start at cluster 2 (arbitrary size)
Data area starts at byte 3104768 (sector 6064)
    125008 data clusters (64004096 bytes)
32 sectors/track, 64 heads
      2048 hidden sectors
    131072 sectors total
Checking for unused clusters.
Checking free cluster summary.
/dev/sdb1: 4 files, 36506/125008 clusters
```

#### Preparing a card with `dd`

A ready-made image `QUAKE_SD.img` is provided in the repository (shareware data, same unmodified `pak0.pak`). Write it to the microSD card — adjust `/dev/sdX` to your reader:

```bash
# inspect the card first
lsblk
sudo fdisk -l /dev/sdX

# write the image
sudo dd if=./QUAKE_SD.img of=/dev/sdX bs=4M status=progress conv=fsync
```

To capture your own image from a working card:

```bash
sudo dd if=/dev/sdX of=./QUAKE_SD.img bs=512 count=131073 status=progress conv=fsync
```

## Hardware summary (STM32)

| Resource | Detail |
|----------|--------|
| MCU | STM32F769NIHx (Cortex-M7 + FPU) |
| SDRAM | 16 MB (FMC) — heap and video buffer |
| Display | 800×480, LTDC + NT35510 |
| Storage | microSD (SDMMC2), FAT32 |
| Debug / UART | On-board ST-Link/V2-1 (SWD + USB VCP) |

Board schematics and datasheets are in `Documentations/`.

## Game data & licensing

The repository ships an **unmodified shareware** `pak0.pak` in two places to lower the barrier for contributors:

| Location | Purpose |
|----------|---------|
| `GNU_MAKE/run_env/id1/pak0.pak` | Host runs via `make run` |
| `QUAKE_SD.img` | microSD image for the STM32 target |

The archive has not been modified; id Software shareware registration / full-version ordering information inside `pak0.pak` is preserved. Replace with a legally obtained full version at your own discretion.

## License

| Component | License |
|-----------|---------|
| **Original Quake engine code** (id Software, 1996–1997) | [GNU GPL v2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html) |
| **Original code written for this port** | MIT License |
| STM32 HAL, BSP, CMSIS, FreeRTOS | ST / ARM / Amazon licenses — see `LICENSE` files under `MCU_src/` |
