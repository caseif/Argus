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
        MemBuf(const char *buf, size_t len) {
            this->setg(const_cast<char*>(buf), const_cast<char*>(buf), const_cast<char*>(buf) + len);
        }
    };

    struct IMemStream : std::istream {
        MemBuf membuf;

        IMemStream(const void *buf, size_t len) :
            std::istream(&membuf),
            membuf(MemBuf(static_cast<const char*>(buf), len)) {
        }
    };
}
