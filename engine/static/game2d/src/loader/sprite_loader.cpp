/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

#include "argus/game2d/sprite.hpp"
#include "internal/game2d/defines.hpp"
#include "internal/game2d/pimpl/sprite.hpp"
#include "internal/game2d/loader/sprite_loader.hpp"

#include "nlohmann/json.hpp"

#define KEY_WIDTH "width"
#define KEY_HEIGHT "height"
#define KEY_STATIC_FRAME "static_frame"
#define KEY_STATIC_FRAME_X "x"
#define KEY_STATIC_FRAME_Y "y"
#define KEY_DEF_ANIM "default_animation"
#define KEY_SPEED "anim_speed"
#define KEY_ATLAS "atlas"
#define KEY_TILE_WIDTH "tile_width"
#define KEY_TILE_HEIGHT "tile_height"
#define KEY_ANIMS "animations"

const char *KEY_ANIM_LOOP = "loop";
const char *KEY_ANIM_DEF_FRAME_DUR = "frame_duration";
const char *KEY_ANIM_PADDING = "padding";
const char *KEY_ANIM_PAD_TOP = "top";
const char *KEY_ANIM_PAD_BOTTOM = "bottom";
const char *KEY_ANIM_PAD_LEFT = "left";
const char *KEY_ANIM_PAD_RIGHT = "right";
const char *KEY_ANIM_FRAMES = "frames";
const char *KEY_ANIM_FRAME_X = "x";
const char *KEY_ANIM_FRAME_Y = "y";
const char *KEY_ANIM_FRAME_DUR = "duration";

const char *MAGIC_ANIM_STATIC = "_static";

namespace argus {
    SpriteLoader::SpriteLoader(void): ResourceLoader({ RESOURCE_TYPE_SPRITE }) {
    }

    void *SpriteLoader::load(ResourceManager &manager, const ResourcePrototype &proto,
            std::istream &stream, size_t size) const {
        UNUSED(manager);
        UNUSED(proto);
        UNUSED(size);
        Logger::default_logger().debug("Loading material %s", proto.uid.c_str());
        try {
            nlohmann::json json_root = nlohmann::json::parse(stream, nullptr, true, true);

            SpriteDef sprite;

            // required attribute
            sprite.base_size.x = json_root.at(KEY_WIDTH);
            // required attribute
            sprite.base_size.y = json_root.at(KEY_HEIGHT);

            if (sprite.base_size.x <= 0 || sprite.base_size.y <= 0) {
                Logger::default_logger().severe("Sprite dimensions must be > 0");
                return nullptr;
            }

            if (json_root.contains(KEY_SPEED)) {
                sprite.def_speed = json_root.at(KEY_SPEED);

                if (sprite.def_speed <= 0) {
                    Logger::default_logger().severe("Sprite animation speed must be > 0");
                    return nullptr;
                }
            } else {
                sprite.def_speed = 1.0f;
            }

            if (json_root.contains(KEY_DEF_ANIM)) {
                sprite.def_anim = json_root.at(KEY_DEF_ANIM);
            } else if (json_root.contains(KEY_STATIC_FRAME)) {
                sprite.def_anim = MAGIC_ANIM_STATIC;
            } else {
                Logger::default_logger().severe("Sprite definition must specify at least one of "
                                                KEY_DEF_ANIM " or " KEY_STATIC_FRAME);
                return nullptr;
            }

            if (json_root.contains(KEY_ATLAS)) {
                sprite.atlas = json_root.at(KEY_ATLAS);
            }

            // use oversized intermediate variables to be able to store UINT32_MAX in a signed value
            int64_t tile_width = 0;
            int64_t tile_height = 0;

            if (json_root.contains(KEY_TILE_WIDTH)) {
                if (!json_root.contains(KEY_TILE_HEIGHT)) {
                    Logger::default_logger().severe("Sprite specifies tile width but not height");
                    return nullptr;
                }

                tile_width = json_root.at(KEY_TILE_WIDTH);
            }

            if (json_root.contains(KEY_TILE_HEIGHT)) {
                if (!json_root.contains(KEY_TILE_WIDTH)) {
                    Logger::default_logger().severe("Sprite specifies tile height but not width");
                    return nullptr;
                }

                tile_height = json_root.at(KEY_TILE_HEIGHT);

                // these values default to zero, so we only want to validate them if they're provided
                if (tile_width <= 0 || tile_height <= 0) {
                    Logger::default_logger().severe("Sprite tile dimensions must be >= 0");
                    return nullptr;
                }

                if (tile_width > UINT32_MAX || tile_height > UINT32_MAX) {
                    Logger::default_logger().severe("Sprite tile dimensions must be <= UINT32_MAX");
                    return nullptr;
                }
            }

            sprite.tile_size.x = static_cast<uint32_t>(tile_width);
            sprite.tile_size.y = static_cast<uint32_t>(tile_height);

            if (!json_root.contains(KEY_ANIMS) && !json_root.contains(KEY_STATIC_FRAME)) {
                Logger::default_logger().severe("Sprite must contain at least one of "
                                                KEY_ANIMS " or " KEY_STATIC_FRAME);
                return nullptr;
            }

            if (json_root.contains(KEY_STATIC_FRAME)) {
                auto frame_json = json_root.at(KEY_STATIC_FRAME);
                // oversized intermediate variables again
                int64_t frame_x = frame_json.at(KEY_STATIC_FRAME_X);
                int64_t frame_y = frame_json.at(KEY_STATIC_FRAME_Y);

                if (frame_x < 0 || frame_y < 0) {
                    Logger::default_logger().severe("Static frame offset values must be >= 0");
                    return nullptr;
                }

                if (frame_x > UINT32_MAX || frame_y > UINT32_MAX) {
                    Logger::default_logger().severe("Static frame offset values must be < UINT32_MAX");
                    return nullptr;
                }

                SpriteAnimation static_anim;
                static_anim.id = KEY_STATIC_FRAME;
                static_anim.loop = false;

                AnimationFrame static_frame;
                printf("%ld, %ld\n", frame_x, frame_y);
                static_frame.offset = { static_cast<uint32_t>(frame_x), static_cast<uint32_t>(frame_y) };
                static_frame.duration = 1;
                static_anim.frames.push_back(static_frame);

                sprite.animations.insert({ MAGIC_ANIM_STATIC, static_anim });
            }

            if (json_root.contains(KEY_ANIMS)) {
                for (auto &anim_pair : json_root.at(KEY_ANIMS).get<nlohmann::json::object_t>()) {
                    auto &anim_id = anim_pair.first;
                    auto &anim_json = anim_pair.second;

                    if (anim_id.length() == 0) {
                        Logger::default_logger().severe("Sprite animation ID must be non-empty");
                        return nullptr;
                    }

                    if (anim_id.at(0) == '_') {
                        Logger::default_logger().severe("Sprite animation ID must not begin with underscore");
                        return nullptr;
                    }

                    if (sprite.animations.find(anim_id) != sprite.animations.cend()) {
                        Logger::default_logger().severe("Sprite animation \"" + anim_id + "\" is already defined");
                        return nullptr;
                    }

                    SpriteAnimation anim;

                    if (anim_json.contains(KEY_ANIM_LOOP)) {
                        anim.loop = anim_json.at(KEY_ANIM_LOOP);
                    }

                    if (anim_json.contains(KEY_ANIM_DEF_FRAME_DUR)) {
                        anim.def_duration = anim_json.at(KEY_ANIM_DEF_FRAME_DUR);
                        if (anim.def_duration <= 0) {
                            Logger::default_logger().severe("Sprite frame duration must be >= 0");
                            return nullptr;
                        }
                    } else {
                        anim.def_duration = 0;
                    }

                    if (anim_json.contains(KEY_ANIM_PADDING)) {
                        auto padding_json = anim_json.at(KEY_ANIM_PADDING);

                        // oversized intermediate variables again
                        int64_t pad_left = 0;
                        int64_t pad_right = 0;
                        int64_t pad_top = 0;
                        int64_t pad_bottom = 0;

                        if (padding_json.contains(KEY_ANIM_PAD_LEFT)) {
                            pad_left = padding_json.at(KEY_ANIM_PAD_LEFT);
                        }

                        if (padding_json.contains(KEY_ANIM_PAD_RIGHT)) {
                            pad_right = padding_json.at(KEY_ANIM_PAD_RIGHT);
                        }

                        if (padding_json.contains(KEY_ANIM_PAD_TOP)) {
                            pad_top = padding_json.at(KEY_ANIM_PAD_TOP);
                        }

                        if (padding_json.contains(KEY_ANIM_PAD_BOTTOM)) {
                            pad_bottom = padding_json.at(KEY_ANIM_PAD_BOTTOM);
                        }

                        if (pad_left < 0 || pad_right < 0
                                || pad_top < 0 || pad_bottom < 0) {
                            Logger::default_logger().severe("Sprite padding values must be >= 0");
                            return nullptr;
                        }

                        if (pad_left > UINT32_MAX || pad_right > UINT32_MAX
                            || pad_top > UINT32_MAX || pad_bottom > UINT32_MAX) {
                            Logger::default_logger().severe("Sprite padding values must be < UINT32_MAX");
                            return nullptr;
                        }

                        if (pad_left + pad_right >= sprite.tile_size.x) {
                            Logger::default_logger().severe("Sprite animation horizontal padding must not "
                                                            "exceed atlas tile width");
                            return nullptr;
                        }

                        if (pad_top + pad_bottom >= sprite.tile_size.y) {
                            Logger::default_logger().severe("Sprite animation vertical padding must not "
                                                            "exceed atlas tile height");
                            return nullptr;
                        }

                        anim.padding.left = static_cast<uint32_t>(pad_left);
                        anim.padding.right = static_cast<uint32_t>(pad_right);
                        anim.padding.top = static_cast<uint32_t>(pad_top);
                        anim.padding.bottom = static_cast<uint32_t>(pad_bottom);
                    }

                    for (auto &frame_json : anim_json.at(KEY_ANIM_FRAMES).get<nlohmann::json::array_t>()) {
                        AnimationFrame frame;

                        // oversized intermediate variables again
                        int64_t offset_x = frame_json.at(KEY_ANIM_FRAME_X);
                        int64_t offset_y = frame_json.at(KEY_ANIM_FRAME_Y);

                        if (offset_x < 0 || offset_y < 0) {
                            Logger::default_logger().severe("Sprite animation frame offset values must be >= 0");
                            return nullptr;
                        }

                        if (offset_x > UINT32_MAX || offset_y > UINT32_MAX) {
                            Logger::default_logger().severe("Sprite animation frame offset values must be <= UINT32_MAX");
                            return nullptr;
                        }

                        frame.offset.x = static_cast<uint32_t>(offset_x);
                        frame.offset.y = static_cast<uint32_t>(offset_y);

                        if (frame_json.contains(KEY_ANIM_FRAME_DUR)) {
                            frame.duration = frame_json.at(KEY_ANIM_FRAME_DUR);
                            if (frame.duration <= 0) {
                                Logger::default_logger().severe("Sprite animation frame duration must be > 0");
                                return nullptr;
                            }
                        } else if (anim.def_duration > 0) {
                            frame.duration = anim.def_duration;
                        } else {
                            Logger::default_logger().severe("Sprite animation frame duration must be provided if no "
                                                            "default exists for the containing animation");
                            return nullptr;
                        }

                        anim.frames.push_back(frame);
                    }

                    sprite.animations.insert({ anim_id, anim });
                }
            }

            Logger::default_logger().debug("Successfully loaded sprite definition %s", proto.uid.c_str());

            return new SpriteDef(std::move(sprite));
        } catch (nlohmann::detail::parse_error&) {
            Logger::default_logger().severe("Failed to parse sprite definition %s", proto.uid.c_str());
            return nullptr;
        } catch (std::out_of_range&) {
            Logger::default_logger().severe("Sprite definition %s is incomplete or malformed", proto.uid.c_str());
            return nullptr;
        } catch (std::exception &ex) {
            Logger::default_logger().severe("Unspecified exception while parsing sprite definition %s (what: %s)",
                    proto.uid.c_str(), ex.what());
            return nullptr;
        }
    }

    void *SpriteLoader::copy(ResourceManager &manager, const ResourcePrototype &proto,
            void *src, std::type_index type) const {
        _ARGUS_ASSERT(type == std::type_index(typeid(SpriteDef)),
                "Incorrect pointer type passed to SpriteLoader::copy");

        auto &src_sprite = *reinterpret_cast<SpriteDef*>(src);

        std::vector<std::string> dep_uids;

        std::map<std::string, const Resource*> deps;
        try {
            deps = load_dependencies(manager, dep_uids);
        } catch (...) {
            Logger::default_logger().warn("Failed to load dependencies for sprite %s", proto.uid.c_str());
            return nullptr;
        }

        return new SpriteDef(std::move(src_sprite));
    }

    void SpriteLoader::unload(void *data_ptr) const {
        delete static_cast<Sprite*>(data_ptr);
    }
}
