/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstdarg>
#include "test_global.hpp"

#include "argus/lowlevel/crash.hpp"
#include "argus/lowlevel/result.hpp"

#define MSG_BUF_LEN 255

static std::string format_str(const char *format, va_list args) {
    char msg_buf[MSG_BUF_LEN];
    snprintf(msg_buf, sizeof(msg_buf), format, args);
    return msg_buf;
}

struct InterceptedCrash : std::runtime_error {

    InterceptedCrash(const char *msg, va_list args):
        std::runtime_error(format_str(msg, args)) {
    }
};

[[noreturn]] static void intercept_crash(const char *msg, va_list args) {
    throw InterceptedCrash(msg, args);
}

SCENARIO("Result class behaves correctly", "[Result]") {
    argus::set_ll_crash_callback(intercept_crash);

    GIVEN("An OK Result<int, int>") {
        argus::Result<int, int> result = argus::ok<int, int>(42);

        THEN("is_ok returns true") {
            REQUIRE(result.is_ok());
        }

        THEN("is_err returns false") {
            REQUIRE(!result.is_err());
        }

        THEN("unwrap returns correct value") {
            REQUIRE(result.unwrap() == 42);
        }

        THEN("expect returns correct value") {
            REQUIRE(result.expect("expect failed") == 42);
        }

        THEN("unwrap_err triggers crash") {
            REQUIRE_THROWS(result.unwrap_err());
        }

        THEN("expect_err triggers crash") {
            REQUIRE_THROWS(result.expect_err("expect_err failed"));
        }

        THEN("unwrap_or_default returns original value") {
            REQUIRE(result.unwrap_or_default(1337) == 42);
        }

        THEN("map returns OK Result") {
            auto mapped = result.map<int>([](auto val) { return val + 1; });
            REQUIRE(mapped.is_ok());
            AND_THEN("map returns correct mapped value") {
                REQUIRE(mapped.unwrap() == 43);
            }
        }

        THEN("map_err returns OK Result") {
            auto mapped = result.map_err<int>([](auto err) { return err + 1; });
            REQUIRE(mapped.is_ok());
            AND_THEN("map_err returns original value") {
                REQUIRE(mapped.unwrap() == 42);
            }
        }

        THEN("map_or returns correct mapped value") {
            REQUIRE(result.map_or<int>(1337, [](auto val) { return val + 1; }) == 43);
        }

        THEN("map_or_else returns correct mapped value") {
            REQUIRE(result.map_or_else<int, int>([](auto _) { return 1337; },
                    [](auto val) { return val + 1; }) == 43);
        }

        THEN("or_else returns OK Result") {
            auto mapped = result.or_else<int>([](auto _) { return argus::err<int, int>(1337); });
            REQUIRE(mapped.is_ok());
            AND_THEN("or_else returns original value") {
                REQUIRE(mapped.unwrap() == 42);
            }
        }

        THEN("otherwise returns OK Result") {
            auto mapped = result.otherwise(argus::err<int, int>(1337));
            REQUIRE(mapped.is_ok());
            AND_THEN("otherwise returns original value") {
                REQUIRE(mapped.unwrap() == 42);
            }
        }

        THEN("collate with OK Result returns OK Result") {
            auto collated = result.collate(argus::ok<int, int>(1337));
            REQUIRE(collated.is_ok());
            AND_THEN("unwrap returns correct value") {
                REQUIRE(collated.unwrap() == 1337);
            }
        }

        THEN("collate with error Result returns error Result") {
            auto collated = result.collate(argus::err<int, int>(1337));
            REQUIRE(collated.is_err());
            AND_THEN("unwrap_err returns correct value") {
                REQUIRE(collated.unwrap_err() == 1337);
            }
        }

        THEN("and_then returns OK Result") {
            auto combined = result.and_then<int>([](auto val) { return val + 1; });
            REQUIRE(combined.is_ok());
            AND_THEN("unwrap returns correct value") {
                REQUIRE(combined.unwrap() == 43);
            }
        }
    }

    GIVEN("An error Result<int, int>") {
        argus::Result<int, int> result = argus::err<int, int>(42);

        THEN("is_ok returns false") {
            REQUIRE(!result.is_ok());
        }

        THEN("is_err returns true") {
            REQUIRE(result.is_err());
        }

        THEN("unwrap triggers crash") {
            REQUIRE_THROWS(result.unwrap());
        }

        THEN("expect triggers crash") {
            REQUIRE_THROWS(result.expect("expect failed"));
        }

        THEN("unwrap_err returns correct value") {
            REQUIRE(result.unwrap_err() == 42);
        }

        THEN("expect_err returns correct value") {
            REQUIRE(result.expect_err("expect_err failed") == 42);
        }

        THEN("unwrap_or_default returns default value") {
            REQUIRE(result.unwrap_or_default(1337) == 1337);
        }

        THEN("map returns error Result") {
            auto mapped = result.map<int>([](auto err) { return err + 1; });
            REQUIRE(mapped.is_err());
            AND_THEN("map returns original value") {
                REQUIRE(mapped.unwrap_err() == 42);
            }
        }

        THEN("map_err returns error Result") {
            auto mapped = result.map_err<int>([](auto val) { return val + 1; });
            REQUIRE(mapped.is_err());
            AND_THEN("map_err returns correct mapped value") {
                REQUIRE(mapped.unwrap_err() == 43);
            }
        }

        THEN("map_or returns default value") {
            REQUIRE(result.map_or<int>(1337, [](auto val) { return val + 1; }) == 1337);
        }

        THEN("map_or_else returns correct mapped value") {
            REQUIRE(result.map_or_else<int, int>([](auto err) { return err + 1; },
                    [](auto _) { return 1337; }) == 43);
        }

        THEN("or_else returns fallback OK Result") {
            auto mapped = result.or_else<int>([](auto err) { return argus::ok<int, int>(err + 1); });
            REQUIRE(mapped.is_ok());
            AND_THEN("or_else returns correct fallback value") {
                REQUIRE(mapped.unwrap() == 43);
            }
        }

        THEN("otherwise returns fallback OK Result") {
            auto mapped = result.otherwise(argus::ok<int, int>(1337));
            REQUIRE(mapped.is_ok());
            AND_THEN("otherwise returns correct fallback value") {
                REQUIRE(mapped.unwrap() == 1337);
            }
        }

        THEN("collate with OK Result returns error Result") {
            auto collated = result.collate(argus::ok<int, int>(1337));
            REQUIRE(collated.is_err());
            AND_THEN("unwrap_err returns original value") {
                REQUIRE(collated.unwrap_err() == 42);
            }
        }

        THEN("collate with error Result returns error Result") {
            auto collated = result.collate(argus::err<int, int>(1337));
            REQUIRE(collated.is_err());
            AND_THEN("unwrap_err returns original value") {
                REQUIRE(collated.unwrap_err() == 42);
            }
        }

        THEN("and_then returns error Result") {
            auto combined = result.and_then<int>([](auto val) { return val + 1; });
            REQUIRE(combined.is_err());
            AND_THEN("unwrap_err returns original value") {
                REQUIRE(combined.unwrap_err() == 42);
            }
        }
    }

    GIVEN("An OK Result<int &, int &>") {
        int val = 42;
        auto result = argus::ok<int &, int &>(val);

        THEN("is_ok returns true") {
            REQUIRE(result.is_ok());
        }

        THEN("is_err returns false") {
            REQUIRE(!result.is_err());
        }

        THEN("unwrap returns correct value") {
            REQUIRE(result.unwrap() == 42);
        }

        THEN("unwrap returns correct value after referenced value is updated") {
            val = 43; // NOLINT(*-deadcode.DeadStores)
            REQUIRE(result.unwrap() == 43);
            val = 42; // NOLINT(*-deadcode.DeadStores)
        }

        THEN("assigning to unwrap return value updates referenced value") {
            result.unwrap() = 43;
            REQUIRE(val == 43);
            val = 42; // NOLINT(*-deadcode.DeadStores)
        }

        THEN("expect returns correct value") {
            REQUIRE(result.expect("expect failed") == 42);
        }

        THEN("expect returns correct value after referenced value is updated") {
            val = 43; // NOLINT(*-deadcode.DeadStores)
            REQUIRE(result.expect("expect failed") == 43);
            val = 42; // NOLINT(*-deadcode.DeadStores)
        }

        THEN("assigning to expect return value updates referenced value") {
            result.expect("expect failed") = 43;
            REQUIRE(val == 43);
            val = 42; // NOLINT(*-deadcode.DeadStores)
        }

        THEN("unwrap_err triggers crash") {
            REQUIRE_THROWS(result.unwrap_err());
        }

        THEN("expect_err triggers crash") {
            REQUIRE_THROWS(result.expect_err("expect_err failed"));
        }

        THEN("unwrap_or_default returns original value") {
            REQUIRE(result.unwrap_or_default(1337) == 42);
        }

        THEN("map returns OK Result") {
            int other = 1337;
            auto mapped = result.map<int &>([&other](int &_) -> int & { return other; });
            REQUIRE(mapped.is_ok());
            AND_THEN("map returns correct mapped value") {
                REQUIRE(mapped.unwrap() == 1337);
            }
        }

        THEN("map_err returns OK Result") {
            int other = 1337;
            auto mapped = result.map_err<int &>([&other](auto &_) -> int & { return other; });
            REQUIRE(mapped.is_ok());
            AND_THEN("map_err returns original value") {
                REQUIRE(mapped.unwrap() == 42);
            }
        }

        THEN("map_or returns correct mapped value") {
            int other = 1337;
            int other_2 = 1338;
            REQUIRE(result.map_or<int>(other, [&other_2](auto &_) -> int & { return other_2; }) == 1338);
        }

        THEN("map_or_else returns correct mapped value") {
            int other = 1337;
            int other_2 = 1338;
            REQUIRE(result.map_or_else<int &, int &>([&other](auto &_) -> int & { return other; },
                    [&other_2](auto &_) -> int & { return other_2; }) == 1338);
        }

        THEN("or_else returns OK Result") {
            int other = 1337;
            auto mapped = result.or_else<int &>([&other](auto &_) { return argus::err<int &, int &>(other); });
            REQUIRE(mapped.is_ok());
            AND_THEN("or_else returns original value") {
                REQUIRE(mapped.unwrap() == 42);
            }
        }

        THEN("otherwise returns OK Result") {
            int err_val = 1337;
            auto mapped = result.otherwise(argus::err<int &, int &>(err_val));
            REQUIRE(mapped.is_ok());
            AND_THEN("otherwise returns original value") {
                REQUIRE(mapped.unwrap() == 42);
            }
        }

        THEN("collate with OK Result returns OK Result") {
            auto other = 1337;
            auto collated = result.collate(argus::ok<int &, int &>(other));
            REQUIRE(collated.is_ok());
            AND_THEN("unwrap returns correct value") {
                REQUIRE(collated.unwrap() == 1337);
            }
        }

        THEN("collate with error Result returns error Result") {
            auto other = 1337;
            auto collated = result.collate(argus::err<int &, int &>(other));
            REQUIRE(collated.is_err());
            AND_THEN("unwrap_err returns correct value") {
                REQUIRE(collated.unwrap_err() == 1337);
            }
        }

        THEN("and_then returns OK Result") {
            auto other = 1337;
            auto combined = result.and_then<int &>([&other](int &_) -> int & { return other; });
            REQUIRE(combined.is_ok());
            AND_THEN("unwrap returns correct value") {
                REQUIRE(combined.unwrap() == 1337);
            }
        }
    }

    GIVEN("An error Result<int &, int &>") {
        int val = 42;

        argus::Result<int &, int &> result = argus::err<int &, int &>(val);

        THEN("is_ok returns false") {
            REQUIRE(!result.is_ok());
        }

        THEN("is_err returns true") {
            REQUIRE(result.is_err());
        }

        THEN("unwrap triggers crash") {
            REQUIRE_THROWS(result.unwrap());
        }

        THEN("expect triggers crash") {
            REQUIRE_THROWS(result.expect("expect failed"));
        }

        THEN("unwrap_err returns correct value") {
            REQUIRE(result.unwrap_err() == 42);
        }

        THEN("unwrap_err returns correct value after referenced value is updated") {
            val = 43; // NOLINT(*-deadcode.DeadStores)
            REQUIRE(result.unwrap_err() == 43);
            val = 42; // NOLINT(*-deadcode.DeadStores)
        }

        THEN("assigning to unwrap_err return value updates referenced value") {
            result.unwrap_err() = 43;
            REQUIRE(val == 43);
            val = 42; // NOLINT(*-deadcode.DeadStores)
        }

        THEN("expect_err returns correct value") {
            REQUIRE(result.expect_err("expect_err failed") == 42);
        }

        THEN("expect_err returns correct value after referenced value is updated") {
            val = 43; // NOLINT(*-deadcode.DeadStores)
            REQUIRE(result.expect_err("expect_err failed") == 43);
            val = 42; // NOLINT(*-deadcode.DeadStores)
        }

        THEN("assigning to expect_err return value updates referenced value") {
            result.expect_err("expect_err failed") = 43;
            REQUIRE(val == 43);
            val = 42; // NOLINT(*-deadcode.DeadStores)
        }

        THEN("unwrap_or_default returns default value") {
            REQUIRE(result.unwrap_or_default(1337) == 1337);
        }

        THEN("map returns error Result") {
            int other = 1337;
            auto mapped = result.map<int &>([&other](auto &_) -> int & { return other; });
            REQUIRE(mapped.is_err());
            AND_THEN("map returns original value") {
                REQUIRE(mapped.unwrap_err() == 42);
            }
        }

        THEN("map_err returns error Result") {
            int other = 1337;
            auto mapped = result.map_err<int &>([&other](auto &_) -> int & { return other; });
            REQUIRE(mapped.is_err());
            AND_THEN("map_err returns correct mapped value") {
                REQUIRE(mapped.unwrap_err() == 1337);
            }
        }

        THEN("map_or returns default value") {
            int other = 1337;
            int other_2 = 1338;
            REQUIRE(result.map_or<int &>(other, [&other_2](auto &_) -> int & { return other_2; }) == 1337);
        }

        THEN("map_or_else returns correct mapped value") {
            REQUIRE(result.map_or_else<int, int>([](auto err) { return err + 1; },
                    [](auto _) { return 1337; }) == 43);
        }

        THEN("or_else returns fallback OK Result") {
            int other = 1337;
            auto mapped = result.or_else<int>([&other](auto _) { return argus::ok<int &, int>(other); });
            REQUIRE(mapped.is_ok());
            AND_THEN("or_else returns correct fallback value") {
                REQUIRE(mapped.unwrap() == 1337);
            }
        }

        THEN("otherwise returns fallback OK Result") {
            int ok_val = 1337;
            auto mapped = result.otherwise(argus::ok<int &, int &>(ok_val));
            REQUIRE(mapped.is_ok());
            AND_THEN("otherwise returns correct fallback value") {
                REQUIRE(mapped.unwrap() == 1337);
            }
        }

        THEN("collate with OK Result returns error Result") {
            auto other = 1337;
            auto collated = result.collate(argus::ok<int &, int &>(other));
            REQUIRE(collated.is_err());
            AND_THEN("unwrap_err returns original value") {
                REQUIRE(collated.unwrap_err() == 42);
            }
        }

        THEN("collate with error Result returns error Result") {
            auto other = 1337;
            auto collated = result.collate(argus::err<int &, int &>(other));
            REQUIRE(collated.is_err());
            AND_THEN("unwrap_err returns original value") {
                REQUIRE(collated.unwrap_err() == 42);
            }
        }

        THEN("and_then returns error Result") {
            auto other = 1337;
            auto combined = result.and_then<int &>([&other](int &_) -> int & { return other; });
            REQUIRE(combined.is_err());
            AND_THEN("unwrap_err returns original value") {
                REQUIRE(combined.unwrap_err() == 42);
            }
        }
    }

    GIVEN("An OK Result<void, int>") {
        argus::Result<void, int> result = argus::ok<void, int>();

        THEN("is_ok returns true") {
            REQUIRE(result.is_ok());
        }

        THEN("is_err returns false") {
            REQUIRE(!result.is_err());
        }

        THEN("expect succeeds") {
            REQUIRE_NOTHROW(result.expect("expect failed"));
        }

        THEN("unwrap_err triggers crash") {
            REQUIRE_THROWS(result.unwrap_err());
        }

        THEN("expect_err triggers crash") {
            REQUIRE_THROWS(result.expect_err("expect_err failed"));
        }

        THEN("map_err returns OK Result") {
            auto mapped = result.map_err<int>([](auto err) { return err + 1; });
            REQUIRE(mapped.is_ok());
        }

        THEN("or_else returns OK Result") {
            auto mapped = result.or_else<int>([](auto _) { return argus::err<void, int>(1337); });
            REQUIRE(mapped.is_ok());
        }

        THEN("otherwise returns OK Result") {
            auto mapped = result.otherwise(argus::err<void, int>(1337));
            REQUIRE(mapped.is_ok());
        }

        THEN("collate with OK Result returns OK Result") {
            auto collated = result.collate(argus::ok<void, int>());
            REQUIRE(collated.is_ok());
        }

        THEN("collate with error Result returns error Result") {
            auto collated = result.collate(argus::err<void, int>(1337));
            REQUIRE(collated.is_err());
            AND_THEN("unwrap_err returns correct value") {
                REQUIRE(collated.unwrap_err() == 1337);
            }
        }

        THEN("and_then returns OK Result") {
            auto combined = result.and_then<int>([]() { return 1337; });
            REQUIRE(combined.is_ok());
            AND_THEN("unwrap returns correct value") {
                REQUIRE(combined.unwrap() == 1337);
            }
        }
    }

    GIVEN("An error Result<void, int>") {
        argus::Result<void, int> result = argus::err<void, int>(42);

        THEN("is_ok returns false") {
            REQUIRE(!result.is_ok());
        }

        THEN("is_err returns true") {
            REQUIRE(result.is_err());
        }

        THEN("expect triggers crash") {
            REQUIRE_THROWS(result.expect("expect failed"));
        }

        THEN("unwrap_err returns correct value") {
            REQUIRE(result.unwrap_err() == 42);
        }

        THEN("expect_err returns correct value") {
            REQUIRE(result.expect_err("expect_err failed") == 42);
        }

        THEN("map_err returns error Result") {
            auto mapped = result.map_err<int>([](auto val) { return val + 1; });
            REQUIRE(mapped.is_err());
            AND_THEN("map_err returns correct mapped value") {
                REQUIRE(mapped.unwrap_err() == 43);
            }
        }

        THEN("or_else returns fallback OK Result") {
            auto mapped = result.or_else<int>([](auto _) { return argus::ok<void, int>(); });
            REQUIRE(mapped.is_ok());
        }

        THEN("otherwise returns fallback OK Result") {
            auto mapped = result.otherwise(argus::ok<void, int>());
            REQUIRE(mapped.is_ok());
        }

        THEN("collate with OK Result returns error Result") {
            auto collated = result.collate(argus::ok<void, int>());
            REQUIRE(collated.is_err());
            AND_THEN("unwrap_err returns original value") {
                REQUIRE(collated.unwrap_err() == 42);
            }
        }

        THEN("collate with error Result returns error Result") {
            auto collated = result.collate(argus::err<void, int>(1337));
            REQUIRE(collated.is_err());
            AND_THEN("unwrap_err returns original value") {
                REQUIRE(collated.unwrap_err() == 42);
            }
        }

        THEN("and_then returns error Result") {
            auto combined = result.and_then<int>([]() { return 1337; });
            REQUIRE(combined.is_err());
            AND_THEN("unwrap_err returns original value") {
                REQUIRE(combined.unwrap_err() == 42);
            }
        }
    }

    GIVEN("An OK Result<int, void>") {
        argus::Result<int, void> result = argus::ok<int, void>(42);

        THEN("is_ok returns true") {
            REQUIRE(result.is_ok());
        }

        THEN("is_err returns false") {
            REQUIRE(!result.is_err());
        }

        THEN("unwrap returns correct value") {
            REQUIRE(result.unwrap() == 42);
        }

        THEN("expect returns correct value") {
            REQUIRE(result.expect("expect failed") == 42);
        }

        THEN("expect_err triggers crash") {
            REQUIRE_THROWS(result.expect_err("expect_err failed"));
        }

        THEN("unwrap_or_default returns original value") {
            REQUIRE(result.unwrap_or_default(1337) == 42);
        }

        THEN("map returns OK Result") {
            auto mapped = result.map<int>([](auto val) { return val + 1; });
            REQUIRE(mapped.is_ok());
            AND_THEN("map returns correct mapped value") {
                REQUIRE(mapped.unwrap() == 43);
            }
        }

        THEN("map_or returns correct mapped value") {
            REQUIRE(result.map_or<int>(1337, [](auto val) { return val + 1; }) == 43);
        }

        THEN("or_else returns OK Result") {
            auto mapped = result.or_else<void>([](void) { return argus::err<int, void>(); });
            REQUIRE(mapped.is_ok());
            AND_THEN("or_else returns original value") {
                REQUIRE(mapped.unwrap() == 42);
            }
        }

        THEN("otherwise returns OK Result") {
            auto mapped = result.otherwise(argus::err<int, void>());
            REQUIRE(mapped.is_ok());
            AND_THEN("otherwise returns original value") {
                REQUIRE(mapped.unwrap() == 42);
            }
        }

        THEN("collate with OK Result returns OK Result") {
            auto collated = result.collate(argus::ok<int, void>(1337));
            REQUIRE(collated.is_ok());
            AND_THEN("unwrap returns correct value") {
                REQUIRE(collated.unwrap() == 1337);
            }
        }

        THEN("collate with error Result returns error Result") {
            auto collated = result.collate(argus::err<int, void>());
            REQUIRE(collated.is_err());
        }

        THEN("and_then returns OK Result") {
            auto combined = result.and_then<int>([](auto val) { return val + 1; });
            REQUIRE(combined.is_ok());
            AND_THEN("unwrap returns correct value") {
                REQUIRE(combined.unwrap() == 43);
            }
        }
    }

    GIVEN("An error Result<int, void>") {
        argus::Result<int, void> result = argus::err<int, void>();

        THEN("is_ok returns false") {
            REQUIRE(!result.is_ok());
        }

        THEN("is_err returns true") {
            REQUIRE(result.is_err());
        }

        THEN("expect triggers crash") {
            REQUIRE_THROWS(result.expect("expect failed"));
        }

        THEN("expect_err succeeds") {
            REQUIRE_NOTHROW(result.expect_err("expect_err failed"));
        }

        THEN("unwrap_or_default returns default value") {
            REQUIRE(result.unwrap_or_default(1337) == 1337);
        }

        THEN("map returns error Result") {
            auto mapped = result.map<int>([](auto err) { return err + 1; });
            REQUIRE(mapped.is_err());
        }

        THEN("map_or returns default value") {
            REQUIRE(result.map_or<int>(1337, [](auto val) { return val + 1; }) == 1337);
        }

        THEN("or_else returns fallback OK Result") {
            auto mapped = result.or_else<void>([](void) { return argus::ok<int, void>(1337); });
            REQUIRE(mapped.is_ok());
            AND_THEN("or_else returns correct fallback value") {
                REQUIRE(mapped.unwrap() == 1337);
            }
        }

        THEN("otherwise returns fallback OK Result") {
            auto mapped = result.otherwise(argus::ok<int, void>(1337));
            REQUIRE(mapped.is_ok());
            AND_THEN("otherwise returns correct fallback value") {
                REQUIRE(mapped.unwrap() == 1337);
            }
        }

        THEN("collate with OK Result returns error Result") {
            auto collated = result.collate(argus::ok<int, void>(1337));
            REQUIRE(collated.is_err());
        }

        THEN("collate with error Result returns error Result") {
            auto collated = result.collate(argus::err<int, void>());
            REQUIRE(collated.is_err());
        }

        THEN("and_then returns error Result") {
            auto combined = result.and_then<int>([](auto val) { return val + 1; });
            REQUIRE(combined.is_err());
        }
    }

    GIVEN("An OK Result<void, void>") {
        argus::Result<void, void> result = argus::ok<void, void>();

        THEN("is_ok returns true") {
            REQUIRE(result.is_ok());
        }

        THEN("is_err returns false") {
            REQUIRE(!result.is_err());
        }

        THEN("expect succeeds") {
            REQUIRE_NOTHROW(result.expect("expect failed"));
        }

        THEN("expect_err triggers crash") {
            REQUIRE_THROWS(result.expect_err("expect_err failed"));
        }

        THEN("or_else returns OK Result") {
            auto mapped = result.or_else<void>([](void) { return argus::err<void, void>(); });
            REQUIRE(mapped.is_ok());
        }

        THEN("otherwise returns OK Result") {
            auto mapped = result.otherwise(argus::err<void, void>());
            REQUIRE(mapped.is_ok());
        }

        THEN("collate with OK Result returns OK Result") {
            auto collated = result.collate(argus::ok<void, void>());
            REQUIRE(collated.is_ok());
        }

        THEN("collate with error Result returns error Result") {
            auto collated = result.collate(argus::err<void, void>());
            REQUIRE(collated.is_err());
        }

        THEN("and_then returns OK Result") {
            auto combined = result.and_then<int>([](void) { return 42; });
            REQUIRE(combined.is_ok());
            AND_THEN("unwrap returns correct value") {
                REQUIRE(combined.unwrap() == 42);
            }
        }
    }

    GIVEN("An error Result<void, void>") {
        argus::Result<void, void> result = argus::err<void, void>();

        THEN("is_ok returns false") {
            REQUIRE(!result.is_ok());
        }

        THEN("is_err returns true") {
            REQUIRE(result.is_err());
        }

        THEN("expect triggers crash") {
            REQUIRE_THROWS(result.expect("expect failed"));
        }

        THEN("expect_err succeeds") {
            REQUIRE_NOTHROW(result.expect_err("expect_err failed"));
        }

        THEN("or_else returns OK Result") {
            auto mapped = result.or_else<void>([](void) { return argus::ok<void, void>(); });
            REQUIRE(mapped.is_ok());
        }

        THEN("otherwise returns OK Result") {
            auto mapped = result.otherwise(argus::ok<void, void>());
            REQUIRE(mapped.is_ok());
        }

        THEN("collate with OK Result returns error Result") {
            auto collated = result.collate(argus::ok<void, void>());
            REQUIRE(collated.is_err());
        }

        THEN("collate with error Result returns error Result") {
            auto collated = result.collate(argus::err<void, void>());
            REQUIRE(collated.is_err());
        }

        THEN("and_then returns error Result") {
            auto combined = result.and_then<int>([](void) { return 42; });
            REQUIRE(combined.is_err());
        }
    }
}
