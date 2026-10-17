# enigma

A minimal macropad laid out like a slice of a keyboard: a 128x64 I2C
OLED set into the case bezel where the top row's first key would be,
six 1u MX keys, and an ISO Enter.

* Keyboard Maintainer: [Chus Jara Peinado](https://github.com/chusjp)
* Hardware Supported: enigma v3 PCB — bare RP2040 with USB-C, MX
  hotswap sockets, SSD1306 128x64 I2C OLED
* Hardware Availability: custom build; the KiCad PCB and FreeCAD case are
  kept outside this repo

## Layout

```
┌───────────┬───┬───┬───────┐
│   OLED    │ A │ B │       │
├───┬───┬───┴┬──┴┬──┴┐ Enter │
│ C │ D │ E  │ F │   │       │
└───┴───┴────┴───┘   └───────┘
```

The `default` keymap types A-F and Enter, and the OLED shows the
current layer. It is meant as a clean starting point for your own keymap.

## Hardware

Each switch is wired straight to its own RP2040 pin (direct pins, no
matrix or diodes): A=GP9, B=GP7, Enter=GP5, C=GP11, D=GP10, E=GP8,
F=GP6. The OLED is on I2C0 (SDA=GP12, SCL=GP13), cabled to a JST-SH (Qwiic)
connector on the PCB and mounted upside down (header toward the keys), so
the firmware rotates the image 180°.

Make example for this keyboard (after setting up your build environment):

    make enigma:default

Flashing example for this keyboard:

    make enigma:default:flash

See the [build environment setup](https://docs.qmk.fm/#/getting_started_build_tools) and the [make instructions](https://docs.qmk.fm/#/getting_started_make_guide) for more information. Brand new to QMK? Start with our [Complete Newbs Guide](https://docs.qmk.fm/#/newbs).

## Bootloader

* **Double-tap RESET**: the board's RESET button, pressed twice quickly,
  drops into the UF2 bootloader (`RP2040_BOOTLOADER_DOUBLE_TAP_RESET`)
* **BOOT button**: hold BOOT while pressing RESET (or while plugging in)
* **Bootmagic reset**: hold down key A (matrix position 0,0) while
  plugging in the keyboard
* A factory-fresh board with empty flash shows up as the `RPI-RP2`
  drive by itself — drag the `.uf2` onto it

The BOOT and RESET buttons are on the underside of the PCB.
