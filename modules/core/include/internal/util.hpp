#pragma once

#include <cstdio>
#include <cstdlib>

namespace argus {

    #define WARN(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)

    #define FATAL(fmt, ...) WARN(fmt, ##__VA_ARGS__); \
                            exit(1)

    #define ASSERT(c, fmt, ...) if (!(c)) {     \
                                        FATAL(fmt, ##__VA_ARGS__);   \
                                    }

}
