// module renderer
#include "argus/renderer.hpp"

namespace argus {

    using vmml::vec2f;
    using vmml::mat4f;

    Transform::Transform(void) {
        translation = {0, 0};
        rotation = 0;
        scale = {1, 1};
    }

    Transform::Transform(vec2f const &translation, const double rotation, vec2f const &scale) {
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

    vec2f const &Transform::get_translation(void) const {
        return translation;
    }

    void Transform::set_translation(vec2f const &translation) {
        this->translation = translation;
        this->dirty = true;
    }

    void Transform::add_translation(vec2f const &translation_delta) {
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

    vec2f const &Transform::get_scale(void) const {
        return scale;
    }

    void Transform::set_scale(vec2f const &scale) {
        this->scale = scale;
        this->dirty = true;
    }

    mat4f const Transform::to_matrix(void) const {
        float cos_rot;
        float sin_rot;
        sincosf32(rotation, &sin_rot, &cos_rot);
        std::vector<float> vals = {
            cos_rot * scale.x(),    sin_rot,                0,  translation.x(),
            sin_rot,                cos_rot * scale.y(),    0,  translation.y(),
            0,                      0,                      1,  0,
            0,                      0,                      0,  1
        };
        mat4f res = mat4f();
        res.set(vals.cbegin(), vals.cend());
        return res;
    }

    const bool Transform::is_dirty(void) const {
        return dirty;
    }

    void Transform::clean(void) {
        dirty = false;
    }

}