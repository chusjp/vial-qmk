// Copyright 2026 Chus Jara Peinado
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define VIAL_KEYBOARD_UID {0xC2, 0x32, 0x6C, 0x60, 0x95, 0xBF, 0xF5, 0x1E}

// Unlock (needed before Vial lets you edit macros etc.): hold Enter, then A.
// In that order the Enter key is in peek mode, so A only shows its name.
#define VIAL_UNLOCK_COMBO_ROWS { 0, 0 }
#define VIAL_UNLOCK_COMBO_COLS { 2, 0 }

// 4 layers x 6 macro keys
#define DYNAMIC_KEYMAP_LAYER_COUNT 4
#define DYNAMIC_KEYMAP_MACRO_COUNT 24
