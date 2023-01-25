/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

/**
 * \file argus/input/keyboard.hpp
 *
 * API for detecting and interpreting keyboard events.
 */

#pragma once

#include "argus/core/event.hpp"

#include "argus/wm/window.hpp"

namespace argus::input {
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
        Unknown = 0,
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
        Number1 = 30,
        Number2 = 31,
        Number3 = 32,
        Number4 = 33,
        Number5 = 34,
        Number6 = 35,
        Number7 = 36,
        Number8 = 37,
        Number9 = 38,
        Number0 = 39,
        Enter = 40,
        Escape = 41,
        Backspace = 42,
        Tab = 43,
        Space = 44,
        Minus = 45,
        Equals = 46,
        LeftBracket = 47,
        RightBracket = 48,
        BackSlash = 49,
        Semicolon = 51,
        Apostrophe = 52,
        Grave = 53,
        Comma = 54,
        Period = 55,
        ForwardSlash = 56,
        CapsLock = 57,
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
        PrintScreen = 70,
        ScrollLock = 71,
        Pause = 72,
        Insert = 73,
        Home = 74,
        PageUp = 75,
        Delete = 76,
        End = 77,
        PageDown = 78,
        ArrowRight = 79,
        ArrowLeft = 80,
        ArrowDown = 81,
        ArrowUp = 82,
        NumpadNumLock = 83,
        NumpadDivide = 84,
        NumpadTimes = 85,
        NumpadMinus = 86,
        NumpadPlus = 87,
        NumpadEnter = 88,
        Numpad1 = 89,
        Numpad2 = 90,
        Numpad3 = 91,
        Numpad4 = 92,
        Numpad5 = 93,
        Numpad6 = 94,
        Numpad7 = 95,
        Numpad8 = 96,
        Numpad9 = 97,
        Numpad0 = 98,
        NumpadDot = 99,
        NumpadEquals = 103,
        Menu = 118,
        LeftControl = 224,
        LeftShift = 225,
        LeftAlt = 226,
        Super = 227,
        RightControl = 228,
        RightShift = 229,
        RightAlt = 230,
    };

    /**
     * \brief Represents a command sent by a key press.
     *
     * Command keys are defined as those which are not representative of a
     * textual character nor a key modifier.
     */
    enum class KeyboardCommand {
        Escape,
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
        Backspace,
        Tab,
        CapsLock,
        Enter,
        Menu,
        PrintScreen,
        ScrollLock,
        Break,
        Insert,
        Home,
        PageUp,
        Delete,
        End,
        PageDown,
        ArrowUp,
        ArrowLeft,
        ArrowDown,
        ArrowRight,
        NumpadNumLock,
        NumpadEnter,
        NumpadDot,
        Super
    };

    /**
     * \brief Represents a modifier enabled by a key press.
     *
     * Modifier keys are defined as the left and right shift, alt, and control
     * keys, the Super key, and the num lock, caps lock, and scroll lock
     * toggles.
     */
    // clang-format off
    enum class KeyboardModifiers : uint16_t {
        None = 0x00,
        Shift = 0x01,
        Control = 0x02,
        Super = 0x04,
        Alt = 0x08
    };
    // clang-format on

    /**
     * \brief Bitwise OR implementation for KeyboardModifiers bitmask elements.
     *
     * \param lhs Left-hand operand.
     * \param rhs Right-hand operand.
     *
     * \return The bitwise OR of the operands.
     */
    constexpr inline KeyboardModifiers operator|(KeyboardModifiers lhs, KeyboardModifiers rhs);

    /**
     * \brief Bitwise OR-assignment implementation for KeyboardModifiers bitmask
     *        elements.
     *
     * \param lhs Left-hand operand.
     * \param rhs Right-hand operand.
     *
     * \return The bitwise OR of the operands.
     *
     * \sa KeyboardModifiers::operator|
     */
    inline KeyboardModifiers operator|=(KeyboardModifiers &lhs, KeyboardModifiers rhs);

    /**
     * \brief Bitwise AND implementation for KeyboardModifiers bitmask elements.
     *
     * \param lhs Left-hand operand.
     * \param rhs Right-hand operand.
     *
     * \return The bitwise AND of the operands.
     */
    constexpr inline KeyboardModifiers operator&(KeyboardModifiers lhs, KeyboardModifiers rhs);

    /**
     * \brief Gets the semantic name of the key associated with the given
     *        scancode.
     *
     * \return The name of the key.
     */
    std::string get_key_name(KeyboardScancode scancode);

    /**
     * \brief Gets whether the key associated with a scancode is currently being
     *        pressed down.
     *
     * \param window The window to query input for.
     * \param scancode The scancode to query.
     *
     * \return Whether the key is being pressed.
     */
    bool keyboard_key_down(const Window &window, KeyboardScancode scancode);
}
