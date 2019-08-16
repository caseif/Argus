// module renderer
#include "argus/renderer.hpp"

namespace argus {

    using vmml::vec2f;
    using vmml::mat4f;

    Transform::Transform(void):
        translation({0, 0}),
        rotation(0),
        scale({1, 1}) {
    }

    Transform::Transform(Transform &rhs):
            translation(rhs.get_translation()),
            rotation(rhs.get_rotation()),
            scale(rhs.get_scale()) {
    }

    Transform::Transform(Transform &&rhs):
            translation(rhs.get_translation()),
            rotation(rhs.get_rotation()),
            scale(rhs.get_scale()) {
    }

    Transform::Transform(vec2f const &translation, const float rotation, vec2f const &scale):
            translation(translation),
            rotation(rotation),
            scale(scale) {
    }

    Transform Transform::operator +(const Transform rhs) {
        return Transform(
                this->translation + rhs.translation,
                this->rotation.load() + rhs.rotation,
                this->scale * rhs.scale
        );
    }

    vec2f const Transform::get_translation(void) {
        this->translation_mutex.lock();
        vec2f translation_current = this->translation;
        this->translation_mutex.unlock();

        return translation;
    }

    void Transform::set_translation(vec2f const &translation) {
        translation_mutex.lock();
        this->translation = translation;
        translation_mutex.unlock();

        this->dirty = true;
    }

    void Transform::add_translation(vec2f const &translation_delta) {
        translation_mutex.lock();
        this->translation += translation_delta;
        translation_mutex.unlock();

        this->dirty = true;
    }
    
    const float Transform::get_rotation(void) const {
        return rotation;
    }

    void Transform::set_rotation(const float rotation_degrees) {
        this->rotation = rotation_degrees;

        this->dirty = true;
    }

    void Transform::add_rotation(const float rotation_degrees) {
        float current = this->rotation.load();
        while (!this->rotation.compare_exchange_weak(current, current + rotation_degrees));
        this->dirty = true;
    }

    vec2f const Transform::get_scale(void) {
        this->scale_mutex.lock();
        vec2f scale_current = this->scale;
        this->scale_mutex.unlock();

        return scale_current;
    }

    void Transform::set_scale(vec2f const &scale) {
        this->scale_mutex.lock();
        this->scale = scale;
        this->scale_mutex.unlock();

        this->dirty = true;
    }

    mat4f const Transform::to_matrix(void) {
        float cos_rot;
        float sin_rot;
        sincosf32(rotation, &sin_rot, &cos_rot);

        translation_mutex.lock();
        vec2f translation_current = translation;
        translation_mutex.unlock();
        
        scale_mutex.lock();
        vec2f scale_current = scale;
        scale_mutex.unlock();

        std::vector<float> vals = {
            cos_rot * scale_current.x(),    -sin_rot,                       0,  translation_current.x(),
            sin_rot,                        cos_rot * scale_current.y(),    0,  translation_current.y(),
            0,                              0,                              1,  0,
            0,                              0,                              0,  1
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