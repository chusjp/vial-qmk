// Copyright 2026 Chus Jara Peinado
// SPDX-License-Identifier: GPL-2.0-or-later

#include "quantum.h"

#ifdef OLED_ENABLE
// The module is mounted upside down (header toward the keys), so flip the image.
oled_rotation_t oled_init_kb(oled_rotation_t rotation) {
    return oled_init_user(OLED_ROTATION_180);
}

bool oled_task_kb(void) {
    if (!oled_task_user()) {
        return false;
    }
    oled_write_ln_P(PSTR("enigma"), false);
    oled_write_ln_P(PSTR(""), false);
    oled_write_P(PSTR("Layer: "), false);
    oled_write_ln(get_u8_str(get_highest_layer(layer_state | default_layer_state), ' '), false);
    return false;
}
#endif
