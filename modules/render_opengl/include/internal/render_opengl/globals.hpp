/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module render_opengl
#include "internal/render_opengl/glfw_include.hpp"

#include "aglet/aglet.h"

#define _VERTEX_POSITION_LEN 2
#define _VERTEX_COLOR_LEN 4
#define _VERTEX_TEXCOORD_LEN 2
#define _VERTEX_LEN (_VERTEX_POSITION_LEN + _VERTEX_COLOR_LEN + _VERTEX_TEXCOORD_LEN)
#define _VERTEX_WORD_LEN sizeof(GLfloat)

#define _GL_LOG_MAX_LEN 255

#define _UNIFORM_PROJECTION "_argus_uni_projection_matrix"
#define _UNIFORM_TEXTURE "_argus_uni_sampler_array"
#define _UNIFORM_LAYER_TRANSFORM "_argus_uni_layer_transform"
#define _UNIFORM_GROUP_TRANSFORM "_argus_uni_group_transform"

#define SHADER_ATTRIB_IN_POSITION "in_Position"
#define SHADER_ATTRIB_IN_NORMAL "in_Normal"
#define SHADER_ATTRIB_IN_COLOR "in_Color"
#define SHADER_ATTRIB_IN_TEXCOORD "in_TexCoord"

#define SHADER_ATTRIB_IN_POSITION_LEN 2
#define SHADER_ATTRIB_IN_NORMAL_LEN 2
#define SHADER_ATTRIB_IN_COLOR_LEN 4
#define SHADER_ATTRIB_IN_TEXCOORD_LEN 2

#define SHADER_ATTRIB_LOC_POSITION 0
#define SHADER_ATTRIB_LOC_NORMAL 1
#define SHADER_ATTRIB_LOC_COLOR 2
#define SHADER_ATTRIB_LOC_TEXCOORD 3

#define SHADER_ATTRIB_OUT_FRAGDATA "out_Color"

#define SHADER_UNIFORM_VIEW_MATRIX "uniform_ViewMat"

#define FRAME_SHADER_PASS_TEXCOORD "pass_TexCoord"

#define FB_SHADER_VERT "\
    #version 330 core \n\
    in vec2 " SHADER_ATTRIB_IN_POSITION "; \n\
    in vec2 " SHADER_ATTRIB_IN_TEXCOORD "; \n\
    out vec2 " FRAME_SHADER_PASS_TEXCOORD "; \n\
    void main() { \n\
        gl_Position = vec4(" SHADER_ATTRIB_IN_POSITION ", 0.0, 1.0); \n\
        " FRAME_SHADER_PASS_TEXCOORD " = " SHADER_ATTRIB_IN_TEXCOORD "; \n\
    }"

#define FB_SHADER_FRAG "\n\
    #version 330 core \n\
    in vec2 " FRAME_SHADER_PASS_TEXCOORD "; \n\
    out vec4 " SHADER_ATTRIB_OUT_FRAGDATA "; \n\
    uniform sampler2D screenTex; \n\
    void main() { \n\
        " SHADER_ATTRIB_OUT_FRAGDATA " = texture(screenTex, " FRAME_SHADER_PASS_TEXCOORD "); \n\
        //" SHADER_ATTRIB_OUT_FRAGDATA " = vec4(1.0, 0.0, 0.0, 1.0); \n\
    }"

#define FB_SHADER_VERT_PATH "argus:shader/framebuffer_vert"
#define FB_SHADER_FRAG_PATH "argus:shader/framebuffer_frag"

namespace argus {
    extern float g_view_matrix[16];

    // all typedefs here serve purely to provide semantic information to declarations

    typedef GLuint buffer_handle_t;
    typedef GLuint array_handle_t;
    typedef GLuint texture_handle_t;
    typedef GLuint shader_handle_t;
    typedef GLuint program_handle_t;
    typedef GLint uniform_location_t;
}
