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

#include "argus/scripting/exception.hpp"

#include <exception>

namespace argus {
    BindingException::BindingException(const std::string &name, const std::string &what):
        m_msg("Unable to bind " + name + ": " + what) {
    }

    const char *BindingException::what(void) const noexcept {
        return m_msg.c_str();
    }

    TypeNotBoundException::TypeNotBoundException(std::string fn_name):
        m_fn_name(std::move(fn_name)) {
    }

    const char *TypeNotBoundException::what(void) const noexcept {
        return m_fn_name.c_str();
    }

    SymbolNotBoundException::SymbolNotBoundException(std::string name):
        m_name(std::move(name)) {
    }

    const char *SymbolNotBoundException::what(void) const noexcept {
        return m_name.c_str();
    }

    ReflectiveArgumentsException::ReflectiveArgumentsException(std::string reason):
        m_reason(std::move(reason)) {
    }

    const char *ReflectiveArgumentsException::what(void) const noexcept {
        return m_reason.c_str();
    }

    ScriptLoadException::ScriptLoadException(const std::string &script_uid, const std::string &msg) noexcept:
        m_msg("Load failed for " + script_uid + ": " + msg) {
    }

    const char *ScriptLoadException::what(void) const noexcept {
        return m_msg.c_str();
    }

    ScriptInvocationException::ScriptInvocationException(const std::string &fn_name, const std::string &msg):
        m_msg("Invocation failed for script function " + fn_name + ": " + msg) {
    }

    const char *ScriptInvocationException::what(void) const noexcept {
        return m_msg.c_str();
    }
}
