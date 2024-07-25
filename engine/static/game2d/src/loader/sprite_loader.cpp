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

#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/macros.hpp"

#include "argus/core/engine.hpp"

#include "argus/resman/resource_manager.hpp"

#include "argus/game2d/sprite.hpp"
#include "internal/game2d/defines.hpp"
#include "internal/game2d/pimpl/sprite.hpp"
#include "internal/game2d/loader/sprite_loader.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"

#include "nlohmann/json.hpp"

#pragma GCC diagnostic pop

#define KEY_STATIC_FRAME "static_frame"
#define KEY_STATIC_FRAME_X "x"
#define KEY_STATIC_FRAME_Y "y"
#define KEY_DEF_ANIM "default_animation"
#define KEY_SPEED "anim_speed"
#define KEY_ATLAS "atlas"
#define KEY_TILE_WIDTH "tile_width"
#define KEY_TILE_HEIGHT "tile_height"
#define KEY_ANIMS "animations"

static constexpr const char *k_key_anim_loop = "loop";
static constexpr const char *k_key_anim_def_frame_dur = "frame_duration";
static constexpr const char *k_key_anim_padding = "padding";
static constexpr const char *k_key_anim_pad_top = "top";
static constexpr const char *k_key_anim_pad_bottom = "bottom";
static constexpr const char *k_key_anim_pad_left = "left";
static constexpr const char *k_key_anim_pad_right = "right";
static constexpr const char *k_key_anim_frames = "frames";
static constexpr const char *k_key_anim_frame_x = "x";
static constexpr const char *k_key_anim_frame_y = "y";
static constexpr const char *k_key_anim_frame_dur = "duration";

static constexpr const char *k_magic_anim_static = "_static";

namespace argus {
    template<typename T>
    static bool _try_get_key(const nlohmann::json &root, const std::string &key, T &dest) {
        if (root.contains(key)) {
            dest = root.at(key).get<T>();
            return true;
        } else {
            return false;
        }
    }

    SpriteLoader::SpriteLoader(void):
        ResourceLoader({ RESOURCE_TYPE_SPRITE }) {
    }

    Result<void *, ResourceError> SpriteLoader::load(ResourceManager &manager, const ResourcePrototype &proto,
            std::istream &stream, size_t size) {
        UNUSED(manager);
        UNUSED(size);

        Logger::default_logger().debug("Loading sprite %s", proto.uid.c_str());

        nlohmann::json json_root = nlohmann::json::parse(stream, nullptr, false);
        if (json_root.is_discarded()) {
            return make_err_result(ResourceErrorReason::MalformedContent, proto,
                    "Failed to parse sprite definition");
        }

        SpriteDef sprite;

        if (json_root.contains(KEY_SPEED)) {
            sprite.def_speed = json_root.at(KEY_SPEED);

            if (sprite.def_speed <= 0) {
                return make_err_result(ResourceErrorReason::InvalidContent, proto,
                        "Sprite animation speed must be > 0");
            }
        } else {
            sprite.def_speed = 1.0f;
        }

        if (json_root.contains(KEY_ATLAS)) {
            sprite.atlas = json_root.at(KEY_ATLAS);
        }

        // use oversized intermediate variables to be able to store UINT32_MAX in a signed value
        int64_t tile_width = 0;
        int64_t tile_height = 0;

        if (json_root.contains(KEY_TILE_WIDTH)) {
            if (!json_root.contains(KEY_TILE_HEIGHT)) {
                return make_err_result(ResourceErrorReason::InvalidContent, proto,
                        "Sprite specifies tile width but not height");
            }

            tile_width = json_root.at(KEY_TILE_WIDTH);
        }

        if (json_root.contains(KEY_TILE_HEIGHT)) {
            if (!json_root.contains(KEY_TILE_WIDTH)) {
                return make_err_result(ResourceErrorReason::InvalidContent, proto,
                        "Sprite specifies tile height but not width");
            }

            tile_height = json_root.at(KEY_TILE_HEIGHT);

            // these values default to zero, so we only want to validate them if they're provided
            if (tile_width <= 0 || tile_height <= 0) {
                return make_err_result(ResourceErrorReason::InvalidContent, proto,
                        "Sprite tile dimensions must be >= 0");
            }

            if (tile_width > UINT32_MAX || tile_height > UINT32_MAX) {
                return make_err_result(ResourceErrorReason::InvalidContent, proto,
                        "Sprite tile dimensions must be <= UINT32_MAX");
            }
        }

        sprite.tile_size.x = uint32_t(tile_width);
        sprite.tile_size.y = uint32_t(tile_height);

        if (sprite.tile_size.x > 0) {
            if (json_root.contains(KEY_DEF_ANIM)) {
                sprite.def_anim = json_root.at(KEY_DEF_ANIM);
            } else if (json_root.contains(KEY_STATIC_FRAME)) {
                sprite.def_anim = k_magic_anim_static;
            } else {
                return make_err_result(ResourceErrorReason::InvalidContent, proto,
                        "Sprite definition must specify at least one of '" KEY_DEF_ANIM "' or '"
                        KEY_STATIC_FRAME "' when tile size is provided explicitly");
            }

            if (!json_root.contains(KEY_ANIMS) && !json_root.contains(KEY_STATIC_FRAME)) {
                return make_err_result(ResourceErrorReason::InvalidContent, proto,
                        "Sprite must contain at least one of '" KEY_ANIMS "' or '"
                        KEY_STATIC_FRAME "' when tile size is provided explicitly");
            }

            if (json_root.contains(KEY_STATIC_FRAME)) {
                auto frame_json = json_root.at(KEY_STATIC_FRAME);
                // oversized intermediate variables again
                int64_t frame_x;
                if (!_try_get_key(frame_json, KEY_STATIC_FRAME_X, frame_x)) {
                    return make_err_result(ResourceErrorReason::InvalidContent, proto,
                            "Sprite static frame definition is missing frame x-offset");
                }
                int64_t frame_y;
                if (!_try_get_key(frame_json, KEY_STATIC_FRAME_Y, frame_y)) {
                    return make_err_result(ResourceErrorReason::InvalidContent, proto,
                            "Sprite static frame definition is missing frame y-offset");
                }

                if (frame_x < 0 || frame_y < 0) {
                    return make_err_result(ResourceErrorReason::InvalidContent, proto,
                            "Static frame offset values must be >= 0");
                }

                if (frame_x > UINT32_MAX || frame_y > UINT32_MAX) {
                    return make_err_result(ResourceErrorReason::InvalidContent, proto,
                            "Static frame offset values must be < UINT32_MAX");
                }

                SpriteAnimation static_anim;
                static_anim.id = KEY_STATIC_FRAME;
                static_anim.loop = false;

                AnimationFrame static_frame;
                static_frame.offset = { uint32_t(frame_x), uint32_t(frame_y) };
                static_frame.duration = 1;
                static_anim.frames.push_back(static_frame);

                sprite.animations.insert({ k_magic_anim_static, static_anim });
            }
        } else {
            if (json_root.contains(KEY_STATIC_FRAME)) {
                return make_err_result(ResourceErrorReason::InvalidContent, proto,
                        "Sprite definition must not include '" KEY_STATIC_FRAME "' when tile size is implicit");
            }

            if (json_root.contains(KEY_ANIMS)) {
                return make_err_result(ResourceErrorReason::InvalidContent, proto,
                        "Sprite definition must not include '" KEY_ANIMS "' when tile size is implicit");
            }

            sprite.def_anim = k_magic_anim_static;

            SpriteAnimation static_anim;
            static_anim.id = KEY_STATIC_FRAME;
            static_anim.loop = false;

            AnimationFrame static_frame;
            static_frame.offset = { 0, 0 };
            static_frame.duration = 1;
            static_anim.frames.push_back(static_frame);

            sprite.animations.insert({ k_magic_anim_static, static_anim });
        }

        if (json_root.contains(KEY_ANIMS)) {
            for (auto &[anim_id, anim_json] : json_root.at(KEY_ANIMS).get<nlohmann::json::object_t>()) {
                if (anim_id.empty()) {
                    return make_err_result(ResourceErrorReason::InvalidContent, proto,
                            "Sprite animation ID must be non-empty");
                }

                if (anim_id.at(0) == '_') {
                    return make_err_result(ResourceErrorReason::InvalidContent, proto,
                            "Sprite animation ID must not begin with underscore");
                }

                if (sprite.animations.find(anim_id) != sprite.animations.cend()) {
                    return make_err_result(ResourceErrorReason::InvalidContent, proto,
                            "Sprite animation \"" + anim_id + "\" is already defined");
                }

                SpriteAnimation anim;

                if (anim_json.contains(k_key_anim_loop)) {
                    anim.loop = anim_json.at(k_key_anim_loop);
                }

                if (anim_json.contains(k_key_anim_def_frame_dur)) {
                    anim.def_duration = anim_json.at(k_key_anim_def_frame_dur);
                    if (anim.def_duration <= 0) {
                        return make_err_result(ResourceErrorReason::InvalidContent, proto,
                                "Sprite frame duration must be >= 0");
                    }
                } else {
                    anim.def_duration = 0;
                }

                if (anim_json.contains(k_key_anim_padding)) {
                    auto padding_json = anim_json.at(k_key_anim_padding);

                    // oversized intermediate variables again
                    int64_t pad_left = 0;
                    int64_t pad_right = 0;
                    int64_t pad_top = 0;
                    int64_t pad_bottom = 0;

                    if (padding_json.contains(k_key_anim_pad_left)) {
                        pad_left = padding_json.at(k_key_anim_pad_left);
                    }

                    if (padding_json.contains(k_key_anim_pad_right)) {
                        pad_right = padding_json.at(k_key_anim_pad_right);
                    }

                    if (padding_json.contains(k_key_anim_pad_top)) {
                        pad_top = padding_json.at(k_key_anim_pad_top);
                    }

                    if (padding_json.contains(k_key_anim_pad_bottom)) {
                        pad_bottom = padding_json.at(k_key_anim_pad_bottom);
                    }

                    if (pad_left < 0 || pad_right < 0
                            || pad_top < 0 || pad_bottom < 0) {
                        return make_err_result(ResourceErrorReason::InvalidContent, proto,
                                "Sprite padding values must be >= 0");
                    }

                    if (pad_left > UINT32_MAX || pad_right > UINT32_MAX
                            || pad_top > UINT32_MAX || pad_bottom > UINT32_MAX) {
                        return make_err_result(ResourceErrorReason::InvalidContent, proto,
                                "Sprite padding values must be < UINT32_MAX");
                    }

                    if (pad_left + pad_right >= sprite.tile_size.x) {
                        return make_err_result(ResourceErrorReason::InvalidContent, proto,
                                "Sprite animation horizontal padding must not exceed atlas tile width");
                    }

                    if (pad_top + pad_bottom >= sprite.tile_size.y) {
                        return make_err_result(ResourceErrorReason::InvalidContent, proto,
                                "Sprite animation vertical padding must not exceed atlas tile height");
                    }

                    anim.padding.left = uint32_t(pad_left);
                    anim.padding.right = uint32_t(pad_right);
                    anim.padding.top = uint32_t(pad_top);
                    anim.padding.bottom = uint32_t(pad_bottom);
                }

                if (!anim_json.contains(k_key_anim_frames)) {
                    return make_err_result(ResourceErrorReason::InvalidContent, proto,
                            "Sprite animation '" + anim_id + "' is missing required key '"
                            + std::string(k_key_anim_frames) + "'");
                }

                for (auto &frame_json : anim_json.at(k_key_anim_frames).get<nlohmann::json::array_t>()) {
                    AnimationFrame frame;

                    // oversized intermediate variables again
                    int64_t offset_x;
                    if (!_try_get_key(frame_json, k_key_anim_frame_x, offset_x)) {
                        return make_err_result(ResourceErrorReason::InvalidContent, proto,
                                "Sprite animation '" + anim_id + "' is missing frame x-offset");
                    }
                    int64_t offset_y;
                    if (!_try_get_key(frame_json, k_key_anim_frame_y, offset_y)) {
                        return make_err_result(ResourceErrorReason::InvalidContent, proto,
                                "Sprite animation '" + anim_id + "' is missing frame y-offset");
                    }

                    if (offset_x < 0 || offset_y < 0) {
                        return make_err_result(ResourceErrorReason::InvalidContent, proto,
                                "Sprite animation frame offset values must be >= 0");
                    }

                    if (offset_x > UINT32_MAX || offset_y > UINT32_MAX) {
                        return make_err_result(ResourceErrorReason::InvalidContent, proto,
                                "Sprite animation frame offset values must be <= UINT32_MAX");
                    }

                    frame.offset.x = uint32_t(offset_x);
                    frame.offset.y = uint32_t(offset_y);

                    if (frame_json.contains(k_key_anim_frame_dur)) {
                        frame.duration = frame_json.at(k_key_anim_frame_dur);
                        if (frame.duration <= 0) {
                            return make_err_result(ResourceErrorReason::InvalidContent, proto,
                                    "Sprite animation frame duration must be > 0");
                        }
                    } else if (anim.def_duration > 0) {
                        frame.duration = anim.def_duration;
                    } else {
                        return make_err_result(ResourceErrorReason::InvalidContent, proto,
                                "Sprite animation frame duration must be provided if no default exists for the "
                                "containing animation");
                    }

                    anim.frames.push_back(frame);
                }

                sprite.animations.insert({ anim_id, anim });
            }
        }

        Logger::default_logger().debug("Successfully loaded sprite definition %s", proto.uid.c_str());

        return make_ok_result(new SpriteDef(std::move(sprite)));
    }

    Result<void *, ResourceError> SpriteLoader::copy(ResourceManager &manager, const ResourcePrototype &proto,
            void *src, std::type_index type) {
        if (type == std::type_index(typeid(SpriteDef))) {
            return make_err_result(ResourceErrorReason::UnexpectedReferenceType, proto);
        }

        auto &src_sprite = *static_cast<SpriteDef *>(src);

        std::vector<std::string> dep_uids;

        auto deps_res = load_dependencies(manager, dep_uids);
        if (deps_res.is_err()) {
            Logger::default_logger().warn("Failed to load dependency %s for sprite %s (%d)",
                    deps_res.unwrap_err().uid.c_str(), proto.uid.c_str(),
                    static_cast<std::underlying_type_t<ResourceErrorReason>>(deps_res.unwrap_err().reason));
        }

        return make_ok_result(new SpriteDef(src_sprite));
    }

    void SpriteLoader::unload(void *data_ptr) {
        delete static_cast<SpriteDef *>(data_ptr);
    }
}
