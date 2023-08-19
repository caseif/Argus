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
#include "argus/lowlevel/math/matrix.hpp"
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

TEST_CASE("Matrix operations behave correctly", "[math][Matrix]") {
    GIVEN("A default-constructed Matrix4") {
        argus::Matrix4 mat = argus::Matrix4();

        THEN("all elements are initialized to 0") {
            auto r = GENERATE(range(0, 4));
            auto c = GENERATE(range(0, 4));

            CHECK(mat(r, c) == 0.);
        }

        WHEN("an element is assigned") {
            mat(3, 1) = 42.;

            THEN("the element is updated correctly") {
                CHECK(mat(3, 1) == 42.);
            }

            THEN("the inverse element is not updated") {
                CHECK(mat(1, 3) == 0.);
            }
        }
    }

    GIVEN("An identity Matrix4") {
        argus::Matrix4 mat = argus::Matrix4::identity();

        THEN("all elements are initialized correctly") {
            auto r = GENERATE(range(0, 4));
            auto c = GENERATE(range(0, 4));

            CHECK(mat(r, c) == (r == c ? 1. : 0.));
        }
    }

    GIVEN("A Matrix4 constructed from a column-major C array") {
        float arr[] = {
                1., 5., 9.,  13.,
                2., 6., 10., 14.,
                3., 7., 11., 15.,
                4., 8., 12., 16.,
        };

        argus::Matrix4 mat = argus::Matrix4(arr);

        THEN("all elements are initialized correctly") {
            auto r = GENERATE(range(0, 4));
            auto c = GENERATE(range(0, 4));

            CHECK(mat(r, c) == r * 4. + c + 1);
        }
    }

    GIVEN("A Matrix4 constructed from an std::array") {
        std::array<float, 16> arr = {
                1., 5., 9.,  13.,
                2., 6., 10., 14.,
                3., 7., 11., 15.,
                4., 8., 12., 16.,
        };

        argus::Matrix4 mat = argus::Matrix4(arr);

        THEN("all elements are initialized correctly") {
            auto r = GENERATE(range(0, 4));
            auto c = GENERATE(range(0, 4));

            REQUIRE(mat(r, c) == r * 4. + c + 1);
        }

        WHEN("it is transposed") {
            mat.transpose();

            THEN("all elements are updated correctly") {
                auto r = GENERATE(range(0, 4));
                auto c = GENERATE(range(0, 4));

                CHECK(mat(c, r) == r * 4. + c + 1);
            }
        }
    }

    GIVEN("A Matrix4 constructed from a row-major C array") {
        float arr[] = {
                1.,  2.,  3.,  4.,
                5.,  6.,  7.,  8.,
                9.,  10., 11., 12.,
                13., 14., 15., 16.,
        };

        argus::Matrix4 mat = argus::Matrix4::from_row_major(arr);

        THEN("all elements are initialized correctly") {
            auto r = GENERATE(range(0, 4));
            auto c = GENERATE(range(0, 4));

            CHECK(mat(r, c) == r * 4. + c + 1);
        }
    }

    GIVEN("A Matrix4 constructed from a row-major std::array") {
        argus::Matrix4 mat = argus::Matrix4::from_row_major({
                1.,  2.,  3.,  4.,
                5.,  6.,  7.,  8.,
                9.,  10., 11., 12.,
                13., 14., 15., 16.,
        });

        THEN("all elements are initialized correctly") {
            auto r = GENERATE(range(0, 4));
            auto c = GENERATE(range(0, 4));

            CHECK(mat(r, c) == r * 4. + c + 1);
        }
    }

    GIVEN("Two new Matrix4 objects") {
        argus::Matrix4 mat_a = argus::Matrix4({
                1., 5., 9.,  13.,
                2., 6., 10., 14.,
                3., 7., 11., 15.,
                4., 8., 12., 16.,
        });
        argus::Matrix4 mat_b = argus::Matrix4({
                16., 12., 8., 4.,
                15., 11., 7., 3.,
                14., 10., 6., 2.,
                13., 9.,  5., 1.,
        });

        WHEN("they are multiplied to a new Matrix4") {
            argus::Matrix4 product = mat_a * mat_b;

            THEN("the product matrix's elements are correct") {
                argus::Matrix4 expected({
                        80., 240., 400., 560.,
                        70., 214., 358., 502.,
                        60., 188., 316., 444.,
                        50., 162., 274., 386.
                });

                auto r = GENERATE(range(0, 4));
                auto c = GENERATE(range(0, 4));

                CHECK(product(r, c) == expected(r, c));
            }
        }
    }
}

TEST_CASE("Matrix-vector operations behave correctly", "[math][MatrixVectorOps]") {
    GIVEN("A matrix and a Vector4f") {
        argus::Matrix4 mat = argus::Matrix4({
                -0.5, 2.5, -4.5, 6.5,
                -1.0, 3.0, -5.0, 7.0,
                -1.5, 3.5, -5.5, 7.5,
                -2.0, 4.0, -6.0, 8.0
        });

        argus::Vector4f vec = argus::Vector4f(0.24f, 0.42f, 1.24f, 1.42f);

        WHEN("they are multiplied together") {
            argus::Vector4f res_vec = mat * vec;

            THEN("the resulting vector is correct") {
                CHECK_THAT(res_vec.x, Catch::Matchers::WithinRel(-5.24f));
                CHECK_THAT(res_vec.y, Catch::Matchers::WithinRel(11.88f));
                CHECK_THAT(res_vec.z, Catch::Matchers::WithinRel(-18.52f));
                CHECK_THAT(res_vec.w, Catch::Matchers::WithinRel(25.16f));
            }
        }
    }
}
