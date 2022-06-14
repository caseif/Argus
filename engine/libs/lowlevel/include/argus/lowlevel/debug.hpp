#pragma once

#include "argus/lowlevel/logging.hpp"

#define _ARGUS_ASSERT(c, fmt)  if (!(c)) { \
                                        Logger::default_logger().fatal("Assertion failed: " #c " (" fmt ")"); \
                                    }

#define _ARGUS_ASSERT_F(c, fmt, ...)  if (!(c)) { \
                                        Logger::default_logger().fatal("Assertion failed: " #c " (" fmt ")", ##__VA_ARGS__); \
                                    }
