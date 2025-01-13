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

#include "argus/core/cabi/event.h"
#include "argus/core/event.hpp"

class WrappedEvent : argus::ArgusEvent {
  private:
    void *m_handle;
    void(*m_destructor)(void *);

  public:
    WrappedEvent(const char *type_id, void *handle, void(*destructor)(void *)):
        argus::ArgusEvent(type_id),
        m_handle(handle),
        m_destructor(destructor) {
    }

    ~WrappedEvent() override {
        m_destructor(m_handle);
    }

    WrappedEvent(WrappedEvent &) = delete;
    WrappedEvent(WrappedEvent &&) = delete;
    WrappedEvent &operator=(WrappedEvent &) = delete;
    WrappedEvent &&operator=(WrappedEvent &&) = delete;

    const void *get_handle() const {
        return m_handle;
    }

    void *get_handle() {
        return m_handle;
    }
};

extern "C" {

const char *argus_event_get_type_id(argus_event_const_t event) {
    return reinterpret_cast<const argus::ArgusEvent *>(event)->type_id.c_str();
}

Index argus_register_event_handler(const char *type_id, argus_event_handler_t handler,
        TargetThread target_thread, void *data, Ordering ordering,
        argus::ArgusEventHandlerUnregisterCallback unregister_callback) {
    auto index = argus::register_event_handler_with_type(type_id,
            [handler](const argus::ArgusEvent &event, void *data) {
                handler(reinterpret_cast<const void *>(&event), data);
            }, argus::TargetThread(target_thread), data, argus::Ordering(ordering), unregister_callback);
    return index;
}

void argus_unregister_event_handler(Index index) {
    argus::unregister_event_handler(index);
}

void argus_dispatch_event(const char *type_id, argus_event_t event, void(*destructor)(void *)) {
    auto *wrapped = new WrappedEvent(type_id, event, destructor);
    argus::_dispatch_event_ptr(*reinterpret_cast<argus::ArgusEvent *>(wrapped));
}

const void *argus_unwrap_event(const void *event) {
    return reinterpret_cast<const WrappedEvent *>(event)->get_handle();
}

void *argus_unwrap_event_mut(void *event) {
    return reinterpret_cast<WrappedEvent *>(event)->get_handle();
}

}
