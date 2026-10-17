// Copyright 2026 Chus Jara Peinado
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define I2C_DRIVER I2CD0
#define I2C1_SDA_PIN GP12
#define I2C1_SCL_PIN GP13

#define OLED_DISPLAY_128X64

// Double-tap the board's RESET button to get to the UF2 bootloader
// (the BOOT button works too, but needs holding while pressing RESET).
#define RP2040_BOOTLOADER_DOUBLE_TAP_RESET
