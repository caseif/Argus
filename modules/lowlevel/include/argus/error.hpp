#pragma once

#include <cassert>
#include <iostream>
#include <cstring>
#include <string>

#ifdef _ARGUS_DEBUG_MODE
#define ARGUS_PRINT_ERRORS
#endif

namespace argus {

    static std::string g_cur_err;

    inline std::string &get_error(void) {
        return g_cur_err;
    }

    inline void set_error(std::string &&msg) {
        #ifdef ARGUS_PRINT_ERRORS
        std::cout << msg << std::endl;
        #endif
        g_cur_err = msg;
    }

}
