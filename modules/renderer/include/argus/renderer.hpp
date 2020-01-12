/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

/**
 * \file argus/renderer.hpp
 *
 * Primary rendering engine interface, providing high-level abstractions for
 * rendering concepts.
 */

#pragma once

#include "argus/renderer/render_group.hpp"
#include "argus/renderer/render_layer.hpp"
#include "argus/renderer/renderable_factory.hpp"
#include "argus/renderer/renderable_square.hpp"
#include "argus/renderer/renderable_triangle.hpp"
#include "argus/renderer/renderable.hpp"
#include "argus/renderer/renderer.hpp"
#include "argus/renderer/shader_program.hpp"
#include "argus/renderer/shader.hpp"
#include "argus/renderer/texture_data.hpp"
#include "argus/renderer/transform.hpp"
#include "argus/renderer/window_event.hpp"
#include "argus/renderer/window.hpp"
