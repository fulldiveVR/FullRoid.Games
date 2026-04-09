# Free Games Catalog

A collection of free, open-source games for the
[FullRoid](https://play.google.com/store/apps/details?id=com.fulldive.extension.fullroid) emulator. No proprietary libraries — every
dependency is open-source and can be verified below.

## Games

| Game | Platforms | Description |
|------|-----------|-------------|
| [Snake](snake/) | DS, 3DS | Classic snake with 25 language UI, volumetric pixel-art sprites, autopilot mode |
| [Number Slide](numberslide/) | DS, 3DS | 2048 clone — slide and merge tiles to reach 2048 |

## Platforms

| Platform | Notes |
|----------|-------|
| DS | `.nds` ROM for FullRoid DS emulation |
| 3DS | `.3dsx` homebrew for FullRoid 3DS emulation |

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

The first run builds the Docker image automatically. Subsequent runs are fast.
Output is placed in `repository/nds/` and `repository/3ds/`.

To build a single platform:

```bash
./build-nds.sh   # DS only
./build-3ds.sh   # 3DS only
```
