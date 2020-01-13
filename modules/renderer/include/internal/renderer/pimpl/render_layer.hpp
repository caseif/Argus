#pragma once

// module renderer
#include "argus/renderer/render_group.hpp"
#include "argus/renderer/renderer.hpp"
#include "argus/renderer/shader.hpp"
#include "argus/renderer/transform.hpp"

#include <vector>

namespace argus {
    struct pimpl_RenderLayer {
        /**
         * \brief The Renderer parent to this layer.
         */
        Renderer &parent_renderer;
        /**
         * \brief The priority of this layer.
         *
         * Higher-priority layers will be rendered later, on top of
         * lower-priority ones.
         */
        const int priority;

        /**
         * \brief The \link RenderGroup RenderGroups \endlink contained by
         *        this layer.
         */
        std::vector<RenderGroup*> children;
        /**
         * \brief The \link Shader Shaders \endlink contained by this layer.
         */
        std::vector<const Shader*> shaders;

        /**
         * \brief The implicit default RenderGroup of this layer.
         *
         * \remark A pointer to this RenderGroup is present in the children
         *         vector.
         */
        RenderGroup *def_group; // this depends on shaders being initialized

        /**
         * \brief The Transform of this RenderLayer.
         *
         * \sa RenderGroup#get_transform
         */
        Transform transform;

        /**
         * \brief Whether the shader list has been modified since it was
         *        last built.
         */
        bool dirty_shaders;
        
        pimpl_RenderLayer(Renderer &parent, const int priority):
                parent_renderer(parent),
                priority(priority),
                transform() {
        }
    };
}
