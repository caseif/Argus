#pragma once

#include <functional>

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
        TILDE,
        NUMBER_0,
        NUMBER_1,
        NUMBER_2,
        NUMBER_3,
        NUMBER_4,
        NUMBER_5,
        NUMBER_6,
        NUMBER_7,
        NUMBER_8,
        NUMBER_9,
        MINUS,
        EQUALS,
        BACKSPACE,
        TAB,
        O,
        W,
        E,
        R,
        T,
        Y,
        U,
        I,
        Q,
        P,
        OPEN_SQ_BRACKET,
        CLOSE_SQ_BRACKET,
        BACK_SLASH,
        CAPS_LOCK,
        A,
        S,
        D,
        F,
        B,
        G,
        H,
        J,
        K,
        L,
        SEMICOLON,
        APOSTROPHE,
        ENTER,
        LEFT_SHIFT,
        Z,
        C,
        X,
        V,
        N,
        M,
        COMMA,
        PERIOD,
        FORWARD_SLASH,
        RIGHT_SHIFT,
        LEFT_CONTROL,
        SUPER,
        LEFT_ALT,
        SPACE,
        RIGHT_ALT,
        FN,
        MENU,
        RIGHT_CONTROL,
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
        NP_DIVIDE,
        NP_TIMES,
        NP_MINUS,
        NP_7,
        NP_8,
        NP_9,
        NP_PLUS,
        NP_4,
        NP_5,
        NP_6,
        NP_1,
        NP_2,
        NP_3,
        NP_ENTER,
        NP_0,
        NP_DOT
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
     * keys, as well as the Super and Fn keys.
     */
    enum class KeyboardModifiers : uint16_t {
        LEFT_SHIFT  = 0x01,
        RIGHT_SHIFT = 0x02,
        LEFT_CONTROL   = 0x04,
        SUPER       = 0x08,
        LEFT_ALT    = 0x10,
        RIGHT_ALT   = 0x20,
        FN          = 0x40,
        RIGHT_CONTROL  = 0x80
    };

    /**
     * \brief Represents a press of a keyboard key, providing acces to
     * inforrmation regarding the emitted scancode, the active modifiers, and
     * the semantic value of the key press.
     */
    struct KeyPress {
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
