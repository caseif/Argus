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

#include "argus/lowlevel/math.hpp"

#include "catch2/catch_all.hpp"

TEMPLATE_TEST_CASE("Vector2 operations behave correctly", "[Vector2]",
        int32_t, uint32_t, float, double) {
    constexpr TestType max_val = TestType(100);
    constexpr TestType min_val = TestType(std::is_unsigned_v<TestType> ? 0 : -100);

    constexpr unsigned int N = 2;

    GIVEN("A new default-contructed Vector2 object") {
        argus::Vector2<TestType> vec = argus::Vector2<TestType>();

        THEN("its values are initialized as 0") {
            CHECK(vec.x == TestType(0));
            CHECK(vec.y == TestType(0));
        }
    }

    GIVEN("A new Vector2 object") {
        auto vals = GENERATE_COPY(take(10, chunk(N, random<TestType>(min_val, max_val))));
        auto x = vals.at(0);
        auto y = vals.at(1);
        REQUIRE(x >= min_val);
        REQUIRE(x <= max_val);
        REQUIRE(y >= min_val);
        REQUIRE(y <= max_val);
        argus::Vector2<TestType> vec(x, y);

        WHEN("it is multiplied by a constant") {
            auto res = vec * 2;

            THEN("the product vector is correct") {
                CHECK(res.x == vec.x * 2);
                CHECK(res.y == vec.y * 2);
            }
        }

        WHEN("it is divided by a constant") {
            auto res = vec / 2;

            THEN("the quotient vector is correct") {
                CHECK(res.x == vec.x / 2);
                CHECK(res.y == vec.y / 2);
            }
        }

        WHEN("it is multiply-assigned with a constant") {
            TestType orig_x = vec.x;
            TestType orig_y = vec.y;
            vec *= 2;

            THEN("the product vector is correct") {
                CHECK(vec.x == orig_x * 2);
                CHECK(vec.y == orig_y * 2);
            }
        }

        WHEN("it is divide-assigned with a constant") {
            TestType orig_x = vec.x;
            TestType orig_y = vec.y;
            vec /= 2;

            THEN("the quotient vector is correct") {
                CHECK(vec.x == orig_x / 2);
                CHECK(vec.y == orig_y / 2);
            }
        }

        WHEN("it is inverted") {
            auto inv = vec.inverse();

            THEN("the values are negated") {
                CHECK(inv.x == -vec.x);
                CHECK(inv.y == -vec.y);
            }
        }

        WHEN("it is converted to a Vector3") {
            argus::Vector3<TestType> vec3(vec);

            THEN("the values are copied") {
                CHECK(vec3.x == vec.x);
                CHECK(vec3.y == vec.y);
            }

            THEN("the remaining values are initialized to 0") {
                CHECK(vec3.z == TestType(0));
            }
        }

        WHEN("it is converted to a Vector3") {
            argus::Vector4<TestType> vec4(vec);

            THEN("the values are copied") {
                CHECK(vec4.x == vec.x);
                CHECK(vec4.y == vec.y);
            }

            THEN("the remaining values are initialized to 0") {
                CHECK(vec4.z == TestType(0));
                CHECK(vec4.w == TestType(0));
            }
        }
    }

    GIVEN("Two new Vector2 objects") {
        auto vals = GENERATE_COPY(take(10, chunk(N, random<TestType>(min_val, max_val))));
        auto x_a = vals.at(0);
        auto y_a = vals.at(1);
        auto x_b = vals.at(0);
        auto y_b = vals.at(1);
        argus::Vector2<TestType> vec_a(x_a, y_a);
        argus::Vector2<TestType> vec_b(x_b, y_b);

        WHEN("they are added") {
            if constexpr (std::is_same_v<TestType, float>) {
                if (std::isnan(vec_a.x + vec_b.x)) {
                    CHECK(vec_a.x == 0.0f);
                    CHECK(vec_b.x == 0.0f);
                } else if (std::isnan(vec_a.y + vec_b.y)) {
                    CHECK(vec_a.y == 0.0f);
                    CHECK(vec_b.y == 0.0f);
                }
            }
            auto res = vec_a + vec_b;

            THEN("the sum vector is correct") {
                CHECK(res.x == vec_a.x + vec_b.x);
                CHECK(res.y == vec_a.y + vec_b.y);
            }
        }

        WHEN("the second is subtracted from the first") {
            auto res = vec_a - vec_b;

            THEN("the difference vector is correct") {
                CHECK(res.x == vec_a.x - vec_b.x);
                CHECK(res.y == vec_a.y - vec_b.y);
            }
        }

        WHEN("they are multipled") {
            auto res = vec_a * vec_b;

            THEN("the product vector is correct") {
                CHECK(res.x == vec_a.x * vec_b.x);
                CHECK(res.y == vec_a.y * vec_b.y);
            }
        }

        WHEN("the first is add-assigned with the second") {
            TestType orig_x_a = vec_a.x;
            TestType orig_y_a = vec_a.y;
            TestType orig_x_b = vec_b.x;
            TestType orig_y_b = vec_b.y;

            vec_a += vec_b;

            THEN("the sum vector is correct") {
                CHECK(vec_a.x == orig_x_a + orig_x_b);
                CHECK(vec_a.y == orig_y_a + orig_y_b);

                AND_THEN("the second vector is unchanged") {
                    CHECK(vec_b.x == orig_x_b);
                    CHECK(vec_b.y == orig_y_b);
                }
            }
        }

        WHEN("the first is subtract-assigned with the second") {
            TestType orig_x_a = vec_a.x;
            TestType orig_y_a = vec_a.y;
            TestType orig_x_b = vec_b.x;
            TestType orig_y_b = vec_b.y;

            vec_a -= vec_b;

            THEN("the difference vector is correct") {
                CHECK(vec_a.x == orig_x_a - orig_x_b);
                CHECK(vec_a.y == orig_y_a - orig_y_b);

                AND_THEN("the second vector is unchanged") {
                    CHECK(vec_b.x == orig_x_b);
                    CHECK(vec_b.y == orig_y_b);
                }
            }
        }

        WHEN("the first is multiply-assigned with the second") {
            TestType orig_x_a = vec_a.x;
            TestType orig_y_a = vec_a.y;
            TestType orig_x_b = vec_b.x;
            TestType orig_y_b = vec_b.y;

            vec_a *= vec_b;

            THEN("the product vector is correct") {
                CHECK(vec_a.x == orig_x_a * orig_x_b);
                CHECK(vec_a.y == orig_y_a * orig_y_b);

                AND_THEN("the second vector is unchanged") {
                    CHECK(vec_b.x == orig_x_b);
                    CHECK(vec_b.y == orig_y_b);
                }
            }
        }
    }
}
