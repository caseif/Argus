/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

#include "test_global.hpp"

#include "argus/lowlevel/dirtiable.hpp"

SCENARIO("Dirty flag and value is set correctly", "[Dirtiable]") {
    GIVEN("A new Dirtiable int") {
        argus::Dirtiable<int> dirtiable(42);

        THEN("the initial value is correct") {
            REQUIRE(dirtiable.read().value == 42);
        }

        THEN("the conversion overload works") {
            CHECK(dirtiable.read() == 42);
        }

        THEN("the dirty flag is clear") {
            REQUIRE(!dirtiable.read().dirty);
        }

        WHEN("an lvalue is directly assigned") {
            int new_val = 43;
            dirtiable = new_val;

            AND_WHEN("the value is read") {
                auto val_and_dirty = dirtiable.read();

                THEN("the value is updated") {
                    CHECK(val_and_dirty.value == new_val);
                }

                THEN("the dirty flag is set") {
                    CHECK(val_and_dirty.dirty);
                }

                AND_WHEN("the value is read again") {
                    val_and_dirty = dirtiable.read();

                    THEN("the dirty flag is clear") {
                        CHECK(!val_and_dirty.dirty);
                    }
                }
            }

            AND_WHEN("the value is read to a const lvalue") {
                const argus::Dirtiable<int> &dirtiable_const_ref = dirtiable;
                auto const_val_and_dirty = dirtiable_const_ref.read();

                THEN("the read value is correct") {
                    CHECK(const_val_and_dirty.value == 43);
                }

                THEN("the dirty flag is set") {
                    CHECK(const_val_and_dirty.dirty);
                }
            }

            AND_WHEN("the value is peeked") {
                auto val = dirtiable.peek();

                THEN("the value is updated") {
                    CHECK(val == 43);
                }

                AND_WHEN("the value is read") {
                    auto val_and_dirty = dirtiable.read();

                    THEN("the dirty flag is set") {
                        CHECK(val_and_dirty.dirty);
                    }
                }
            }
        }

        WHEN("an rvalue is directly assigned") {
            dirtiable = 43;

            AND_WHEN("the value is read") {
                auto val_and_dirty = dirtiable.read();

                THEN("the value is updated") {
                    CHECK(val_and_dirty.value == 43);
                }

                THEN("the dirty flag is set") {
                    CHECK(val_and_dirty.dirty);
                }
            }
        }

        WHEN("the addition assignment operator is used") {
            dirtiable += 1;

            THEN("the value is updated") {
                CHECK(dirtiable.read().value == 42 + 1);
            }

            THEN("the dirty flag is set") {
                CHECK(dirtiable.read().dirty);
            }
        }

        WHEN("the subtraction assignment operator is used") {
            dirtiable -= 1;

            THEN("the value is updated") {
                CHECK(dirtiable.read().value == 42 - 1);
            }

            THEN("the dirty flag is set") {
                CHECK(dirtiable.read().dirty);
            }
        }

        WHEN("the multiplication assignment operator is used") {
            dirtiable *= 2;

            THEN("the value is updated") {
                CHECK(dirtiable.read().value == 42 * 2);
            }

            THEN("the dirty flag is set") {
                CHECK(dirtiable.read().dirty);
            }
        }

        WHEN("the addition assignment operator is used") {
            dirtiable /= 2;

            THEN("the value is updated") {
                CHECK(dirtiable.read().value == 42 / 2);
            }

            THEN("the dirty flag is set") {
                CHECK(dirtiable.read().dirty);
            }
        }

        WHEN("it is set quietly with an lvalue") {
            int new_val = 43;
            dirtiable.set_quietly(new_val);

            THEN("the value is updated") {
                CHECK(dirtiable.read().value == 43);
            }

            THEN("the dirty flag is not set") {
                CHECK(!dirtiable.read().dirty);
            }
        }

        WHEN("it is set quietly with an rvalue") {
            dirtiable.set_quietly(43);

            THEN("the value is updated") {
                CHECK(dirtiable.read().value == 43);
            }

            THEN("the dirty flag is not set") {
                CHECK(!dirtiable.read().dirty);
            }
        }

        AND_GIVEN("another new Dirtiable int") {
            argus::Dirtiable<int> new_dirtiable(43);

            WHEN("it is assigned to the original") {
                dirtiable = new_dirtiable;

                THEN("the original's value is updated") {
                    CHECK(dirtiable.read().value == 43);
                }

                THEN("the original's dirty flag is clear") {
                    CHECK(!dirtiable.read().dirty);
                }
            }

            WHEN("its value is changed") {
                new_dirtiable = 44;

                AND_WHEN("it is assigned to the original") {
                    dirtiable = new_dirtiable;

                    THEN("the original's value is updated") {
                        CHECK(dirtiable.read().value == 44);
                    }

                    THEN("the original's dirty flag is set") {
                        CHECK(dirtiable.read().dirty);
                    }
                }
            }
        }
    }

    GIVEN("A new Dirtiable string") {
        argus::Dirtiable<std::string> dirtiable("foo");

        WHEN("it is read") {
            auto val_and_dirty = dirtiable.read();

            AND_WHEN("the read value is coerced to a string") {
                std::string str = val_and_dirty;

                THEN("the value is correct") {
                    CHECK(str == "foo");
                }
            }

            AND_WHEN("the read value is coerced to a const string reference") {
                const std::string &str_ref = val_and_dirty;

                THEN("the value is correct") {
                    CHECK(str_ref == "foo");
                }
            }

            THEN("the arrow operator works") {
                CHECK(val_and_dirty->length() == 3);
            }
        }
    }

    GIVEN("A new Dirtiable string pointer") {
        std::string str = "foo";
        argus::Dirtiable<std::string *> dirtiable(&str);

        WHEN("it is read") {
            auto val_and_dirty = dirtiable.read();

            THEN("the arrow operator works") {
                CHECK(val_and_dirty->length() == 3);
            }
        }
    }
}
