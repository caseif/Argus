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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct argus_vector_2d_t {
    double x;
    double y;
} argus_vector_2d_t;

typedef struct argus_vector_3d_t {
    union {
        double x;
        double r;
    };
    union {
        double y;
        double g;
    };
    union {
        double z;
        double b;
    };
} argus_vector_3d_t;

typedef struct argus_vector_4d_t {
    union {
        double x;
        double r;
    };
    union {
        double y;
        double g;
    };
    union {
        double z;
        double b;
    };
    union {
        double w;
        double a;
    };
} argus_vector_4d_t;

typedef struct argus_vector_2f_t {
    float x;
    float y;
} argus_vector_2f_t;

typedef struct argus_vector_3f_t {
    union {
        float x;
        float r;
    };
    union {
        float y;
        float g;
    };
    union {
        float z;
        float b;
    };
} argus_vector_3f_t;

typedef struct argus_vector_4f_t {
    union {
        float x;
        float r;
    };
    union {
        float y;
        float g;
    };
    union {
        float z;
        float b;
    };
    union {
        float w;
        float a;
    };
} argus_vector_4f_t;

typedef struct argus_vector_2i_t {
    int32_t x;
    int32_t y;
} argus_vector_2i_t;

typedef struct argus_vector_3i_t {
    union {
        int32_t x;
        int32_t r;
    };
    union {
        int32_t y;
        int32_t g;
    };
    union {
        int32_t z;
        int32_t b;
    };
} argus_vector_3i_t;

typedef struct argus_vector_4i_t {
    union {
        int32_t x;
        int32_t r;
    };
    union {
        int32_t y;
        int32_t g;
    };
    union {
        int32_t z;
        int32_t b;
    };
    union {
        int32_t w;
        int32_t a;
    };
} argus_vector_4i_t;

typedef struct argus_vector_2u_t {
    uint32_t x;
    uint32_t y;
} argus_vector_2u_t;

typedef struct argus_vector_3u_t {
    union {
        uint32_t x;
        uint32_t r;
    };
    union {
        uint32_t y;
        uint32_t g;
    };
    union {
        uint32_t z;
        uint32_t b;
    };
} argus_vector_3u_t;

typedef struct argus_vector_4u_t {
    union {
        uint32_t x;
        uint32_t r;
    };
    union {
        uint32_t y;
        uint32_t g;
    };
    union {
        uint32_t z;
        uint32_t b;
    };
    union {
        uint32_t w;
        uint32_t a;
    };
} argus_vector_4u_t;

#ifdef __cplusplus
}
#endif
