// module renderer
#include "argus/renderer.hpp"

#define VMMLIB_OLD_TYPEDEFS
#include "vmmlib/matrix.hpp"
#include "vmmlib/vector.hpp"

namespace argus {

    Transform::Transform(void) {
        translation = *new vmml::vec2d(0, 0);
        rotation = 0;
        scale = *new vmml::vec2d(0, 0);
        parent = nullptr;
    }

    Transform::Transform(vmml::vec2d translation, double rotation, vmml::vec2d scale) {
        this->translation = translation;
        this->rotation = rotation;
        this->scale = scale;
    }

    Transform::~Transform(void) {
        if (parent != nullptr) {
            free(parent);
        }
    }

    Transform Transform::operator +(const Transform rhs) {
        return *new Transform(
                this->translation + rhs.translation,
                this->rotation + rhs.rotation,
                this->scale * rhs.scale
        );
    }

    const vmml::vec2d &Transform::get_translation(void) const {
        return translation;
    }

    void Transform::set_translation(vmml::vec2d &translation) {
        this->translation = translation;
        this->dirty = true;
    }

    void Transform::add_translation(vmml::vec2d &translation_delta) {
        this->translation += translation_delta;
        this->dirty = true;
    }
    
    const double Transform::get_rotation(void) const {
        return rotation;
    }

    void Transform::set_rotation(double rotation_degrees) {
        this->rotation = rotation_degrees;
        this->dirty = true;
    }

    void Transform::add_rotation(double rotation_degrees) {
        this->rotation += rotation_degrees;
        this->dirty = true;
    }

    const vmml::vec2d &Transform::get_scale(void) const {
        return scale;
    }

    void Transform::set_scale(vmml::vec2d &scale) {
        this->scale = scale;
        this->dirty = true;
    }

    void Transform::set_parent(Transform &parent) {
        this->parent = new Transform(parent);
        this->dirty = true;
    }

    const vmml::mat4f &Transform::to_matrix(void) const {
        //TODO
    }

    const bool Transform::is_dirty(void) const {
        return dirty;
    }

    void Transform::clean(void) {
        dirty = false;
    }

}