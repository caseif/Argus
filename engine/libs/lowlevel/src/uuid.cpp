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

#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/uuid.hpp"

#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <rpcdce.h>
#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFUUID.h>
#elif defined(__linux__)
#include <uuid/uuid.h>
#else
#error "This OS is not supported at this time."
#endif

namespace argus {
    #ifdef _WIN32
    Uuid Uuid::random(void) {
        UUID uuid;
        auto res = UuidCreate(&uuid);
        if (!(res == RPC_S_OK || res == RPC_S_UUID_LOCAL_ONLY)) {
            Logger::default_logger().fatal("Failed to generate UUID");
        }

        argus::Uuid final;
        std::memcpy(&final.data[0], &uuid.Data1, sizeof(uuid.Data1));
        std::memcpy(&final.data[4], &uuid.Data2, sizeof(uuid.Data2));
        std::memcpy(&final.data[6], &uuid.Data3, sizeof(uuid.Data3));
        std::memcpy(&final.data[8], &uuid.Data4, sizeof(uuid.Data4));

        return final;
    }
    #elif defined(__APPLE__)
    Uuid Uuid::random(void) {
        auto uuid_ref = CFUUIDCreate(nullptr);
        auto uuid_bytes = CFUUIDGetUUIDBytes(uuid_ref);

        argus::Uuid final;
        std::memcpy(final.data, uuid_bytes, sizeof(final.data));

        return final;
    }
    #elif defined(__linux__)
    Uuid Uuid::random(void) {
        uuid_t uuid;
        uuid_generate(uuid);

        argus::Uuid final;
        memcpy(final.data, uuid, sizeof(final.data));

        return final;
    }
    #else
    #error "This OS is not supported at this time."
    #endif

    bool Uuid::operator==(const Uuid &rhs) const {
        return std::memcmp(this->data, rhs.data, sizeof(this->data)) == 0;
    }

    bool Uuid::operator<(const Uuid &rhs) const {
        auto *lhs_hi = reinterpret_cast<const uint64_t*>(this->data);
        auto *rhs_hi = reinterpret_cast<const uint64_t*>(rhs.data);
        if (*lhs_hi < *rhs_hi) {
            return true;
        } else if (*lhs_hi > *rhs_hi) {
            return false;
        } else { // lhs_hi == rhs_hi
            auto *lhs_lo = reinterpret_cast<const uint64_t*>(this->data + 8);
            auto *rhs_lo = reinterpret_cast<const uint64_t*>(rhs.data + 8);
            return *lhs_lo < *rhs_lo;
        }
    }

    bool Uuid::operator>(const Uuid &rhs) const {
        return rhs < *this;
    }

    Uuid::operator const std::string() const {
        return this->to_string();
    }

    std::string Uuid::to_string(void) const {
        std::string rendered(37, '\0');
        snprintf(rendered.data(), rendered.size(), "%08x-%04x-%04x-%04x-%08x%04x",
                *reinterpret_cast<const uint32_t*>(&data[0]),
                *reinterpret_cast<const uint16_t*>(&data[4]),
                *reinterpret_cast<const uint16_t*>(&data[6]),
                *reinterpret_cast<const uint16_t*>(&data[8]),
                *reinterpret_cast<const uint32_t*>(&data[10]),
                *reinterpret_cast<const uint16_t*>(&data[14]));
        return rendered;
    }
}
