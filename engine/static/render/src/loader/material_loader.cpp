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

#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/result.hpp"

#include "argus/resman/resource.hpp"
#include "argus/resman/resource_loader.hpp"
#include "argus/resman/resource_manager.hpp" // IWYU pragma: keep

#include "argus/render/defines.hpp"
#include "argus/render/common/material.hpp"
#include "argus/render/common/shader.hpp"
#include "internal/render/loader/material_loader.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"

#include "nlohmann/json.hpp"
#include "nlohmann/detail/exceptions.hpp"

#pragma GCC diagnostic pop

#include <fstream> // IWYU pragma: keep
#include <istream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#define KEY_TEXTURE "texture"
#define KEY_SHADERS "shaders"

#define KEY_SHADER_STAGE "stage"
#define KEY_SHADER_UID "uid"

#define SHADER_VERT "vertex"
#define SHADER_FRAG "fragment"
#define SHADER_GEOM "geometry"
#define SHADER_COMP "compute"
#define SHADER_MESH "mesh"
#define SHADER_TESS_CTRL "tess_control"
#define SHADER_TESS_EVAL "tess_evaluation"

namespace argus {
    static Result<void *, ResourceError> _make_knf_err(const ResourcePrototype &proto,
            const std::string &key) {
        return err<void *, ResourceError>(ResourceError { ResourceErrorReason::InvalidContent, proto.uid,
                "Material is missing required key '" + key + "'" });
    }

    template<typename T>
    static bool _try_get_key(const nlohmann::json &root, const std::string &key, T &dest) {
        if (root.contains(key)) {
            dest = root.at(key).get<T>();
            return true;
        } else {
            return false;
        }
    }

    MaterialLoader::MaterialLoader():
            ResourceLoader({ RESOURCE_TYPE_MATERIAL }) {
    }

    Result<void *, ResourceError> MaterialLoader::load(ResourceManager &manager, const ResourcePrototype &proto,
            std::istream &stream, size_t size) {
        UNUSED(proto);
        UNUSED(size);
        Logger::default_logger().debug("Loading material %s", proto.uid.c_str());

        nlohmann::json json_root = nlohmann::json::parse(stream, nullptr, false);
        if (json_root.is_discarded()) {
            return make_err_result(ResourceErrorReason::MalformedContent, proto);
        }

        std::string tex_uid;
        if (!_try_get_key<std::string>(json_root, KEY_TEXTURE, tex_uid)) {
            return _make_knf_err(proto, KEY_TEXTURE);
        }
        nlohmann::json::array_t shaders_arr;
        if (!_try_get_key(json_root, KEY_SHADERS, shaders_arr)) {
            return _make_knf_err(proto, KEY_SHADERS);
        }

        std::map<ShaderStage, std::string> shader_map;
        std::vector<std::string> shader_uids;
        for (auto shader_obj : shaders_arr) {
            std::string shader_type;
            if (!_try_get_key(shader_obj, KEY_SHADER_STAGE, shader_type)) {
                return _make_knf_err(proto, KEY_SHADER_STAGE);
            }
            std::string shader_uid;
            if (!_try_get_key(shader_obj, KEY_SHADER_UID, shader_uid)) {
                return _make_knf_err(proto, KEY_SHADER_UID);
            }

            ShaderStage stage;
            if (shader_type == SHADER_VERT) {
                stage = ShaderStage::Vertex;
            } else if (shader_type == SHADER_FRAG) {
                stage = ShaderStage::Fragment;
            } else {
                // we don't support any other shader stages right now
                return make_err_result(ResourceErrorReason::InvalidContent, proto,
                        "Invalid shader stage in material");
            }

            if (shader_map.find(stage) != shader_map.end()) {
                // only one shader can be specified per stage
                return make_err_result(ResourceErrorReason::InvalidContent, proto,
                        "Duplicate shader stage in material");
            }

            shader_map[stage] = shader_uid;
            shader_uids.push_back(shader_uid);
        }

        std::vector<std::string> dep_uids;
        dep_uids.insert(dep_uids.end(), shader_uids.begin(), shader_uids.end());

        auto deps_res = load_dependencies(manager, dep_uids);
        if (deps_res.is_err()) {
            return err<void *, ResourceError>(deps_res.unwrap_err());
        }
        auto deps = deps_res.unwrap();

        std::vector<const Shader *> shaders;
        for (auto &[stage, shader_uid] : shader_map) {
            const Shader &shader = deps[shader_uid]->get<const Shader>();
            if (shader.get_stage() != stage) {
                // stage of loaded shader does not match stage specified by material
                return make_err_result(ResourceErrorReason::InvalidContent, proto,
                        "Mismatched shader stage in material");
            }

            shaders.push_back(&shader);
        }

        Logger::default_logger().debug("Successfully loaded material %s", proto.uid.c_str());

        return make_ok_result(new Material(tex_uid, shader_uids));
    }

    Result<void *, ResourceError> MaterialLoader::copy(ResourceManager &manager, const ResourcePrototype &proto,
            void *src, std::type_index type) {
        if (type != std::type_index(typeid(Material))) {
            return make_err_result(ResourceErrorReason::UnexpectedReferenceType, proto);
        }

        auto &src_mat = *reinterpret_cast<Material *>(src);

        // need to load shaders as dependencies before doing a copy

        std::vector<std::string> dep_uids;
        dep_uids.insert(dep_uids.end(), src_mat.get_shader_uids().begin(), src_mat.get_shader_uids().end());

        std::map<std::string, const Resource *> deps;
        auto deps_res = load_dependencies(manager, dep_uids);
        if (deps_res.is_err()) {
            return err<void *, ResourceError>(deps_res.unwrap_err());
        }

        return make_ok_result(new Material(std::move(src_mat)));
    }

    void MaterialLoader::unload(void *const data_buf) {
        delete static_cast<Material *>(data_buf);
    }

}
