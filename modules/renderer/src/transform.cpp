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

    const vmml::vec2d Transform::get_translation(void) const {
        return translation;
    }

    void Transform::set_translation(const vmml::vec2d &translation) {
        this->translation = translation;
    }

    void Transform::add_translation(const vmml::vec2d &translation_delta) {
        this->translation += translation_delta;
    }
    
    double Transform::get_rotation(void) const {
        return rotation;
    }

    void Transform::set_rotation(double rotation_degrees) {
        this->rotation = rotation_degrees;
    }

    void Transform::add_rotation(double rotation_degrees) {
        this->rotation += rotation_degrees;
    }

    const vmml::vec2d Transform::get_scale(void) const {
        return scale;
    }

    void Transform::set_scale(const vmml::vec2d &scale) {
        this->scale = scale;
    }

    void Transform::set_parent(const Transform &parent) {
        this->parent = new Transform(parent);
    }

    const vmml::mat3d &Transform::to_matrix(void) const {
        //TODO
    }

}