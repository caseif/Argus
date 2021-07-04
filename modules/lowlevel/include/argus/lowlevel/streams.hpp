/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <iostream>
#include <istream>
#include <streambuf>

namespace argus {
    struct MemBuf : std::streambuf {
        MemBuf(char *buf, size_t len) {
            this->setg(buf, buf, buf + len);
        }
    };

    struct MemIstream : std::istream {
        MemBuf membuf;

        MemIstream(char *buf, size_t len) :
            membuf(MemBuf(buf, len)),
            std::istream(&membuf) {
        }
    };
}
