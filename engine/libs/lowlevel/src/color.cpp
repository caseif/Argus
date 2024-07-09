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

#include "argus/lowlevel/color.hpp"
#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/math/vector.hpp"

#include <algorithm>

#include <cmath>

namespace argus {
    Vector3f rgb_to_hsv(const Vector3f &rgb) {
        float r = std::clamp(rgb.r, 0.0f, 1.0f);
        float g = std::clamp(rgb.g, 0.0f, 1.0f);
        float b = std::clamp(rgb.b, 0.0f, 1.0f);

        double cmax = std::max(r, std::max(g, b));
        double cmin = std::min(r, std::min(g, b));
        double diff = cmax - cmin;
        double h;
        double s;

        if (cmax == cmin) {
            h = 0;
        } else if (cmax == r) {
            h = std::fmod(60 * ((g - b) / diff) + 360, 360);
        } else if (cmax == g) {
            h = std::fmod(60 * ((b - r) / diff) + 120, 360);
        } else {
            h = std::fmod(60 * ((r - g) / diff) + 240, 360);
        }

        if (cmax == 0) {
            s = 0;
        } else {
            s = diff / cmax;
        }

        double v = cmax;

        return { float(h), float(s), float(v) };
    }

    Vector3f hsv_to_rgb(const Vector3f &hsv) {
        float h = hsv.x;
        float s = std::clamp(hsv.y, 0.0f, 1.0f);
        float v = std::clamp(hsv.z, 0.0f, 1.0f);

        double max = v;
        double c = s * v;
        double min = max - c;

        double hprime;
        if (h >= 300) {
            hprime = (h - 360.0) / 60.0;
        } else {
            hprime = h / 60.0;
        }

        double r;
        double g;
        double b;

        argus_assert_ll(hprime >= -1);
        if (hprime < 0) {
            r = max;
            g = min;
            b = g - hprime * c;
        } else if (hprime < 1) {
            r = max;
            b = min;
            g = b + hprime * c;
        } else if (hprime < 2) {
            g = max;
            b = min;
            r = b - (hprime - 2) * c;
        } else if (hprime < 3) {
            r = min;
            g = max;
            b = r + (hprime - 2) * c;
        } else if (hprime < 4) {
            r = min;
            b = max;
            g = r - (hprime - 4) * c;
        } else {
            g = min;
            b = max;
            r = g + (hprime - 4) * c;
        }

        return { float(r), float(g), float(b) };
    }

    Vector3f normalize_rgb(const Vector3f &rgb) {
        auto hsv_color = rgb_to_hsv(rgb);
        hsv_color.z = 1.0; // set value to max
        auto final_rgb = hsv_to_rgb(hsv_color);
        return final_rgb;
    }
}
