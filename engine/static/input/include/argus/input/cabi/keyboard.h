/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ArgusKeyboardScancode {
    KB_SCANCODE_UNKNOWN = 0,
    KB_SCANCODE_A = 4,
    KB_SCANCODE_B = 5,
    KB_SCANCODE_C = 6,
    KB_SCANCODE_D = 7,
    KB_SCANCODE_E = 8,
    KB_SCANCODE_F = 9,
    KB_SCANCODE_G = 10,
    KB_SCANCODE_H = 11,
    KB_SCANCODE_I = 12,
    KB_SCANCODE_J = 13,
    KB_SCANCODE_K = 14,
    KB_SCANCODE_L = 15,
    KB_SCANCODE_M = 16,
    KB_SCANCODE_N = 17,
    KB_SCANCODE_O = 18,
    KB_SCANCODE_P = 19,
    KB_SCANCODE_Q = 20,
    KB_SCANCODE_R = 21,
    KB_SCANCODE_S = 22,
    KB_SCANCODE_T = 23,
    KB_SCANCODE_U = 24,
    KB_SCANCODE_V = 25,
    KB_SCANCODE_W = 26,
    KB_SCANCODE_X = 27,
    KB_SCANCODE_Y = 28,
    KB_SCANCODE_Z = 29,
    KB_SCANCODE_NUMBER_1 = 30,
    KB_SCANCODE_NUMBER_2 = 31,
    KB_SCANCODE_NUMBER_3 = 32,
    KB_SCANCODE_NUMBER_4 = 33,
    KB_SCANCODE_NUMBER_5 = 34,
    KB_SCANCODE_NUMBER_6 = 35,
    KB_SCANCODE_NUMBER_7 = 36,
    KB_SCANCODE_NUMBER_8 = 37,
    KB_SCANCODE_NUMBER_9 = 38,
    KB_SCANCODE_NUMBER_0 = 39,
    KB_SCANCODE_ENTER = 40,
    KB_SCANCODE_ESCAPE = 41,
    KB_SCANCODE_BACKSPACE = 42,
    KB_SCANCODE_TAB = 43,
    KB_SCANCODE_SPACE = 44,
    KB_SCANCODE_MINUS = 45,
    KB_SCANCODE_EQUALS = 46,
    KB_SCANCODE_LEFT_BRACKET = 47,
    KB_SCANCODE_RIGHT_BRACKET = 48,
    KB_SCANCODE_BACK_SLASH = 49,
    KB_SCANCODE_SEMICOLON = 51,
    KB_SCANCODE_APOSTROPHE = 52,
    KB_SCANCODE_GRAVE = 53,
    KB_SCANCODE_COMMA = 54,
    KB_SCANCODE_PERIOD = 55,
    KB_SCANCODE_FORWARD_SLASH = 56,
    KB_SCANCODE_CAPS_LOCK = 57,
    KB_SCANCODE_F1 = 58,
    KB_SCANCODE_F2 = 59,
    KB_SCANCODE_F3 = 60,
    KB_SCANCODE_F4 = 61,
    KB_SCANCODE_F6 = 63,
    KB_SCANCODE_F7 = 64,
    KB_SCANCODE_F8 = 65,
    KB_SCANCODE_F5 = 62,
    KB_SCANCODE_F9 = 66,
    KB_SCANCODE_F10 = 67,
    KB_SCANCODE_F11 = 68,
    KB_SCANCODE_F12 = 69,
    KB_SCANCODE_PRINT_SCREEN = 70,
    KB_SCANCODE_SCROLL_LOCK = 71,
    KB_SCANCODE_PAUSE = 72,
    KB_SCANCODE_INSERT = 73,
    KB_SCANCODE_HOME = 74,
    KB_SCANCODE_PAGE_UP = 75,
    KB_SCANCODE_DELETE = 76,
    KB_SCANCODE_END = 77,
    KB_SCANCODE_PAGE_DOWN = 78,
    KB_SCANCODE_ARROW_RIGHT = 79,
    KB_SCANCODE_ARROW_LEFT = 80,
    KB_SCANCODE_ARROW_DOWN = 81,
    KB_SCANCODE_ARROW_UP = 82,
    KB_SCANCODE_NUMPAD_NUM_LOCK = 83,
    KB_SCANCODE_NUMPADDIVIDE = 84,
    KB_SCANCODE_NUMPAD_TIMES = 85,
    KB_SCANCODE_NUMPAD_MINUS = 86,
    KB_SCANCODE_NUMPAD_PLUS = 87,
    KB_SCANCODE_NUMPAD_ENTER = 88,
    KB_SCANCODE_NUMPAD_1 = 89,
    KB_SCANCODE_NUMPAD_2 = 90,
    KB_SCANCODE_NUMPAD_3 = 91,
    KB_SCANCODE_NUMPAD_4 = 92,
    KB_SCANCODE_NUMPAD_5 = 93,
    KB_SCANCODE_NUMPAD_6 = 94,
    KB_SCANCODE_NUMPAD_7 = 95,
    KB_SCANCODE_NUMPAD_8 = 96,
    KB_SCANCODE_NUMPAD_9 = 97,
    KB_SCANCODE_NUMPAD_0 = 98,
    KB_SCANCODE_NUMPAD_DOT = 99,
    KB_SCANCODE_NUMPAD_EQUALS = 103,
    KB_SCANCODE_MENU = 118,
    KB_SCANCODE_LEFT_CONTROL = 224,
    KB_SCANCODE_LEFT_SHIFT = 225,
    KB_SCANCODE_LEFT_ALT = 226,
    KB_SCANCODE_SUPER = 227,
    KB_SCANCODE_RIGHT_CONTROL = 228,
    KB_SCANCODE_RIGHT_SHIFT = 229,
    KB_SCANCODE_RIGHT_ALT = 230,
} ArgusKeyboardScancode;

/**
 * @brief Represents a command sent by a key press.
 *
 * Command keys are defined as those which are not representative of a
 * textual character nor a key modifier.
 */
typedef enum ArgusKeyboardCommand {
    KB_COMMAND_ESCAPE,
    KB_COMMAND_F1,
    KB_COMMAND_F2,
    KB_COMMAND_F3,
    KB_COMMAND_F4,
    KB_COMMAND_F5,
    KB_COMMAND_F6,
    KB_COMMAND_F7,
    KB_COMMAND_F8,
    KB_COMMAND_F9,
    KB_COMMAND_F10,
    KB_COMMAND_F11,
    KB_COMMAND_F12,
    KB_COMMAND_BACKSPACE,
    KB_COMMAND_TAB,
    KB_COMMAND_CAPS_LOCK,
    KB_COMMAND_ENTER,
    KB_COMMAND_MENU,
    KB_COMMAND_PRINT_SCREEN,
    KB_COMMAND_SCROLL_LOCK,
    KB_COMMAND_BREAK,
    KB_COMMAND_INSERT,
    KB_COMMAND_HOME,
    KB_COMMAND_PAGE_UP,
    KB_COMMAND_DELETE,
    KB_COMMAND_END,
    KB_COMMAND_PAGE_DOWN,
    KB_COMMAND_ARROW_UP,
    KB_COMMAND_ARROW_LEFT,
    KB_COMMAND_ARROW_DOWN,
    KB_COMMAND_ARROW_RIGHT,
    KB_COMMAND_NUMPAD_NUM_LOCK,
    KB_COMMAND_NUMPAD_ENTER,
    KB_COMMAND_NUMPAD_DOT,
    KB_COMMAND_SUPER,
} ArgusKeyboardCommand;

typedef enum ArgusKeyboardModifiers {
    KB_MODIFIER_NONE = 0x00,
    KB_MODIFIER_SHIFT = 0x01,
    KB_MODIFIER_CONTROL = 0x02,
    KB_MODIFIER_SUPER = 0x04,
    KB_MODIFIER_ALT = 0x08,
} ArgusKeyboardModifiers;

const char *argus_get_key_name(ArgusKeyboardScancode scancode);

bool argus_is_key_pressed(ArgusKeyboardScancode scancode);

#ifdef __cplusplus
}
#endif
