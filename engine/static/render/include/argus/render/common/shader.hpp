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

#include <functional>
#include <initializer_list>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <cstdint>

namespace argus {
    // forward declarations
    struct pimpl_Shader;

    constexpr const char *SHADER_TYPE_GLSL = "glsl";
    constexpr const char *SHADER_TYPE_SPIR_V = "spirv";

    /**
     * \brief Represents a stage corresponding to a step in the render pipeline.
     */
    enum class ShaderStage : uint32_t {
        Vertex = 0x01,
        Fragment = 0x02
    };

    ShaderStage operator|(ShaderStage lhs, ShaderStage rhs);

    ShaderStage &operator|=(ShaderStage &lhs, ShaderStage rhs);

    ShaderStage operator&(ShaderStage lhs, ShaderStage rhs);

    struct ShaderReflectionInfo {
        std::map<std::string, uint32_t> attribute_locations;
        std::map<std::string, uint32_t> output_locations;
        std::map<std::string, uint32_t> uniform_variable_locations;
        std::map<std::string, uint32_t> buffer_locations;
        std::map<std::string, uint32_t> ubo_bindings;
        std::map<std::string, std::string> ubo_instance_names;

        [[nodiscard]] bool has_attr(const std::string &name) const;

        [[nodiscard]] std::optional<uint32_t> get_attr_loc(const std::string &name) const;

        void get_attr_loc_and_then(const std::string &name, std::function<void(uint32_t)> fn) const;

        [[nodiscard]] bool has_output(const std::string &name) const;

        [[nodiscard]] std::optional<uint32_t> get_output_loc(const std::string &name) const;

        void get_output_loc_and_then(const std::string &name, std::function<void(uint32_t)> fn) const;

        [[nodiscard]] bool has_uniform(const std::string &name) const;

        [[nodiscard]] bool has_uniform(const std::string &ubo, const std::string &name) const;

        [[nodiscard]] std::optional<uint32_t> get_uniform_loc(const std::string &name) const;

        [[nodiscard]] std::optional<uint32_t> get_uniform_loc(const std::string &ubo, const std::string &name) const;

        void get_uniform_loc_and_then(const std::string &name, std::function<void(uint32_t)> fn) const;

        void get_uniform_loc_and_then(const std::string &ubo, const std::string &name,
                std::function<void(uint32_t)> fn) const;

        [[nodiscard]] bool has_ubo(const std::string &name) const;

        [[nodiscard]] std::optional<uint32_t> get_ubo_binding(const std::string &name) const;

        void get_ubo_binding_and_then(const std::string &name, std::function<void(uint32_t)> fn) const;

        [[nodiscard]] std::optional<std::string> get_ubo_instance_name(const std::string &name) const;
    };

    /**
     * \brief Represents a shader for use with a RenderObject.
     */
    class Shader {
      public:
        pimpl_Shader *m_pimpl;

        /**
         * \brief Constructs a new Shader with the given parameters.
         *
         * \param uid The unique identifier of the shader.
         * \param type The type of shader stored by this object.
         * \param stage The stage of the graphblock_locationsics pipeline this shader is to
         *        be run at.
         * \param src The source data of the Shader.
         */
        Shader(const std::string &uid, const std::string &type, ShaderStage stage, const std::vector<uint8_t> &src);

        Shader(const Shader &) noexcept;

        Shader(Shader &&) noexcept;

        ~Shader(void);

        const std::string &get_uid(void) const;

        const std::string &get_type(void) const;

        ShaderStage get_stage(void) const;

        const std::vector<uint8_t> &get_source(void) const;
    };
}
