# K6V3-512K Camp Mode

This branch targets UV-K6 V3 radios built around the PY32F071 MCU, including
512K units that do not have the 2 MB external SPI Flash layout used by some
Dondji assets. It intentionally avoids Chinese UI/font work and logo writes.

## Goals

- Airband AM receive presets with AM Fix enabled in the Camp build.
- Marine VHF FM receive presets.
- One-touch save for a scan hit into the first free memory channel.
- RX-only behavior for Camp presets: TX lock is forced on, offsets and tones
  are cleared, and low power is kept as the defensive default.
- No runtime hardware probing. Pick the firmware by radio label / known board
  version before flashing.

## Side Key Actions

The Camp build adds these assignable side-key actions:

- `AIR SCAN`: cycles airband segments.
- `SEA SCAN`: cycles marine VHF segments.
- `CAMP SAVE`: saves the current Camp hit or fixed-frequency watch point.

Repeated presses of `AIR SCAN` cycle:

- `AIR GND`: 121.600-121.950 MHz, AM
- `AIR LOW`: 118.000-123.000 MHz, AM
- `AIR MID`: 123.000-129.000 MHz, AM
- `AIR HI`: 129.000-136.975 MHz, AM
- `AIR ALL`: 118.000-136.975 MHz, AM
- `GUARD`: 121.500 MHz, AM fixed watch

Repeated presses of `SEA SCAN` cycle:

- `SEA 16`: 156.800 MHz, FM fixed watch
- `SEA SHP`: 156.000-157.425 MHz, FM
- `SEA CST`: 160.600-161.950 MHz, FM
- `SEA ALL`: 156.000-161.950 MHz, FM

AIS channels at 161.975 / 162.025 MHz are intentionally not included in the
first MVP because analog FM voice scanning is not useful for decoding AIS data.

## Build

Configure:

```sh
cmake --preset Camp
```

Build with Docker:

```sh
./compile-with-docker.sh Camp
```

or:

```sh
PATH="/Users/clay/Documents/UV-K5&6/toolchains/xpack-arm-none-eabi-gcc-15.2.1-1.1/bin:$PATH" cmake --build --preset Camp -j
```

Expected artifacts:

- `build/Camp/f4hwn.camp.bin`
- `build/Camp/f4hwn.camp.map`

## Current Validation

- `cmake --preset Camp` configures successfully.
- `CMakePresets.json` parses successfully.
- `git diff --check` passes.
- Full build succeeds with xPack GNU Arm Embedded GCC 15.2.1.
- Build memory report:
  - RAM: 10,848 B / 16 KB, 66.21%
  - FLASH: 74,220 B / 118 KB, 61.42%
- `build/Camp/f4hwn.camp.bin` SHA256:
  `a550d84af1b523379baf675eda401c6a836e5e9df3187e1e5fea6eab0e2070e4`

## Flashing Notes

Back up calibration before flashing any radio. Test on one non-primary 512K V3
radio first. This Camp branch does not depend on 2 MB external Flash assets and
does not implement Dondji logo/font writes.
