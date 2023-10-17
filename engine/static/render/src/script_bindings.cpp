#include "argus/scripting.hpp"

#include "argus/render/common/transform.hpp"
#include "internal/render/script_bindings.hpp"

namespace argus {
    static void _register_transform_symbols(void) {
        bind_type<Transform2D>("Transform2D");
        bind_member_instance_function("get_translation", &Transform2D::get_translation);
        bind_member_instance_function("get_rotation", &Transform2D::get_rotation);
        bind_member_instance_function("get_scale", &Transform2D::get_scale);
        bind_extension_function<Transform2D>("x",
                +[](const Transform2D &transform) { return transform.get_translation().x; });
        bind_extension_function<Transform2D>("y",
                +[](const Transform2D &transform) { return transform.get_translation().y; });
        bind_extension_function<Transform2D>("sx",
                +[](const Transform2D &transform) { return transform.get_scale().x; });
        bind_extension_function<Transform2D>("sy",
                +[](const Transform2D &transform) { return transform.get_scale().y; });
    }

    void register_render_script_bindings(void) {
        _register_transform_symbols();
    }
}
