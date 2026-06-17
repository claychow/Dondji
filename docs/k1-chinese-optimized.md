# K1 Chinese Optimized

This branch adds a UV-K1 focused Chinese build on top of Dondji `motorola_r7`.

## Target

- UV-K1 / UV-K1(8) on the PY32F071 platform.
- Radios with enough external SPI Flash for Dondji resources.
- Chinese UI and Chinese channel names.

This is not the K6 512K Camp build. Do not use this profile for a K6 that only
has a 4 Mbit / 512 KB SPI Flash layout unless the high-address Dondji resources
are disabled and verified separately.

## What Changed

- Added `K1Chinese` CMake preset.
- Kept Chinese UI, FM broadcast radio, spectrum, screenshot, and F4HWN UI
  features.
- Disabled voice/audio scope/game in the K1 preset to reduce flash/RAM pressure
  and avoid depending on high-address voice resources.
- Upgraded the Chinese font layout to v3.
- Sorted the Unicode index table so firmware can use binary search instead of
  linearly scanning all 1372 entries for every Chinese character.
- Added font-ready validation. Old v2 font data will not be used by the v3
  firmware.

## Required Font Pairing

The firmware and `cn_font.bin` must be used as a matching pair:

- Firmware expects `CN_FONT_VERSION = 3`.
- `docs/font/cn_font.bin` length remains `42917` bytes.
- The v3 Unicode index starts at `U+4E00` and ends at `U+9F99`.

After flashing this firmware, flash the updated font from the updated web tool
or from `docs/font/cn_font.bin`. If the old v2 font is still on the radio, the
font status page should show `WAIT`, and Chinese channel names will not render
until the v3 font is written.

## Build

```sh
cmake --preset K1Chinese
cmake --build --preset K1Chinese -j
```

or:

```sh
./compile-with-docker.sh K1Chinese
```

Expected artifacts:

- `build/K1Chinese/Dondji.k1-cn.bin`
- `build/K1Chinese/Dondji.k1-cn.hex`
- `build/K1Chinese/Dondji.k1-cn.map`

## Future K1 Work

- Add an explicit K1 first-run default for navigation key layout and 2500 mAh
  battery profile.
- Add an 8 MB SPI Flash resource map if UV-K1(8) units are confirmed in hand.
- Consider moving optional boot logos or larger voice packs into capacity-aware
  layouts instead of the current fixed 2 MB-tail Dondji logo address.
