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

#include "argus/render/cabi/common/shader.h"

#include "argus/render/common/shader.hpp"

#include <utility>
#include <vector>

using argus::Shader;
using argus::ShaderReflectionInfo;

static inline const Shader &_as_ref(argus_shader_const_t shader) {
    return *reinterpret_cast<const Shader *>(shader);
}

static inline ShaderReflectionInfo &_refl_as_ref(argus_shader_refl_info_t refl) {
    return *reinterpret_cast<ShaderReflectionInfo *>(refl);
}

static inline const ShaderReflectionInfo &_refl_as_ref(argus_shader_refl_info_const_t refl) {
    return *reinterpret_cast<const ShaderReflectionInfo *>(refl);
}

extern "C" {

void argus_shader_refl_info_delete(argus_shader_refl_info_t refl) {
    delete reinterpret_cast<ShaderReflectionInfo *>(refl);
}

bool argus_shader_refl_info_has_attr(argus_shader_refl_info_const_t refl, const char *name) {
    return _refl_as_ref(refl).has_attr(name);
}

uint32_t argus_shader_refl_info_get_attr_loc(argus_shader_refl_info_const_t refl, const char *name, bool *out_found) {
    auto res = _refl_as_ref(refl).get_attr_loc(name);
    *out_found = res.has_value();
    return res.value_or(0);
}

void argus_shader_refl_info_set_attr_loc(argus_shader_refl_info_t refl, const char *name, uint32_t loc) {
    _refl_as_ref(refl).set_attr_loc(name, loc);
}

bool argus_shader_refl_info_has_output(argus_shader_refl_info_const_t refl, const char *name) {
    return _refl_as_ref(refl).has_output(name);
}

uint32_t argus_shader_refl_info_get_output_loc(argus_shader_refl_info_const_t refl, const char *name, bool *out_found) {
    auto res = _refl_as_ref(refl).get_attr_loc(name);
    *out_found = res.has_value();
    return res.value_or(0);
}

void argus_shader_refl_info_set_output_loc(argus_shader_refl_info_t refl, const char *name, uint32_t loc) {
    _refl_as_ref(refl).set_output_loc(name, loc);
}

bool argus_shader_refl_info_has_uniform_var(argus_shader_refl_info_const_t refl, const char *name) {
    return _refl_as_ref(refl).has_uniform(name);
}

bool argus_shader_refl_info_has_uniform(argus_shader_refl_info_const_t refl, const char *ubo, const char *name) {
    return _refl_as_ref(refl).has_uniform(ubo, name);
}

uint32_t argus_shader_refl_info_get_uniform_var_loc(argus_shader_refl_info_const_t refl, const char *name,
        bool *out_found) {
    auto res = _refl_as_ref(refl).get_uniform_loc(name);
    *out_found = res.has_value();
    return res.value_or(0);
}

uint32_t argus_shader_refl_info_get_uniform_loc(argus_shader_refl_info_const_t refl, const char *ubo, const char *name,
        bool *out_found) {
    auto res = _refl_as_ref(refl).get_uniform_loc(ubo, name);
    *out_found = res.has_value();
    return res.value_or(0);
}

void argus_shader_refl_info_set_uniform_var_loc(argus_shader_refl_info_t refl, const char *name, uint32_t loc) {
    _refl_as_ref(refl).set_uniform_loc(name, loc);
}

void argus_shader_refl_info_set_uniform_loc(argus_shader_refl_info_t refl, const char *ubo, const char *name,
        uint32_t loc) {
    _refl_as_ref(refl).set_uniform_loc(ubo, name, loc);
}

bool argus_shader_refl_info_has_ubo(argus_shader_refl_info_const_t refl, const char *name) {
    return _refl_as_ref(refl).has_ubo(name);
}

uint32_t argus_shader_refl_info_get_ubo_binding(argus_shader_refl_info_const_t refl, const char *name,
        bool *out_found) {
    auto res = _refl_as_ref(refl).get_ubo_binding(name);
    *out_found = res.has_value();
    return res.value_or(0);
}

void argus_shader_refl_info_set_ubo_binding(argus_shader_refl_info_t refl, const char *name, uint32_t binding) {
    _refl_as_ref(refl).set_ubo_binding(name, binding);
}

const char *argus_shader_refl_info_get_ubo_instance_name(argus_shader_refl_info_const_t refl, const char *name,
        bool *out_found) {
    auto res = _refl_as_ref(refl).get_ubo_instance_name(name);
    *out_found = res.has_value();
    if (res.has_value()) {
        return res.value().get().c_str();
    } else {
        return nullptr;
    }
}

void argus_shader_refl_info_set_ubo_instance_name(argus_shader_refl_info_t refl, const char *ubo_name,
        const char *instance_name) {
    _refl_as_ref(refl).set_ubo_instance_name(ubo_name, instance_name);
}

argus_shader_t argus_shader_new(const char *uid, const char *type, ArgusShaderStage stage,
        const unsigned char *src, size_t src_len) {
    return new Shader(uid, type, argus::ShaderStage(stage), std::vector<uint8_t>(src, src + src_len));
}

void argus_shader_delete(argus_shader_t shader) {
    delete reinterpret_cast<Shader *>(shader);
}

const char *argus_shader_get_uid(argus_shader_const_t shader) {
    return _as_ref(shader).get_uid().c_str();
}

const char *argus_shader_get_type(argus_shader_const_t shader) {
    return _as_ref(shader).get_type().c_str();
}

ArgusShaderStage argus_shader_get_stage(argus_shader_const_t shader) {
    return ArgusShaderStage(_as_ref(shader).get_stage());
}

void argus_shader_get_source(argus_shader_const_t shader, const unsigned char **out_ptr, size_t *out_len) {
    const auto &res = _as_ref(shader).get_source();
    *out_ptr = res.data();
    *out_len = res.size();
}

}
