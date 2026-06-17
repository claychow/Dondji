# K1 Chinese Optimized

This branch adds a UV-K1 focused Chinese build on top of Dondji `motorola_r7`.

## Target

- UV-K1 / UV-K1(8) on the PY32F071 platform.
- Radios with enough external SPI Flash for Dondji resources.
- Chinese UI and Chinese channel names.
- Common UV-K1 2 MB external SPI Flash layout.

This is not the K6 512K Camp build. Do not use this profile for a K6 that only
has a 4 Mbit / 512 KB SPI Flash layout unless the high-address Dondji resources
are disabled and verified separately.

## What Changed

- Added `K1Chinese` CMake preset.
- Kept Chinese UI, FM broadcast radio, spectrum, screenshot, and F4HWN UI
  features.
- Disabled voice/audio scope/game in the K1 preset to reduce flash/RAM pressure
  and avoid depending on high-address voice resources.
- Added assignable side-key actions for airband AM scan, marine VHF FM scan,
  and saving the current Camp scan hit.
- Enabled AM Fix in the K1 preset for airband AM receive.
- Upgraded the Chinese font layout to v3.
- Sorted the Unicode index table so firmware can use binary search instead of
  linearly scanning all 1372 entries for every Chinese character.
- Added font-ready validation. Old v2 font data will not be used by the v3
  firmware.

## Air And Marine Scan

The K1 Chinese profile includes the Camp scan actions from the K6 field build:

- `AIR SCAN`: cycles aviation AM scan segments.
- `SEA SCAN`: cycles marine VHF FM scan segments.
- `CAMP SAVE`: saves the current Camp hit or fixed watch point to the first
  free memory channel.

Airband scan enables AM Fix in the K1 preset. Marine scan uses 25 kHz wide FM
receive bandwidth to better match common VHF marine voice channels.

Repeated presses of `AIR SCAN` cycle:

- `AIR GND`: 121.600-121.950 MHz, AM.
- `AIR LOW`: 118.000-123.000 MHz, AM.
- `AIR MID`: 123.000-129.000 MHz, AM.
- `AIR HI`: 129.000-136.975 MHz, AM.
- `AIR ALL`: 118.000-136.975 MHz, AM.
- `GUARD`: 121.500 MHz, AM fixed watch.

Repeated presses of `SEA SCAN` cycle:

- `SEA 16`: 156.800 MHz, FM fixed watch.
- `SEA SHP`: 156.000-157.425 MHz, FM.
- `SEA CST`: 160.600-161.950 MHz, FM.
- `SEA ALL`: 156.000-161.950 MHz, FM.

AIS channels at 161.975 / 162.025 MHz are intentionally not included because
analog FM voice scanning is not useful for decoding AIS data.

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
- Consider moving optional boot logos or larger voice packs into capacity-aware
  layouts instead of the current fixed 2 MB-tail Dondji logo address.
