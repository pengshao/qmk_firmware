/* Copyright 2024 @ Keychron (https://www.keychron.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "c3_pro.h"

static uint32_t os_switch_timer_buffer   = 0;
static uint8_t  os_switch_indicate_count = 0;
static uint32_t os_switch_led_timer_buffer = 0;
static uint32_t os_switch_led_bit_disable_time = 2000;
static uint8_t os_switch_led_disable_bit = 0;

void keyboard_post_init_kb(void) {
    setPinOutputPushPull(LED_MAC_OS_PIN);
    setPinOutputPushPull(LED_WIN_OS_PIN);
    writePin(LED_MAC_OS_PIN, !LED_OS_PIN_ON_STATE);
    writePin(LED_WIN_OS_PIN, !LED_OS_PIN_ON_STATE);

    layer_state_t last_layer = eeconfig_read_default_layer();
    if (last_layer) {
        default_layer_set(last_layer);
    } else {
        default_layer_set(1U << 2);
    }

    keyboard_post_init_user();
}

void eeconfig_init_kb(void) {
#if (EECONFIG_KB_DATA_SIZE) == 0
    // Reset Keyboard EEPROM value to blank, rather than to a set value
    eeconfig_update_kb(0);
#endif
    keymap_config.raw  = eeconfig_read_keymap();
    keymap_config.nkro = 1;
    eeconfig_update_keymap(keymap_config.raw);

    eeconfig_init_user();
}

void housekeeping_task_kb(void) {
    if (os_switch_led_timer_buffer == 0) {
        os_switch_led_timer_buffer = timer_read32();
    }

    if (!os_switch_led_disable_bit && timer_elapsed32(os_switch_led_timer_buffer) > os_switch_led_bit_disable_time) {
        os_switch_led_disable_bit = 1;
    }

    if (!os_switch_led_disable_bit) {
		if (default_layer_state == (1U << 0)) {
    	    writePin(LED_MAC_OS_PIN, LED_OS_PIN_ON_STATE);
    	    writePin(LED_WIN_OS_PIN, !LED_OS_PIN_ON_STATE);
    	}
    	if (default_layer_state == (1U << 2)) {
    	    writePin(LED_MAC_OS_PIN, !LED_OS_PIN_ON_STATE);
    	    writePin(LED_WIN_OS_PIN, LED_OS_PIN_ON_STATE);
    	} 
    } else {
        writePin(LED_MAC_OS_PIN, !LED_OS_PIN_ON_STATE);
        writePin(LED_WIN_OS_PIN, !LED_OS_PIN_ON_STATE);
    }

    if (os_switch_timer_buffer && timer_elapsed32(os_switch_timer_buffer) > 300) {
        if (os_switch_indicate_count++ > 5) {
            os_switch_indicate_count = 0;
            os_switch_timer_buffer   = 0;
        } else {
            os_switch_timer_buffer = timer_read32();
        }
    }

    housekeeping_task_user();
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (!process_record_user(keycode, record)) {
        return false;
    }

    switch (keycode) {
#ifdef LED_MATRIX_ENABLE
        case BL_TOGG:
            if (record->event.pressed) {
                switch (led_matrix_get_flags()) {
                    case LED_FLAG_ALL: {
                        led_matrix_set_flags(LED_FLAG_NONE);
                        led_matrix_set_value_all(0);
                    } break;
                    default: {
                        led_matrix_set_flags(LED_FLAG_ALL);
                    } break;
                }
            }
            if (!led_matrix_get_val()) {
                led_matrix_set_flags(LED_FLAG_ALL);
                led_matrix_increase_val();
            }
            return false;
#endif
#ifdef RGB_MATRIX_ENABLE
        case RGB_TOG:
            if (record->event.pressed) {
                switch (rgb_matrix_get_flags()) {
                    case LED_FLAG_ALL: {
                        rgb_matrix_set_flags(LED_FLAG_NONE);
                        rgb_matrix_set_color_all(0, 0, 0);
                    } break;
                    default: {
                        rgb_matrix_set_flags(LED_FLAG_ALL);
                    } break;
                }
            }
            if(!rgb_matrix_get_val()){
                rgb_matrix_set_flags(LED_FLAG_ALL);
                rgb_matrix_increase_val();
            }
            return false;
#endif
        case KC_OSSW:
            if (record->event.pressed) {
                default_layer_xor(1U << 0);
                default_layer_xor(1U << 2);
                eeconfig_update_default_layer(default_layer_state);
                os_switch_timer_buffer = timer_read32();
                os_switch_led_timer_buffer = timer_read32();
                os_switch_led_disable_bit = 0;
            }
            return false;
        default:
            return true;
    }
}

#ifdef LED_MATRIX_ENABLE
bool led_matrix_indicators_kb(void) {
    if (!led_matrix_indicators_user()) {
        return false;
    }

    if (os_switch_indicate_count) {
        led_matrix_set_value_all(os_switch_indicate_count % 2 ? 0 : UINT8_MAX);
    }

    return true;
}
#endif

#ifdef RGB_MATRIX_ENABLE
bool rgb_matrix_indicators_kb(void){
    if(!rgb_matrix_indicators_user()){
        return false;
    }
    if(os_switch_indicate_count){
        rgb_matrix_set_color_all(os_switch_indicate_count % 2 ? 0 : RGB_RED);
    }
    return true;
}
#endif
void suspend_power_down_kb(void) {
    writePin(LED_WIN_OS_PIN, !LED_OS_PIN_ON_STATE);
    writePin(LED_MAC_OS_PIN, !LED_OS_PIN_ON_STATE);
    suspend_power_down_user();
}
