#pragma once

#include <cassert>
#include <cstring>
#include <string>

namespace argus {

    static std::string g_cur_err;

    inline std::string &get_error(void) {
        return g_cur_err;
    }

    inline void set_error(std::string &&msg) {
        g_cur_err = msg;
    }

}
