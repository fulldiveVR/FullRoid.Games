# Free Games Catalog

A collection of free, open-source games for handheld consoles. No proprietary
libraries — every dependency is open-source and can be verified below.

## Games

| Game | Platforms | Description |
|------|-----------|-------------|
| [Snake](snake/) | DS, 3DS | Classic snake with 25 language UI, volumetric pixel-art sprites, autopilot mode |

## Platforms

| Platform | Notes |
|----------|-------|
| DS | `.nds` ROM, runs on hardware via flashcart or TWiLight Menu++ |
| 3DS | `.3dsx` homebrew, runs via Homebrew Launcher or Luma3DS |

Prebuilt ROMs are in [`repository/`](repository/).

## Dependencies

All dependencies are open-source. No proprietary libraries are used.

| Library | License | Source |
|---------|---------|--------|
| [devkitARM](https://devkitpro.org) | GPL-2.0 | devkitPro toolchain (ARM cross-compiler) |
| [libnds / calico](https://github.com/devkitPro/calico) | Zlib | NDS hardware abstraction |
| [libctru](https://github.com/devkitPro/libctru) | Zlib | 3DS hardware abstraction |
| [citro3d](https://github.com/devkitPro/citro3d) | Zlib | 3DS GPU rendering |
| [citro2d](https://github.com/devkitPro/citro2d) | Zlib | 3DS 2D graphics helpers |

## Building

Requires [Docker](https://docs.docker.com/get-docker/).

```bash
cd snake
./build.sh
```

The first run builds the `snake-devkit` Docker image automatically. Subsequent
runs are fast. Output is placed in `repository/nds/` and `repository/3ds/`.

To build a single platform:

```bash
./build-nds.sh   # DS only
./build-3ds.sh   # 3DS only
```
