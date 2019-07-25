#pragma once

#include <cstdio>
#include <cstdlib>

namespace argus {

    #define FATAL(fmt, ...) printf(fmt, ##__VA_ARGS__); \
                            exit(1)

    #define ASSERT(c, fmt, ...) if (!(c)) {     \
                                        FATAL(fmt, ##__VA_ARGS__);   \
                                    }

}
