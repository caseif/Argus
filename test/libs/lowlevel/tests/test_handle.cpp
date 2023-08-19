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

#include "argus/lowlevel/handle.hpp"

TEST_CASE("Handle functions behave correctly", "[Handle]") {
    GIVEN("A pointer to a variable and a HandleTable") {
        int i = 42;
        argus::HandleTable table = argus::HandleTable();

        AND_GIVEN("A corresponding Handle") {
            argus::Handle handle = table.create_handle(i);

            WHEN("the handle is derefed generically") {
                void *derefed = table.deref(handle);

                THEN("the pointer is correct") {
                    CHECK(derefed == &i);
                }
            }

            WHEN("the handle is derefed to a typed pointer") {
                int *derefed = table.deref<int>(handle);

                THEN("the handle is derefed to the correct typed pointer") {
                    CHECK(derefed == &i);

                    AND_THEN("the pointed-to value is correct") {
                        CHECK(*derefed == 42);
                    }
                }
            }

            WHEN("the Handle is copied") {
                argus::Handle handle_copy = handle;

                THEN("the copy passes an equality test against the original") {
                    CHECK(handle_copy == handle);
                }

                AND_WHEN("the handle is derefed to a typed pointer") {
                    int *derefed = table.deref<int>(handle_copy);

                    THEN("the pointer is correct") {
                        CHECK(derefed == &i);
                    }
                }
            }

            WHEN("the Handle is updated") {
                int j = 43;
                table.update_handle(handle, &j);

                AND_WHEN("the Handle is derefed") {
                    int *val = table.deref<int>(handle);

                    THEN("the pointed-to value is correct") {
                        CHECK(*val == 43);
                    }
                }
            }

            WHEN("the Handle is released") {
                table.release_handle(handle);

                AND_WHEN("the Handle is derefed") {
                    void *val = table.deref(handle);

                    THEN("the returned value is a null pointer") {
                        CHECK(val == nullptr);
                    }
                }
            }

            WHEN("the Handle's uid is tampered with") {
                handle.uid += 1;

                AND_WHEN("the Handle is derefed") {
                    void *ptr = table.deref(handle);

                    THEN("the returned value is a null pointer") {
                        CHECK(ptr == nullptr);
                    }
                }
            }

            WHEN("a second Handle is created") {
                argus::Handle handle_2 = table.create_handle(&i);

                AND_WHEN("the first Handle's index is set to the second's") {
                    handle.index = handle_2.index;

                    AND_WHEN("the first Handle is derefed") {
                        void *ptr = table.deref(handle);

                        THEN("the returned value is a null pointer") {
                            CHECK(ptr == nullptr);
                        }
                    }
                }
            }
        }
    }
}
