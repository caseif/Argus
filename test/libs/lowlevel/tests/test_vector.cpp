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

#include "argus/lowlevel/extra_type_traits.hpp"
#include "argus/lowlevel/math/vector.hpp"

#include "catch2/catch_all.hpp"

template <typename T>
static constexpr unsigned int _get_vector_size(void) {
    if constexpr (argus::is_specialization_v<T, argus::Vector2>) {
        return 2;
    } else if constexpr (argus::is_specialization_v<T, argus::Vector3>) {
        return 3;
    } else if constexpr (argus::is_specialization_v<T, argus::Vector4>) {
        return 4;
    }
}

TEMPLATE_TEST_CASE("Vector operations behave correctly", "[math][Vector]",
        argus::Vector2d, argus::Vector2f, argus::Vector2i, argus::Vector2u,
        argus::Vector3d, argus::Vector3f, argus::Vector3i, argus::Vector3u,
        argus::Vector4d, argus::Vector4f, argus::Vector4i, argus::Vector4u) {
    using ElementType = typename TestType::element_type;

    constexpr ElementType max_val = ElementType(100);
    constexpr ElementType min_val = ElementType(std::is_unsigned_v<ElementType> ? 0 : -100);

    constexpr unsigned int N = 4;

    GIVEN("A new default-contructed Vector object") {
        TestType vec = TestType();

        THEN("its values are initialized as 0") {
            CHECK(vec.x == ElementType(0));
            CHECK(vec.y == ElementType(0));
            if constexpr (_get_vector_size<TestType>() >= 3) {
                CHECK(vec.z == ElementType(0));
            }
            if constexpr (_get_vector_size<TestType>() >= 4) {
                CHECK(vec.w == ElementType(0));
            }
        }
    }

    GIVEN("A new Vector object") {
        auto vals = GENERATE_COPY(take(10, chunk(N, random<ElementType>(min_val, max_val))));
        auto x = vals.at(0);
        auto y = vals.at(1);
        auto z = vals.at(2);
        auto w = vals.at(3);
        REQUIRE(x >= min_val);
        REQUIRE(x <= max_val);
        REQUIRE(y >= min_val);
        REQUIRE(y <= max_val);
        REQUIRE(z >= min_val);
        REQUIRE(z <= max_val);
        REQUIRE(w >= min_val);
        REQUIRE(w <= max_val);
        TestType vec;
        if constexpr (argus::is_specialization_v<TestType, argus::Vector2>) {
            vec = TestType(x, y);
        } else if constexpr (argus::is_specialization_v<TestType, argus::Vector3>) {
            vec = TestType(x, y, z);
        } else if constexpr (argus::is_specialization_v<TestType, argus::Vector4>) {
            vec = TestType(x, y, z, w);
        }

        WHEN("it is multiplied by a constant") {
            auto res = vec * 2;

            THEN("the product vector is correct") {
                CHECK(res.x == vec.x * 2);
                CHECK(res.y == vec.y * 2);
                if constexpr (_get_vector_size<TestType>() >= 3) {
                    CHECK(res.z == vec.z * 2);
                }
                if constexpr (_get_vector_size<TestType>() >= 4) {
                    CHECK(res.w == vec.w * 2);
                }
            }
        }

        WHEN("it is divided by a constant") {
            auto res = vec / 2;

            THEN("the quotient vector is correct") {
                CHECK(res.x == vec.x / 2);
                CHECK(res.y == vec.y / 2);
                if constexpr (_get_vector_size<TestType>() >= 3) {
                    CHECK(res.z == vec.z / 2);
                }
                if constexpr (_get_vector_size<TestType>() >= 4) {
                    CHECK(res.w == vec.w / 2);
                }
            }
        }

        WHEN("it is multiply-assigned with a constant") {
            vec *= 2;

            THEN("the product vector is correct") {
                CHECK(vec.x == x * 2);
                CHECK(vec.y == y * 2);
                if constexpr (_get_vector_size<TestType>() >= 3) {
                    CHECK(vec.z == z * 2);
                }
                if constexpr (_get_vector_size<TestType>() >= 4) {
                    CHECK(vec.w == w * 2);
                }
            }
        }

        WHEN("it is divide-assigned with a constant") {
            vec /= 2;

            THEN("the quotient vector is correct") {
                CHECK(vec.x == x / 2);
                CHECK(vec.y == y / 2);
                if constexpr (_get_vector_size<TestType>() >= 3) {
                    CHECK(vec.z == z / 2);
                }
                if constexpr (_get_vector_size<TestType>() >= 4) {
                    CHECK(vec.w == w / 2);
                }
            }
        }

        if constexpr (std::is_signed_v<ElementType>) {
            WHEN("it is inverted") {
                auto inv = vec.inverse();

                THEN("the values are negated") {
                    CHECK(inv.x == -vec.x);
                    CHECK(inv.y == -vec.y);
                    if constexpr (_get_vector_size<TestType>() >= 3) {
                        CHECK(inv.z == -vec.z);
                    }
                    if constexpr (_get_vector_size<TestType>() >= 4) {
                        CHECK(inv.w == -vec.w);
                    }
                }
            }
        }

        if constexpr (_get_vector_size<TestType>() < 3) {
            WHEN("it is converted to a Vector3") {
                argus::Vector3<ElementType> vec3(vec);

                THEN("the values are copied") {
                    CHECK(vec3.x == vec.x);
                    CHECK(vec3.y == vec.y);
                }

                THEN("the remaining values are initialized to 0") {
                    CHECK(vec3.z == ElementType(0));
                }
            }
        }

        if constexpr (_get_vector_size<TestType>() < 4) {
            WHEN("it is converted to a Vector3") {
                argus::Vector4<ElementType> vec4(vec);

                THEN("the values are copied") {
                    CHECK(vec4.x == vec.x);
                    CHECK(vec4.y == vec.y);
                    if constexpr (_get_vector_size<TestType>() >= 3) {
                        CHECK(vec4.z == vec.z);
                    }
                }

                THEN("the remaining values are initialized to 0") {
                    if constexpr (_get_vector_size<TestType>() < 3) {
                        CHECK(vec4.z == ElementType(0));
                    }
                    CHECK(vec4.w == ElementType(0));
                }
            }
        }
    }

    GIVEN("Two new Vector objects") {
        auto vals = GENERATE_COPY(take(10, chunk(N * 2, random<ElementType>(min_val, max_val))));
        auto x_a = vals.at(0);
        auto y_a = vals.at(1);
        auto z_a = vals.at(2);
        auto w_a = vals.at(3);
        auto x_b = vals.at(4);
        auto y_b = vals.at(5);
        auto z_b = vals.at(6);
        auto w_b = vals.at(7);
        TestType vec_a;
        TestType vec_b;
        if constexpr (argus::is_specialization_v<TestType, argus::Vector2>) {
            vec_a = TestType(x_a, y_a);
            vec_b = TestType(x_b, y_b);
        } else if constexpr (argus::is_specialization_v<TestType, argus::Vector3>) {
            vec_a = TestType(x_a, y_a, z_a);
            vec_b = TestType(x_b, y_b, z_b);
        } else if constexpr (argus::is_specialization_v<TestType, argus::Vector4>) {
            vec_a = TestType(x_a, y_a, z_a, w_a);
            vec_b = TestType(x_b, y_b, z_b, w_b);
        }

        WHEN("they are added") {
            auto res = vec_a + vec_b;

            THEN("the sum vector is correct") {
                CHECK(res.x == vec_a.x + vec_b.x);
                CHECK(res.y == vec_a.y + vec_b.y);
                if constexpr (_get_vector_size<TestType>() >= 3) {
                    CHECK(res.z == vec_a.z + vec_b.z);
                }
                if constexpr (_get_vector_size<TestType>() >= 4) {
                    CHECK(res.w == vec_a.w + vec_b.w);
                }
            }
        }

        WHEN("the second is subtracted from the first") {
            auto res = vec_a - vec_b;

            THEN("the difference vector is correct") {
                CHECK(res.x == vec_a.x - vec_b.x);
                CHECK(res.y == vec_a.y - vec_b.y);
                if constexpr (_get_vector_size<TestType>() >= 3) {
                    CHECK(res.z == vec_a.z - vec_b.z);
                }
                if constexpr (_get_vector_size<TestType>() >= 4) {
                    CHECK(res.w == vec_a.w - vec_b.w);
                }
            }
        }

        WHEN("they are multipled") {
            auto res = vec_a * vec_b;

            THEN("the product vector is correct") {
                CHECK(res.x == vec_a.x * vec_b.x);
                CHECK(res.y == vec_a.y * vec_b.y);
                if constexpr (_get_vector_size<TestType>() >= 3) {
                    CHECK(res.z == vec_a.z * vec_b.z);
                }
                if constexpr (_get_vector_size<TestType>() >= 4) {
                    CHECK(res.w == vec_a.w * vec_b.w);
                }
            }
        }

        WHEN("the first is add-assigned with the second") {
            vec_a += vec_b;

            THEN("the sum vector is correct") {
                CHECK(vec_a.x == x_a + x_b);
                CHECK(vec_a.y == y_a + y_b);
                if constexpr (_get_vector_size<TestType>() >= 3) {
                    CHECK(vec_a.z == z_a + z_b);
                }
                if constexpr (_get_vector_size<TestType>() >= 4) {
                    CHECK(vec_a.w == w_a + w_b);
                }

                AND_THEN("the second vector is unchanged") {
                    CHECK(vec_b.x == x_b);
                    CHECK(vec_b.y == y_b);
                    if constexpr (_get_vector_size<TestType>() >= 3) {
                        CHECK(vec_b.z == z_b);
                    }
                    if constexpr (_get_vector_size<TestType>() >= 4) {
                        CHECK(vec_b.w == w_b);
                    }
                }
            }
        }

        WHEN("the first is subtract-assigned with the second") {
            auto orig_x_a = vec_a.x;
            auto orig_y_a = vec_a.y;
            auto orig_x_b = vec_b.x;
            auto orig_y_b = vec_b.y;

            vec_a -= vec_b;

            THEN("the difference vector is correct") {
                CHECK(vec_a.x == orig_x_a - orig_x_b);
                CHECK(vec_a.y == orig_y_a - orig_y_b);
                if constexpr (_get_vector_size<TestType>() >= 3) {
                    CHECK(vec_a.z == z_a - z_b);
                }
                if constexpr (_get_vector_size<TestType>() >= 4) {
                    CHECK(vec_a.w == w_a - w_b);
                }

                AND_THEN("the second vector is unchanged") {
                    CHECK(vec_b.x == orig_x_b);
                    CHECK(vec_b.y == orig_y_b);
                    if constexpr (_get_vector_size<TestType>() >= 3) {
                        CHECK(vec_b.z == z_b);
                    }
                    if constexpr (_get_vector_size<TestType>() >= 4) {
                        CHECK(vec_b.w == w_b);
                    }
                }
            }
        }

        WHEN("the first is multiply-assigned with the second") {
            auto orig_x_a = vec_a.x;
            auto orig_y_a = vec_a.y;
            auto orig_x_b = vec_b.x;
            auto orig_y_b = vec_b.y;

            vec_a *= vec_b;

            THEN("the product vector is correct") {
                CHECK(vec_a.x == orig_x_a * orig_x_b);
                CHECK(vec_a.y == orig_y_a * orig_y_b);
                if constexpr (_get_vector_size<TestType>() >= 3) {
                    CHECK(vec_a.z == z_a * z_b);
                }
                if constexpr (_get_vector_size<TestType>() >= 4) {
                    CHECK(vec_a.w == w_a * w_b);
                }

                AND_THEN("the second vector is unchanged") {
                    CHECK(vec_b.x == orig_x_b);
                    CHECK(vec_b.y == orig_y_b);
                    if constexpr (_get_vector_size<TestType>() >= 3) {
                        CHECK(vec_b.z == z_b);
                    }
                    if constexpr (_get_vector_size<TestType>() >= 4) {
                        CHECK(vec_b.w == w_b);
                    }
                }
            }
        }
    }
}
