// module renderer
#include "argus/renderer.hpp"

namespace argus {

    using vmml::vec2d;
    using vmml::mat4f;

    Transform::Transform(void) {
        translation = {0, 0};
        rotation = 0;
        scale = {0, 0};
    }

    Transform::Transform(vec2d const &translation, const double rotation, vec2d const &scale) {
        this->translation = translation;
        this->rotation = rotation;
        this->scale = scale;
    }

    Transform Transform::operator +(const Transform rhs) {
        return Transform(
                this->translation + rhs.translation,
                this->rotation + rhs.rotation,
                this->scale * rhs.scale
        );
    }

    vec2d const &Transform::get_translation(void) const {
        return translation;
    }

    void Transform::set_translation(vec2d const &translation) {
        this->translation = translation;
        this->dirty = true;
    }

    void Transform::add_translation(vec2d const &translation_delta) {
        this->translation += translation_delta;
        this->dirty = true;
    }
    
    const double Transform::get_rotation(void) const {
        return rotation;
    }

    void Transform::set_rotation(const double rotation_degrees) {
        this->rotation = rotation_degrees;
        this->dirty = true;
    }

    void Transform::add_rotation(const double rotation_degrees) {
        this->rotation += rotation_degrees;
        this->dirty = true;
    }

    vec2d const &Transform::get_scale(void) const {
        return scale;
    }

    void Transform::set_scale(vec2d const &scale) {
        this->scale = scale;
        this->dirty = true;
    }

    mat4f const &Transform::to_matrix(void) const {
        //TODO
    }

    const bool Transform::is_dirty(void) const {
        return dirty;
    }

    void Transform::clean(void) {
        dirty = false;
    }

}