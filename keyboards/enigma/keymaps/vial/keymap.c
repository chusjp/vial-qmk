// Copyright 2026 Chus Jara Peinado
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Vial keymap for enigma. Keys, macros, combos and layers are all editable
// live from the Vial app (https://vial.rocks). On top of that:
//
//  - PEEK_ENT (the ISO Enter by default): tap = Enter. Hold it and press any
//    key to see on the OLED what that key does -- a macro's content, "Ctrl+C",
//    "Hold L2"... -- instead of doing it.
//  - LAYER_NEXT cycles the 4 layers. A fresh board gets one A+B combo per
//    layer for it, plus a few starter macros (see install_defaults()).
//  - The idle screen lists what A-F do on the current layer.

#include QMK_KEYBOARD_H
#include <string.h>
#include <stdio.h>
#include "dynamic_keymap.h"
#include "vial.h"
#include "keycode_string.h"

enum custom_keycodes {
    PEEK_ENT = QK_KB_0, // order must match "customKeycodes" in vial.json
    LAYER_NEXT,
};

#define LAYERS DYNAMIC_KEYMAP_LAYER_COUNT
#define M(n) (QK_MACRO_0 + (n))

// Layer n's six keys run macros M(6n)..M(6n+5).
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /*
     * ┌───────────┬───┬───┬───────┐
     * │   OLED    │ A │ B │       │
     * ├───┬───┬───┴┬──┴┬──┴┐ Ent │  <- PEEK_ENT: tap = Enter,
     * │ C │ D │ E  │ F │   │     │     hold + key = show what the key does
     * └───┴───┴────┴───┘   └─────┘
     */
    [0] = LAYOUT(
                          M(0),    M(1),    PEEK_ENT,
        M(2),    M(3),    M(4),    M(5)
    ),
    [1] = LAYOUT(
                          M(6),    M(7),    PEEK_ENT,
        M(8),    M(9),    M(10),   M(11)
    ),
    [2] = LAYOUT(
                          M(12),   M(13),   PEEK_ENT,
        M(14),   M(15),   M(16),   M(17)
    ),
    [3] = LAYOUT(
                          M(18),   M(19),   PEEK_ENT,
        M(20),   M(21),   M(22),   M(23)
    ),
};

// ------------------------------------------------------------------ first-boot defaults
// Installed once on a fresh board (or after an EEPROM reset); afterwards Vial owns
// them and they're edited in the app.
static const char *const default_macros[] = {
    SS_DOWN(X_LCTL) SS_TAP(X_C) SS_UP(X_LCTL),                                 // A: copy
    SS_DOWN(X_LCTL) SS_TAP(X_V) SS_UP(X_LCTL),                                 // B: paste
    SS_DOWN(X_LCTL) SS_TAP(X_Z) SS_UP(X_LCTL),                                 // C: undo
    "git commit -m \"" SS_TAP(X_ENTER),                                        // D
    SS_DOWN(X_LGUI) SS_DOWN(X_LSFT) SS_TAP(X_4) SS_UP(X_LSFT) SS_UP(X_LGUI),   // E: macOS screenshot
    SS_DOWN(X_LCTL) SS_TAP(X_Y) SS_UP(X_LCTL),                                 // F: redo
    "git push" SS_TAP(X_ENTER),                                                // layer 2 A
    "git status" SS_TAP(X_ENTER),                                              // layer 2 B
};
#define DEFAULTS_INSTALLED 0xE41A0001

static void install_defaults(void) {
    uint16_t offset = 0;
    for (uint8_t i = 0; i < sizeof(default_macros) / sizeof(default_macros[0]); i++) {
        uint16_t size = strlen(default_macros[i]) + 1; // with its terminating 0
        dynamic_keymap_macro_set_buffer(offset, size, (uint8_t *)default_macros[i]);
        offset += size;
    }
    // A+B -> next layer, on every layer (combos match keycodes, and A/B differ per layer)
    for (uint8_t l = 0; l < LAYERS; l++) {
        vial_combo_entry_t combo = {.input = {M(6 * l), M(6 * l + 1), KC_NO, KC_NO}, .output = LAYER_NEXT};
        dynamic_keymap_set_combo(l, &combo);
    }
    vial_init(); // reload Vial's combo table, which it loaded before we got here
    eeconfig_update_user(DEFAULTS_INSTALLED);
}

void keyboard_post_init_user(void) {
    if (eeconfig_read_user() != DEFAULTS_INSTALLED) {
        install_defaults();
    }
}

// ------------------------------------------------------------------ peek + layer cycling
static bool     peek_held = false; // PEEK_ENT is down
static bool     peek_used = false; // a key was peeked during this hold (so no Enter on release)
static uint16_t peek_timer;
static uint16_t peek_kc = KC_NO;   // what the last peeked key does
static uint8_t  swallowed = 0;     // keys pressed while peeking: their release is swallowed too

// No combos while peeking: holding Enter and pressing A, B should show A and B, not switch layers.
bool combo_should_trigger(uint16_t combo_index, combo_t *combo, uint16_t keycode, keyrecord_t *record) {
    return !peek_held;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (keycode == PEEK_ENT) {
        if (record->event.pressed) {
            peek_held  = true;
            peek_used  = false;
            peek_kc    = KC_NO;
            peek_timer = timer_read();
        } else {
            peek_held = false;
            if (!peek_used && timer_elapsed(peek_timer) < TAPPING_TERM) {
                tap_code(KC_ENTER);
            }
        }
        return false;
    }
    // real matrix keys only (combos report a position outside the matrix)
    if (record->event.key.row < MATRIX_ROWS && record->event.key.col < MATRIX_COLS) {
        uint8_t bit = 1 << (record->event.key.row * MATRIX_COLS + record->event.key.col);
        if (record->event.pressed && peek_held) {
            peek_kc   = keycode;
            peek_used = true;
            swallowed |= bit;
            return false;
        }
        if (!record->event.pressed && (swallowed & bit)) {
            swallowed &= ~bit;
            return false;
        }
    }
    if (keycode == LAYER_NEXT) {
        if (record->event.pressed) {
            layer_move((get_highest_layer(layer_state) + 1) % LAYERS);
        }
        return false;
    }
    return true;
}

// ------------------------------------------------------------------ "what does this key do" in words
static void append(char *out, size_t n, const char *s) {
    size_t len = strlen(out);
    while (*s && len + 1 < n) {
        out[len++] = *s++;
    }
    out[len] = 0;
}

// Ctrl/Shift/Alt/GUI prefix from a 4-bit mod mask (bit 0 Ctrl, 1 Shift, 2 Alt, 3 GUI)
static void append_mods(char *out, size_t n, uint8_t mods) {
    static const char *const names[] = {"Ctrl+", "Shift+", "Alt+", "GUI+"};
    for (uint8_t i = 0; i < 4; i++) {
        if (mods & (1 << i)) {
            append(out, n, names[i]);
        }
    }
}

static const char *basic_name(uint8_t kc, char tmp[12]) {
    if (kc >= KC_A && kc <= KC_Z) {
        tmp[0] = 'A' + (kc - KC_A);
        tmp[1] = 0;
        return tmp;
    }
    if (kc >= KC_1 && kc <= KC_0) {
        tmp[0] = kc == KC_0 ? '0' : '1' + (kc - KC_1);
        tmp[1] = 0;
        return tmp;
    }
    if (kc >= KC_F1 && kc <= KC_F12) {
        snprintf(tmp, 12, "F%d", 1 + kc - KC_F1);
        return tmp;
    }
    if (kc >= KC_F13 && kc <= KC_F24) {
        snprintf(tmp, 12, "F%d", 13 + kc - KC_F13);
        return tmp;
    }
    switch (kc) {
        case KC_ENTER: return "Enter";
        case KC_ESCAPE: return "Esc";
        case KC_BACKSPACE: return "Bksp";
        case KC_TAB: return "Tab";
        case KC_SPACE: return "Space";
        case KC_MINUS: return "-";
        case KC_EQUAL: return "=";
        case KC_LEFT_BRACKET: return "[";
        case KC_RIGHT_BRACKET: return "]";
        case KC_BACKSLASH: return "\\";
        case KC_SEMICOLON: return ";";
        case KC_QUOTE: return "'";
        case KC_GRAVE: return "`";
        case KC_COMMA: return ",";
        case KC_DOT: return ".";
        case KC_SLASH: return "/";
        case KC_DELETE: return "Del";
        case KC_INSERT: return "Ins";
        case KC_HOME: return "Home";
        case KC_END: return "End";
        case KC_PAGE_UP: return "PgUp";
        case KC_PAGE_DOWN: return "PgDn";
        case KC_LEFT: return "Left";
        case KC_RIGHT: return "Right";
        case KC_UP: return "Up";
        case KC_DOWN: return "Down";
        case KC_PRINT_SCREEN: return "PrtSc";
        case KC_CAPS_LOCK: return "Caps";
        case KC_AUDIO_MUTE: return "Mute";
        case KC_AUDIO_VOL_UP: return "Vol+";
        case KC_AUDIO_VOL_DOWN: return "Vol-";
        case KC_MEDIA_PLAY_PAUSE: return "Play";
        case KC_MEDIA_NEXT_TRACK: return "Next";
        case KC_MEDIA_PREV_TRACK: return "Prev";
        case KC_LEFT_CTRL: case KC_RIGHT_CTRL: return "Ctrl";
        case KC_LEFT_SHIFT: case KC_RIGHT_SHIFT: return "Shift";
        case KC_LEFT_ALT: case KC_RIGHT_ALT: return "Alt";
        case KC_LEFT_GUI: case KC_RIGHT_GUI: return "GUI";
    }
    return NULL;
}

static void macro_name(uint8_t id, char *out, size_t n);

// Appends a short human name for keycode kc.
static void keycode_name(uint16_t kc, char *out, size_t n) {
    char        tmp[12];
    const char *s = NULL;
    if (kc == KC_NO) {
        s = "(nothing)";
    } else if (kc == KC_TRANSPARENT) {
        s = "(layer below)";
    } else if (kc == PEEK_ENT) {
        s = "Peek/Enter";
    } else if (kc == LAYER_NEXT) {
        s = "Next layer";
    } else if (IS_QK_MACRO(kc)) {
        macro_name(kc - QK_MACRO, out, n);
        return;
    } else if (IS_QK_BASIC(kc)) {
        s = basic_name(kc, tmp);
    } else if (IS_QK_MODS(kc)) {
        const char *base = basic_name(QK_MODS_GET_BASIC_KEYCODE(kc), tmp);
        if (base) {
            uint8_t m = QK_MODS_GET_MODS(kc);
            append_mods(out, n, m & 0x0F); // bit 4 = right-hand mods: same names
            append(out, n, base);
            return;
        }
    } else if (IS_QK_MOMENTARY(kc)) {
        snprintf(tmp, sizeof(tmp), "Hold L%d", QK_MOMENTARY_GET_LAYER(kc) + 1);
        s = tmp;
    } else if (IS_QK_TOGGLE_LAYER(kc)) {
        snprintf(tmp, sizeof(tmp), "Tog L%d", QK_TOGGLE_LAYER_GET_LAYER(kc) + 1);
        s = tmp;
    } else if (IS_QK_TO(kc)) {
        snprintf(tmp, sizeof(tmp), "Go L%d", QK_TO_GET_LAYER(kc) + 1);
        s = tmp;
    }
    if (!s) { // anything else: QMK's own name, without the KC_ prefix
        s = get_keycode_string(kc);
        if (strncmp(s, "KC_", 3) == 0) {
            s += 3;
        }
    }
    append(out, n, s);
}

static uint8_t macro_byte(uint16_t offset) {
    uint8_t b = 0;
    dynamic_keymap_macro_get_buffer(offset, 1, &b);
    return b;
}

// Vial stores 16-bit keycodes so that neither byte is 0 (see dynamic_keymap.c)
static uint16_t decode_ext_keycode(uint16_t kc) {
    return kc > 0xFF00 ? (kc & 0xFF) << 8 : kc;
}

// A macro's own content as its name: typed text as-is, keys as "Ctrl+C"; a final Enter is
// left out ("git push" rather than "git push Enter").
static void macro_name(uint8_t id, char *out, size_t n) {
    uint16_t size = dynamic_keymap_macro_get_buffer_size();
    uint16_t off  = 0;
    if (macro_byte(size - 1) != 0) {
        append(out, n, "(saving...)");
        return;
    }
    for (uint8_t skip = id; skip > 0 && off < size; off++) {
        if (macro_byte(off) == 0) {
            skip--;
        }
    }
    uint8_t mods          = 0;
    bool    pending_enter = false;
    size_t  start         = strlen(out);
    for (uint8_t c; off < size && (c = macro_byte(off++)) != 0;) {
        uint16_t kc     = KC_NO;
        bool     is_key = false, down = false, up = false;
        if (c != SS_QMK_PREFIX) {
            if (c >= 0x20 && c < 0x7F) {
                if (pending_enter) {
                    append(out, n, " Enter ");
                    pending_enter = false;
                }
                char s[2] = {(char)c, 0};
                append(out, n, s);
            }
            continue;
        }
        uint8_t code = macro_byte(off++);
        if (code == SS_TAP_CODE || code == SS_DOWN_CODE || code == SS_UP_CODE) {
            kc     = macro_byte(off++);
            is_key = true;
            down   = code == SS_DOWN_CODE;
            up     = code == SS_UP_CODE;
        } else if (code == VIAL_MACRO_EXT_TAP || code == VIAL_MACRO_EXT_DOWN || code == VIAL_MACRO_EXT_UP) {
            kc = macro_byte(off) | (macro_byte(off + 1) << 8);
            kc = decode_ext_keycode(kc);
            off += 2;
            is_key = true;
            down   = code == VIAL_MACRO_EXT_DOWN;
            up     = code == VIAL_MACRO_EXT_UP;
        } else if (code == SS_DELAY_CODE) {
            off += 2;
        } else {
            break; // malformed
        }
        if (!is_key) {
            continue;
        }
        if (IS_MODIFIER_KEYCODE(kc)) { // held modifiers become a "Ctrl+" prefix
            uint8_t bit = 1 << ((kc - KC_LEFT_CTRL) & 3);
            if (down) mods |= bit;
            if (up) mods &= ~bit;
            if (down || up) continue;
        }
        if (up) {
            continue;
        }
        if (kc == KC_ENTER && mods == 0) {
            pending_enter = true;
            continue;
        }
        if (pending_enter) {
            append(out, n, " Enter");
            pending_enter = false;
        }
        if (strlen(out) > start && out[strlen(out) - 1] != ' ') {
            append(out, n, " ");
        }
        append_mods(out, n, mods);
        keycode_name(kc, out, n);
    }
    if (strlen(out) == start) {
        append(out, n, pending_enter ? "Enter" : "(empty)");
    }
}

// ------------------------------------------------------------------ OLED
#ifdef OLED_ENABLE
// big_font[] (printable ASCII, 5 columns/glyph), extracted from QMK's glcdfont.c
#    include "keymap_bigfont.h"

// Draws one line of `str` with each font pixel enlarged to a scale x scale block, centred.
static void oled_write_big(const char *str, uint8_t scale) {
    uint8_t len    = strlen(str);
    uint8_t cell_w = 6 * scale; // 5 glyph columns + 1 space column
    int16_t x0     = ((int16_t)OLED_DISPLAY_WIDTH - (int16_t)(len * cell_w)) / 2;
    if (x0 < 0) {
        x0 = 0;
    }
    uint8_t y0 = (OLED_DISPLAY_HEIGHT - 8 * scale) / 2;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t c = (uint8_t)str[i];
        if (c < BIG_FONT_FIRST || c > BIG_FONT_LAST) {
            c = ' ';
        }
        uint16_t glyph_off = (uint16_t)(c - BIG_FONT_FIRST) * 5;
        for (uint8_t col = 0; col < 5; col++) {
            uint8_t bits = pgm_read_byte(&big_font[glyph_off + col]);
            for (uint8_t row = 0; row < 8; row++) {
                if (!(bits & (1 << row))) {
                    continue;
                }
                for (uint8_t sx = 0; sx < scale; sx++) {
                    for (uint8_t sy = 0; sy < scale; sy++) {
                        oled_write_pixel((uint8_t)(x0 + i * cell_w + col * scale + sx), y0 + row * scale + sy, true);
                    }
                }
            }
        }
    }
}

// One idle-screen row of two keys, "A:Ctrl+C   B:Ctrl+V": label + first 8 characters of
// what the key does (21 characters = the full width of the display).
static void draw_key_row(uint8_t y, char l1, uint8_t r1, uint8_t c1, char l2, uint8_t r2, uint8_t c2, uint8_t layer) {
    char a[24] = "", b[24] = "", line[24];
    keycode_name(dynamic_keymap_get_keycode(layer, r1, c1), a, sizeof(a));
    keycode_name(dynamic_keymap_get_keycode(layer, r2, c2), b, sizeof(b));
    snprintf(line, sizeof(line), "%c:%-8.8s %c:%-8.8s", l1, a, l2, b);
    oled_set_cursor(0, y);
    oled_write(line, false);
}

bool oled_task_user(void) {
    static uint16_t last_draw = 0;
    static uint32_t last_sig  = 0xFFFFFFFF;
    uint8_t         layer     = get_highest_layer(layer_state);
    // redraw on any change of what's shown, and every 500 ms (macros may have been edited in Vial)
    uint32_t sig = ((uint32_t)peek_held << 31) | ((uint32_t)peek_used << 30) | ((uint32_t)layer << 16) | peek_kc;
    if (sig == last_sig && timer_elapsed(last_draw) < 500) {
        return false;
    }
    last_sig  = sig;
    last_draw = timer_read();
    oled_clear();

    char header[8];
    snprintf(header, sizeof(header), "L%d/%d", layer + 1, LAYERS);
    if (peek_held) {
        oled_set_cursor(0, 0);
        oled_write(header, false);
        if (!peek_used) {
            oled_set_cursor(0, 3);
            oled_write_ln_P(PSTR(" Press a key to see"), false);
            oled_write_ln_P(PSTR("   what it does"), false);
            return false;
        }
        char name[64] = "";
        keycode_name(peek_kc, name, sizeof(name));
        uint8_t len = strlen(name);
        if (len <= 7) {
            oled_write_big(name, 3);
        } else if (len <= 10) {
            oled_write_big(name, 2);
        } else { // long macros: wrap over the lines below the header
            oled_set_cursor(0, 2);
            oled_write(name, false);
        }
        return false;
    }
    oled_set_cursor(0, 0);
    oled_write(header, false);
    oled_set_cursor(15, 0);
    oled_write_P(PSTR("enigma"), false);
    // matrix positions: A/B = row 0 cols 0/1, C-F = row 1 cols 0-3
    draw_key_row(2, 'A', 0, 0, 'B', 0, 1, layer);
    draw_key_row(3, 'C', 1, 0, 'D', 1, 1, layer);
    draw_key_row(4, 'E', 1, 2, 'F', 1, 3, layer);
    oled_set_cursor(0, 6);
    oled_write_P(PSTR("Hold Enter + a key"), false);
    oled_set_cursor(0, 7);
    oled_write_P(PSTR("to see it in full"), false);
    return false;
}
#endif
