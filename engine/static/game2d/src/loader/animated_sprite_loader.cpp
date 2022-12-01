/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
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

#include "argus/game2d/animated_sprite.hpp"
#include "internal/game2d/defines.hpp"
#include "internal/game2d/pimpl/animated_sprite.hpp"
#include "internal/game2d/loader/animated_sprite_loader.hpp"

#include "nlohmann/json.hpp"

const char *KEY_WIDTH = "width";
const char *KEY_HEIGHT = "height";
const char *KEY_DEF_ANIM = "default_animation";
const char *KEY_SPEED = "anim_speed";
const char *KEY_ATLAS = "atlas";
const char *KEY_TILE_WIDTH = "tile_width";
const char *KEY_TILE_HEIGHT = "tile_height";
const char *KEY_ANIMS = "animations";

const char *KEY_ANIM_LOOP = "loop";
const char *KEY_ANIM_ATLAS = "atlas";
const char *KEY_ANIM_OFF_TYPE = "offset_type";
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

const char *ENUM_OFF_TYPE_TILE = "tile";
const char *ENUM_OFF_TYPE_ABS = "absolute";

namespace argus {
    AnimatedSpriteLoader::AnimatedSpriteLoader(void): ResourceLoader({ RESOURCE_TYPE_ASPRITE }) {
    }

    void *AnimatedSpriteLoader::load(ResourceManager &manager, const ResourcePrototype &proto,
            std::istream &stream, size_t size) const {
        UNUSED(manager);
        UNUSED(proto);
        UNUSED(size);
        Logger::default_logger().debug("Loading material %s", proto.uid.c_str());
        try {
            nlohmann::json json_root = nlohmann::json::parse(stream, nullptr, true, true);

            AnimatedSpriteDef sprite;

            // required attribute
            sprite.base_size.x = json_root.at(KEY_WIDTH);
            // required attribute
            sprite.base_size.y = json_root.at(KEY_HEIGHT);

            if (sprite.base_size.x <= 0 || sprite.base_size.y <= 0) {
                Logger::default_logger().severe("Animated sprite dimensions must be > 0");
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

            // required attribute
            sprite.def_anim = json_root.at(KEY_DEF_ANIM);

            if (json_root.contains(KEY_ATLAS)) {
                sprite.def_atlas = json_root.at(KEY_ATLAS);
            }

            // use oversized intermediate variables to be able to store UINT32_MAX in a signed value
            int64_t tile_width = 0;
            int64_t tile_height = 0;

            if (json_root.contains(KEY_TILE_WIDTH)) {
                if (!json_root.contains(KEY_TILE_HEIGHT)) {
                    Logger::default_logger().severe("Animated sprite specifies tile width but not height");
                    return nullptr;
                }

                tile_width = json_root.at(KEY_TILE_WIDTH);
            }

            if (json_root.contains(KEY_TILE_HEIGHT)) {
                if (!json_root.contains(KEY_TILE_WIDTH)) {
                    Logger::default_logger().severe("Animated sprite specifies tile height but not width");
                    return nullptr;
                }

                tile_height = json_root.at(KEY_TILE_HEIGHT);

                // these values default to zero, so we only want to validate them if they're provided
                if (tile_width <= 0 || tile_height <= 0) {
                    Logger::default_logger().severe("Animated sprite tile dimensions must be >= 0");
                    return nullptr;
                }

                if (tile_width > UINT32_MAX || tile_height > UINT32_MAX) {
                    Logger::default_logger().severe("Animated sprite tile dimensions must be <= UINT32_MAX");
                    return nullptr;
                }
            }

            sprite.tile_size.x = static_cast<unsigned int>(tile_width);
            sprite.tile_size.y = static_cast<unsigned int>(tile_height);

            for (auto &anim_pair : json_root.at(KEY_ANIMS).get<nlohmann::json::object_t>()) {
                auto &anim_id = anim_pair.first;
                auto &anim_json = anim_pair.second;

                SpriteAnimation anim;

                if (anim_json.contains(KEY_ANIM_LOOP)) {
                    anim.loop = anim_json.at(KEY_ANIM_LOOP);
                }

                if (anim_json.contains(KEY_ANIM_ATLAS)) {
                    anim.atlas = anim_json.at(KEY_ANIM_ATLAS);
                }

                if (anim_json.contains(KEY_ANIM_OFF_TYPE)) {
                    auto anim_type_str = anim_json.at(KEY_ANIM_OFF_TYPE);
                    if (anim_type_str == ENUM_OFF_TYPE_ABS) {
                        anim.offset_type = OffsetType::Absolute;
                    } else if (anim_type_str == ENUM_OFF_TYPE_TILE) {
                        anim.offset_type = OffsetType::Tile;
                    } else {
                        Logger::default_logger().severe("Animated sprite offset type is invalid");
                        return nullptr;
                    }
                }

                if (anim_json.contains(KEY_ANIM_DEF_FRAME_DUR)) {
                    anim.def_duration = anim_json.at(KEY_ANIM_DEF_FRAME_DUR);
                    if (anim.def_duration <= 0) {
                        Logger::default_logger().severe("Animated sprite frame duration must be >= 0");
                        return nullptr;
                    }
                }

                if (anim_json.contains(KEY_ANIM_PADDING)) {
                    auto padding_json = anim_json.at(KEY_ANIM_PADDING);

                    if (padding_json.contains(KEY_ANIM_PAD_TOP)) {
                        anim.padding.top = padding_json.at(KEY_ANIM_PAD_TOP);
                    }

                    if (padding_json.contains(KEY_ANIM_PAD_BOTTOM)) {
                        anim.padding.bottom = padding_json.at(KEY_ANIM_PAD_BOTTOM);
                    }

                    if (padding_json.contains(KEY_ANIM_PAD_LEFT)) {
                        anim.padding.left = padding_json.at(KEY_ANIM_PAD_LEFT);
                    }

                    if (padding_json.contains(KEY_ANIM_PAD_RIGHT)) {
                        anim.padding.right = padding_json.at(KEY_ANIM_PAD_RIGHT);
                    }

                    if (anim.padding.top < 0 || anim.padding.bottom < 0
                            || anim.padding.left < 0 || anim.padding.right < 0) {
                        printf("%f, %f, %f, %f\n", anim.padding.top, anim.padding.bottom, anim.padding.left, anim.padding.right);
                        Logger::default_logger().severe("Animated sprite padding values must be >= 0");
                        return nullptr;
                    }
                }

                for (auto &frame_json : anim_json.at(KEY_ANIM_FRAMES).get<nlohmann::json::array_t>()) {
                    AnimationFrame frame;

                    // oversized intermediate variables again
                    int64_t offset_x = frame_json.at(KEY_ANIM_FRAME_X);
                    int64_t offset_y = frame_json.at(KEY_ANIM_FRAME_Y);

                    if (offset_x < 0 || offset_y < 0) {
                        Logger::default_logger().severe("Animated sprite frame offset values must be >= 0");
                        return nullptr;
                    }

                    if (offset_x > UINT32_MAX || offset_y > UINT32_MAX) {
                        Logger::default_logger().severe("Animated sprite frame offset values must be <= UINT32_MAX");
                        return nullptr;
                    }

                    frame.offset.x = static_cast<unsigned int>(offset_x);
                    frame.offset.y = static_cast<unsigned int>(offset_y);

                    if (frame_json.contains(KEY_ANIM_FRAME_DUR)) {
                        frame.duration = frame_json.at(KEY_ANIM_FRAME_DUR);
                        if (frame.duration <= 0) {
                            Logger::default_logger().severe("Animated sprite frame duration must be > 0");
                            return nullptr;
                        }
                    }

                    anim.frames.push_back(frame);
                }

                sprite.animations.insert({ anim_id, anim });
            }

            Logger::default_logger().debug("Successfully loaded animated sprite %s", proto.uid.c_str());

            return new AnimatedSpriteDef(std::move(sprite));
        } catch (nlohmann::detail::parse_error&) {
            Logger::default_logger().severe("Failed to parse animated sprite %s", proto.uid.c_str());
            return nullptr;
        } catch (std::out_of_range&) {
            Logger::default_logger().severe("Animated sprite %s is incomplete or malformed", proto.uid.c_str());
            return nullptr;
        } catch (std::exception &ex) {
            Logger::default_logger().severe("Unspecified exception while parsing animated sprite %s (what: %s)",
                    proto.uid.c_str(), ex.what());
            return nullptr;
        }
    }

    void *AnimatedSpriteLoader::copy(ResourceManager &manager, const ResourcePrototype &proto,
            void *src, std::type_index type) const {
        _ARGUS_ASSERT(type == std::type_index(typeid(AnimatedSprite)),
                "Incorrect pointer type passed to AnimatedSpriteLoader::copy");

        auto &src_sprite = *reinterpret_cast<AnimatedSprite*>(src);

        std::vector<std::string> dep_uids;

        std::map<std::string, const Resource*> deps;
        try {
            deps = load_dependencies(manager, dep_uids);
        } catch (...) {
            Logger::default_logger().warn("Failed to load dependencies for animated sprite %s", proto.uid.c_str());
            return nullptr;
        }

        return new AnimatedSprite(std::move(src_sprite));
    }

    void AnimatedSpriteLoader::unload(void *data_ptr) const {
        delete static_cast<AnimatedSprite*>(data_ptr);
    }
}
