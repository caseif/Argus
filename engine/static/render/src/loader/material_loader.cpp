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

#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/macros.hpp"

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

#include <exception>
#include <fstream> // IWYU pragma: keep
#include <istream>
#include <map>
#include <stdexcept>
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
    MaterialLoader::MaterialLoader():
        ResourceLoader({ RESOURCE_TYPE_MATERIAL }) {
    }

    void *MaterialLoader::load(ResourceManager &manager, const ResourcePrototype &proto,
            std::istream &stream, size_t size) const {
        UNUSED(proto);
        UNUSED(size);
        Logger::default_logger().debug("Loading material %s", proto.uid.c_str());
        try {
            nlohmann::json json_root = nlohmann::json::parse(stream, nullptr, true);

            std::string tex_uid = json_root.at(KEY_TEXTURE);
            nlohmann::json::array_t shaders_arr = json_root.at(KEY_SHADERS);

            std::map<ShaderStage, std::string> shader_map;
            std::vector<std::string> shader_uids;
            for (auto shader_obj : shaders_arr) {
                std::string shader_type = shader_obj.at(KEY_SHADER_STAGE);
                std::string shader_uid = shader_obj.at(KEY_SHADER_UID);

                ShaderStage stage;
                if (shader_type == SHADER_VERT) {
                    stage = ShaderStage::Vertex;
                } else if (shader_type == SHADER_FRAG) {
                    stage = ShaderStage::Fragment;
                } else {
                    // we don't support any other shader stages right now
                    Logger::default_logger().warn("Invalid shader stage in material %s", proto.uid.c_str());
                    return nullptr;
                }

                if (shader_map.find(stage) != shader_map.end()) {
                    // only one shader can be specified per stage
                    Logger::default_logger().warn("Duplicate shader stage in material %s", proto.uid.c_str());
                    return nullptr;
                }

                shader_map[stage] = shader_uid;
                shader_uids.push_back(shader_uid);
            }

            std::vector<std::string> dep_uids;
            dep_uids.insert(dep_uids.end(), shader_uids.begin(), shader_uids.end());

            std::map<std::string, const Resource *> deps;
            try {
                deps = load_dependencies(manager, dep_uids);
            } catch (...) {
                Logger::default_logger().warn("Failed to load dependencies for material %s", proto.uid.c_str());
                return nullptr;
            }

            std::vector<const Shader *> shaders;
            for (auto &[stage, shader_uid] : shader_map) {
                const Shader &shader = deps[shader_uid]->get<const Shader>();
                if (shader.get_stage() != stage) {
                    // stage of loaded shader does not match stage specified by material
                    Logger::default_logger().warn("Mismatched shader stage in material %s", proto.uid.c_str());
                    return nullptr;
                }

                shaders.push_back(&shader);
            }

            Logger::default_logger().debug("Successfully loaded material %s", proto.uid.c_str());
            return new Material(tex_uid, shader_uids);
        } catch (nlohmann::detail::parse_error &) {
            Logger::default_logger().warn("Failed to parse material %s", proto.uid.c_str());
            return nullptr;
        } catch (std::out_of_range &) {
            Logger::default_logger().debug("Material %s is incomplete or malformed", proto.uid.c_str());
            return nullptr;
        } catch (std::exception &ex) {
            UNUSED(ex); // only gets used in debug mode
            Logger::default_logger().debug("Unspecified exception while parsing material %s (what: %s)",
                    proto.uid.c_str(), ex.what());
            return nullptr;
        }
    }

    void *MaterialLoader::copy(ResourceManager &manager, const ResourcePrototype &proto,
            void *src, std::type_index type) const {
        affirm_precond(type == std::type_index(typeid(Material)),
                "Incorrect pointer type passed to MaterialLoader::copy");

        auto &src_mat = *reinterpret_cast<Material *>(src);

        // need to load shaders as dependencies before doing a copy

        std::vector<std::string> dep_uids;
        dep_uids.insert(dep_uids.end(), src_mat.get_shader_uids().begin(), src_mat.get_shader_uids().end());

        std::map<std::string, const Resource *> deps;
        try {
            deps = load_dependencies(manager, dep_uids);
        } catch (...) {
            Logger::default_logger().warn("Failed to load dependencies for material %s", proto.uid.c_str());
            return nullptr;
        }

        return new Material(std::move(src_mat));
    }

    void MaterialLoader::unload(void *const data_buf) const {
        delete static_cast<Material *>(data_buf);
    }

}
