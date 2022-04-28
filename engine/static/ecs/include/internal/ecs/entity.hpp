#pragma once

#include "argus/ecs/entity.hpp"

#include <mutex>
#include <vector>

namespace argus {
    extern std::vector<const Entity*> g_created_entities;
    extern std::vector<EntityId> g_destroyed_entities;

    extern std::mutex g_entity_changes_mutex;
}
