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

#pragma once

#define CATCH_CONFIG_MAIN
#include "catch2/catch_all.hpp"

template <typename T, unsigned int N>
  class RandomArrayGenerator : public Catch::Generators::IGenerator<std::array<T, N>> {
    using ArrayType = std::array<T, N>;

  private:
    Catch::Generators::GeneratorWrapper<T> gen;
    ArrayType cur;

  public:
    RandomArrayGenerator(T min, T max) :
        gen(Catch::Generators::random(min, max)) {
    }

    const ArrayType &get() const override {
        return cur;
    }

    bool next() override {
        std::generate(cur.begin(), cur.end(), [&]() { return gen.get(); });
        return true;
    }
};

template <typename T, unsigned int N>
Catch::Generators::GeneratorWrapper<std::array<T, N>> random_array(T a, T b) {
    return Catch::Generators::GeneratorWrapper<std::array<T, N>>(
            Catch::Detail::make_unique<RandomArrayGenerator<T, N>>(a, b)
    );
}
