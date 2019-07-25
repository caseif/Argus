#pragma once

namespace argus {

    #define FATAL(s)    std::cerr << s << std::endl;    \
                        exit(1)

    #define ASSERT(c, s)    if (!(c)) {     \
                                FATAL(s);   \
                            }

}
