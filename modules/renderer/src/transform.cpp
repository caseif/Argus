#include "argus/renderer.hpp"

#include "vmmlib/matrix.hpp"
#include "vmmlib/vector.hpp"

namespace argus {

    using namespace vmml;

    Transform::Transform(void) {
        translation = *new Vector2d(0, 0);
        rotation = 0;
        scale = *new Vector2d(0, 0);
    }

    Transform::Transform(Vector2d translation, double rotation, Vector2d scale) {
        this->translation = translation;
        this->rotation = rotation;
        this->scale = scale;
    }

    Vector2d Transform::get_translation(void) const {
        return translation;
    }

    void Transform::set_translation(const Vector2d &translation) {
        this->translation = translation;
    }

    void Transform::add_translation(const Vector2d &translation_delta) {
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

    Vector2d Transform::get_scale(void) const {
        return scale;
    }

    void Transform::set_scale(const Vector2d &scale) {
        this->scale = scale;
    }

    void Transform::set_parent(const Transform &parent) {
        this->parent = new Transform(parent);
    }

    const vmml::Matrix3d &Transform::to_matrix(void) {
        //TODO
    }

}