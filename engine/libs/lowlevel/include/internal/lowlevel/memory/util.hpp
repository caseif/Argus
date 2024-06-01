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

#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace argus {
    // helper function for determining the nearest aligned address preceding the given unaligned address
    inline uintptr_t prev_aligned_value(uintptr_t base_val, size_t alignment_exp) {
        // special case if no alignment is requested
        if (alignment_exp == 0) {
            return base_val;
        }

        size_t alignment_bytes = size_t(exp2(double(alignment_exp)));
        // We create a bitmask from the alignment "chunk" size by subtracting one (e.g. 0x0010 -> 0x000F)
        // and then inverting it (0x000F -> 0xFFF0). Then, we AND it with the base address minus 1 to get
        // the next aligned address in the direction of zero.
        return (base_val & ~(alignment_bytes - 1U));
    }

    // helper function for determining the nearest aligned address following the given unaligned address
    inline uintptr_t next_aligned_value(uintptr_t base_val, size_t alignment_exp) {
        // special case if no alignment is requested
        if (alignment_exp == 0) {
            return base_val;
        }

        size_t alignment_bytes = size_t(exp2(double(alignment_exp)));
        // We create a bitmask from the alignment "chunk" size by subtracting one (e.g. 0x0010 -> 0x000F)
        // and then inverting it (0x000F -> 0xFFF0). Then, we AND it with the base address minus 1 to get
        // the next aligned address in the direction of zero, then add the alignment "chunk" size to get
        // the next aligned address in the direction of max size_t.
        // Subtracting one from the base address accounts for the case where the address is already aligned.
        return ((base_val - 1) & ~(alignment_bytes - 1U)) + alignment_bytes;
    }
}
