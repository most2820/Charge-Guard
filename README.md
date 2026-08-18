# Charge Guard

Sysmodule and overlay for Nintendo Switch (Atmosphere) that limit the maximum battery charge level.

## Features

- Charge limit: 80%, 85%, 90%, 95%, 100%
- Configuration via overlay (Ultrahand / Tesla)
- Automatic background control (sysmodule)
- Auto-start on console boot

## Installation

1. Build the project:

```bash
bash build_all.sh
```

2. Copy the contents of `out/` to the root of your SD card:

```
SD:/
├── atmosphere/contents/010000000000BC01/
│   ├── exefs.nsp
│   └── flags/boot2.flag
├── config/charge-guard/
└── switch/.overlays/ChargeGuard.ovl
```

3. Reboot your console.

## Usage

- Open the overlay menu
- Open Charge Guard
- Select the desired limit with the scroll bar
- The sysmodule automatically applies the limit in the background

## Project Structure

```
├── build_all.sh              Build script
├── overlay/
│   ├── Makefile              Overlay build file
│   ├── source/main.cpp       Overlay UI
│   └── libs/libultrahand/    Tesla / Ultrahand framework
├── sysmodule/
│   ├── Makefile              Sysmodule build file
│   ├── source/main.c         Background process
│   └── sysmodule.json        NPDM configuration
```

## Requirements

- [devkitPro](https://devkitpro.org/) (devkitA64 + libnx)
- Atmosphere CFW on Switch
- Ultrahand Overlay или Tesla Menu

## Title ID

Sysmodule: `010000000000BC01`

If there is a conflict with another sysmodule, change it in `sysmodule/sysmodule.json`.