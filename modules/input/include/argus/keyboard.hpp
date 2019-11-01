/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include "argus/core.hpp"

namespace argus {

    /**
     * \brief Represents a scancode tied to a key press.
     *
     * Argus's scancode definitions are based on a 104-key QWERTY layout.
     *
     * Scancodes are indiciative of the location of a pressed key on the
     * keyboard, but the actual value of the key will depend on the current
     * keyboard layout. For instance, KeyboardScancode::Q will correspond to a
     * press of the "A" key if an AZERTY layout is active.
     */
    enum class KeyboardScancode {
        A = 4,
        B = 5,
        C = 6,
        D = 7,
        E = 8,
        F = 9,
        G = 10,
        H = 11,
        I = 12,
        J = 13,
        K = 14,
        L = 15,
        M = 16,
        N = 17,
        O = 18,
        P = 19,
        Q = 20,
        R = 21,
        S = 22,
        T = 23,
        U = 24,
        V = 25,
        W = 26,
        X = 27,
        Y = 28,
        Z = 29,
        NUMBER_1 = 30,
        NUMBER_2 = 31,
        NUMBER_3 = 32,
        NUMBER_4 = 33,
        NUMBER_5 = 34,
        NUMBER_6 = 35,
        NUMBER_7 = 36,
        NUMBER_8 = 37,
        NUMBER_9 = 38,
        NUMBER_0 = 39,
        ENTER = 40,
        ESCAPE = 41,
        BACKSPACE = 42,
        TAB = 43,
        SPACE = 44,
        MINUS = 45,
        EQUALS = 46,
        OPEN_SQ_BRACKET = 47,
        CLOSE_SQ_BRACKET = 48,
        BACK_SLASH = 49,
        SEMICOLON = 51,
        APOSTROPHE = 52,
        GRAVE = 53,
        COMMA = 54,
        PERIOD = 55,
        FORWARD_SLASH = 56,
        CAPS_LOCK = 57,
        F1 = 58,
        F2 = 59,
        F3 = 60,
        F4 = 61,
        F6 = 63,
        F7 = 64,
        F8 = 65,
        F5 = 62,
        F9 = 66,
        F10 = 67,
        F11 = 68,
        F12 = 69,
        PRINT_SCREEN = 70,
        SCROLL_LOCK = 71,
        PAUSE = 72,
        INSERT = 73,
        HOME = 74,
        PAGE_UP = 75,
        DELETE = 76,
        END = 77,
        PAGE_DOWN = 78,
        ARROW_RIGHT = 79,
        ARROW_LEFT = 80,
        ARROW_DOWN = 81,
        ARROW_UP = 82,
        NP_NUM_LOCK = 83,
        NP_DIVIDE = 84,
        NP_TIMES = 85,
        NP_MINUS = 86,
        NP_PLUS = 87,
        NP_ENTER = 88,
        NP_1 = 89,
        NP_2 = 90,
        NP_3 = 91,
        NP_4 = 92,
        NP_5 = 93,
        NP_6 = 94,
        NP_7 = 95,
        NP_8 = 96,
        NP_9 = 97,
        NP_0 = 98,
        NP_DOT = 99,
        MENU = 118,
        LEFT_CONTROL = 224,
        LEFT_SHIFT = 225,
        LEFT_ALT = 226,
        SUPER = 227,
        RIGHT_CONTROL = 228,
        RIGHT_SHIFT = 229,
        RIGHT_ALT = 230,
    };

    /**
     * \brief Represents a command sent by a key press.
     *
     * Command keys are defined as those which are not representative of a
     * textual character nor a key modifier.
     */
    enum class KeyboardCommand {
        ESCAPE,
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,
        BACKSPACE,
        CAPS_LOCK,
        ENTER,
        MENU,
        PRINT_SCREEN,
        SCROLL_LOCK,
        BREAK,
        INSERT,
        HOME,
        PAGE_UP,
        DELETE,
        END,
        PAGE_DOWN,
        ARROW_UP,
        ARROW_LEFT,
        ARROW_DOWN,
        ARROW_RIGHT,
        NP_NUM_LOCK,
        NP_ENTER,
        NP_DOT
    };

    /**
     * \brief Represents a modifier enabled by a key press.
     *
     * Modifier keys are defined as the left and right shift, alt, and control
     * keys, the Super key, and the num lock, caps lock, and scroll lock
     * toggles.
     */
    enum class KeyboardModifiers : uint16_t {
        LEFT_SHIFT      = 0x01,
        RIGHT_SHIFT     = 0x02,
        LEFT_CONTROL    = 0x04,
        SUPER           = 0x08,
        LEFT_ALT        = 0x10,
        RIGHT_ALT       = 0x20,
        RIGHT_CONTROL   = 0x40,
        NUM_LOCK        = 0x80,
        CAPS_LOCK       = 0x10,
        SCROLL_LOCK     = 0x200,
        SHIFT           = LEFT_SHIFT | RIGHT_SHIFT,
        CONTROL         = LEFT_CONTROL | RIGHT_CONTROL,
        ALT             = LEFT_ALT | RIGHT_ALT
    };

    /**
     * \brief Represents a specific type of keyboard event.
     */
    enum class KeyboardEventType {
        /**
         * \brief A key has been pressed down.
         */
        KEY_DOWN,
        /**
         * \brief A key has been released.
         */
        KEY_UP
    };

    /**
     * \brief Represents a press of a keyboard key, providing access to
     * inforrmation regarding the emitted scancode, the active modifiers, and
     * the semantic value of the key press.
     */
    struct KeyboardEvent : public ArgusEvent {
        /**
         * \brief The particular \link KeyboardEventType type \endlink of
         * KeyboardEvent.
         */
        const KeyboardEventType subtype;

        /**
         * \brief The scancode emitted by this key press.
         */
        const KeyboardScancode scancode;
        /**
         * \brief The modifiers active during this key press.
         *
         * If the key press is associated with a modifier key, said key will not
         * be included by this field.
         */
        const KeyboardModifiers modifiers;

        /**
         * \brief Gets whether the key press is representative of a textual
         * character.
         *
         * \return Whether the key press is representative of a textual
         * character.
         */
        bool is_character(void);

        /**
         * \brief Gets whether the key press is representative of a command.
         *
         * \return Whether the key press is representative of a command.
         */
        bool is_command(void);

        /**
         * \brief Gets whether the key press is representative of a key
         * modifier.
         *
         * \return Whether the key press is representative of a key modifier.
         */
        bool is_modifier(void);

        /**
         * \brief Gets the textual character associated with this key press,
         * taking key modifiers into account.
         *
         * This will throw invalid_argument if is_character returns false.
         *
         * \return The textual character associated with this key press.
         */
        wchar_t get_character(void);

        /**
         * \brief Gets the command associated with this key press, taking key
         * modifiers into account.
         *
         * This will throw invalid_argument if is_command returns false.
         *
         * \return The command associated with this key press.
         */
        KeyboardCommand get_command(void);

        /**
         * \brief Gets the key modifier associated with this key press.
         *
         * This will throw invalid_argument if is_modifier returns false.
         *
         * \return The key modifier associated with this key press.
         */
        KeyboardModifiers get_modifier(void);
    };

    /**
     * \brief Gets whether the given scancode is associated with a textual
     * character key.
     *
     * \return Whether the given scancode is associated with a textual character
     * key.
     */
    bool is_character_key(KeyboardScancode scancode);

    /**
     * \brief Gets whether the given scancode is associated with a command key.
     *
     * \return Whether the given scancode is associated with a command key.
     */
    bool is_command_key(KeyboardScancode scancode);

    /**
     * \brief Gets whether the given scancode is associated with a modifier key.
     *
     * \return Whether the given scancode is associated with a modifier key.
     */
    bool is_modifier_key(KeyboardScancode scancode);

    /**
     * \brief Gets the character associated with the key represented by the
     * given scancode, not taking key modifiers into account.
     *
     * \return The character for the given scancode.
     */
    wchar_t get_key_character(KeyboardScancode scancode);

    /**
     * \brief Gets the command associated with the key represented by the
     * given scancode, not taking key modifiers into account.
     *
     * \return The command for the given scancode.
     */
    KeyboardCommand get_key_command(KeyboardScancode scancode);

    /**
     * \brief Gets the key modifier associated with the key represented by the
     * given scancode.
     *
     * \return The key modifier for the given scancode.
     */
    KeyboardModifiers get_key_modifier(KeyboardScancode scancode);

}
