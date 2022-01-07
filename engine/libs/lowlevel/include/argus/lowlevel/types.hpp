#pragma once

namespace argus {
    struct TriState {
        enum enum_type {
            Unset,
            True,
            False,
        };

        enum enum_type value;

        TriState(void): TriState(Unset) {
        }

        TriState(enum enum_type value):
            value(value) {
        }


        operator bool() {
            return value == True;
        }

        TriState &operator =(bool rhs) {
            value = rhs ? True : False;
            return *this;
        }

        bool is_set(void) {
            return value != Unset;
        }
    };
}
