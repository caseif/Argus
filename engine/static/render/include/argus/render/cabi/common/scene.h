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

#ifdef __cplusplus
extern "C" {
#endif

typedef void *argus_scene_t;
typedef const void *argus_scene_const_t;

typedef enum ArgusSceneType {
    ARGUS_SCENE_TYPE_TWO_D,
    ARGUS_SCENE_TYPE_THREE_D,
} ArgusSceneType;

argus_scene_t argus_scene_find(const char *id);

ArgusSceneType argus_scene_get_type(argus_scene_const_t scene);

#ifdef __cplusplus
}
#endif
