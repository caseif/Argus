/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
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

// module lowlevel
#include "argus/lowlevel/macros.hpp"
#include "internal/lowlevel/logging.hpp"

// module resman
#include "argus/resman/resource.hpp"
#include "argus/resman/resource_loader.hpp"
#include "argus/resman/resource_manager.hpp" // IWYU pragma: keep

// module render
#include "argus/render/common/material.hpp"
#include "argus/render/common/shader.hpp"
#include "internal/render/defines.hpp"
#include "internal/render/loader/material_loader.hpp"


#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "nlohmann/detail/exceptions.hpp"

#include <algorithm>
#include <exception>
#include <fstream> // IWYU pragma: keep
#include <initializer_list>
#include <istream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <cstdio>

#define KEY_TEXTURE "texture"
#define KEY_SHADERS "shaders"
#define KEY_ATTRS "attributes"

#define KEY_SHADER_STAGE "stage"
#define KEY_SHADER_UID "uid"

#define SHADER_VERT "vertex"
#define SHADER_FRAG "fragment"
#define SHADER_GEOM "geometry"
#define SHADER_COMP "compute"
#define SHADER_MESH "mesh"
#define SHADER_TESS_CTRL "tess_control"
#define SHADER_TESS_EVAL "tess_evaluation"

#define ATTR_POS "position"
#define ATTR_NORM "normal"
#define ATTR_COLOR "color"
#define ATTR_TEXCOORD "texcoord"

namespace argus {
    MaterialLoader::MaterialLoader():
            ResourceLoader({ RESOURCE_TYPE_MATERIAL }) {
    }

    void *MaterialLoader::load(ResourceManager &manager, const ResourcePrototype &proto,
            std::istream &stream, size_t size) const {
        UNUSED(proto);
        UNUSED(size);
        _ARGUS_DEBUG("Loading material %s", proto.uid.c_str());
        try {
            nlohmann::json json_root = nlohmann::json::parse(stream, nullptr, true, true);

            std::string tex_uid = json_root.at(KEY_TEXTURE);
            nlohmann::json::array_t shaders_arr = json_root.at(KEY_SHADERS);
            std::vector<std::string> attrs_arr = json_root.at(KEY_ATTRS);

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
                    _ARGUS_WARN("Invalid shader stage in material %s", proto.uid.c_str());
                    return NULL;
                }

                if (shader_map.find(stage) != shader_map.end()) {
                    // only one shader can be specified per stage
                    _ARGUS_WARN("Duplicate shader stage in material %s", proto.uid.c_str());
                    return NULL;
                }

                shader_map[stage] = shader_uid;
                shader_uids.push_back(shader_uid);
            }

            VertexAttributes attrs = VertexAttributes::NONE;
            for (auto attr : attrs_arr) {
                if (attr == ATTR_POS) {
                    attrs |= VertexAttributes::POSITION;
                } else if (attr == ATTR_NORM) {
                    attrs |= VertexAttributes::NORMAL;
                } else if (attr == ATTR_COLOR) {
                    attrs |= VertexAttributes::COLOR;
                } else if (attr == ATTR_TEXCOORD) {
                    attrs |= VertexAttributes::TEXCOORD;
                } else {
                    // invalid attribute name
                    _ARGUS_WARN("Invalid vertex attribute name in material %s", proto.uid.c_str());
                    return NULL;
                }
            }

            std::vector<std::string> dep_uids;
            dep_uids.push_back(tex_uid);
            dep_uids.insert(dep_uids.end(), shader_uids.begin(), shader_uids.end());

            std::map<std::string, const Resource*> deps;
            try {
                deps = load_dependencies(manager, dep_uids);
            } catch (...) {
                _ARGUS_WARN("Failed to load dependencies for material %s", proto.uid.c_str());
                return NULL;
            }

            std::vector<const Shader*> shaders;
            for (auto shader_info : shader_map) {
                const Shader &shader = deps[shader_info.second]->get<const Shader>();
                if (shader.get_stage() != shader_info.first) {
                    // stage of loaded shader does not match stage specified by material
                    _ARGUS_WARN("Mismatched shader stage in material %s", proto.uid.c_str());
                    return NULL;
                }

                shaders.push_back(&shader);
            }

            _ARGUS_DEBUG("Successfully loaded material %s", proto.uid.c_str());
            return new Material(tex_uid, shader_uids, attrs);
        } catch (nlohmann::detail::parse_error&) {
            _ARGUS_WARN("Failed to parse material %s", proto.uid.c_str());
            return NULL;
        } catch (std::out_of_range&) {
            _ARGUS_DEBUG("Material %s is incomplete or malformed", proto.uid.c_str());
            return NULL;
        } catch (std::exception &ex) {
            UNUSED(ex); // only gets used in debug mode
            _ARGUS_DEBUG("Unspecified exception while parsing material %s (what: %s)", proto.uid.c_str(), ex.what());
            return NULL;
        }
    }

    void MaterialLoader::unload(void *const data_buf) const {
        delete static_cast<Material*>(data_buf);
    }

}
