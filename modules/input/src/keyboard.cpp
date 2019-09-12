#include "argus/keyboard.hpp"

namespace argus {

    bool KeyPress::is_character(void) {
        //TODO
    }

    bool KeyPress::is_command(void) {
        //TODO
    }

    bool KeyPress::is_modifier(void) {
        return is_modifier_key(scancode);
    }

    wchar_t KeyPress::get_character(void) {
        //TODO
    }

    KeyboardCommand KeyPress::get_command(void) {
        //TODO
    }

    KeyboardModifiers KeyPress::get_modifier(void) {
        //TODO
    }

    bool is_character_key(KeyboardScancode scancode) {
        //TODO
    }

    bool is_command_key(KeyboardScancode scancode) {
        //TODO
    }

    bool is_modifier_key(KeyboardScancode scancode) {
        //TODO
    }

    wchar_t get_key_character(KeyboardScancode scancode) {
        //TODO
    }

    KeyboardCommand get_key_command(KeyboardScancode scancode) {
        //TODO
    }

    KeyboardModifiers get_key_modifier(KeyboardScancode scancode) {
        //TODO
    }

}
