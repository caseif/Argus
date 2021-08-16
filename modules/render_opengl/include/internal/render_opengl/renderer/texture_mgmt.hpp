/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <string>

namespace argus {
    // forward declarations
    struct RendererState;
    class Resource;

    void prepare_texture(RendererState &state, const Resource &material_res);

    void deinit_texture(RendererState &state, const std::string &texture_uid);
}
