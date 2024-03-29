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

#pragma once

#include <exception>
#include <string>

namespace argus {
      class BindingException : public std::exception {
        private:
          std::string msg;

        public:
          BindingException(const std::string &name, const std::string &what);

          [[nodiscard]] const char *what(void) const noexcept override;
      };

    class TypeNotBoundException : public std::exception {
      private:
        std::string fn_name;

      public:
        TypeNotBoundException(std::string fn_name);

        [[nodiscard]] const char *what(void) const noexcept override;
    };

    class SymbolNotBoundException : public std::exception {
      private:
        std::string name;

      public:
        SymbolNotBoundException(std::string name);

        [[nodiscard]] const char *what(void) const noexcept override;
    };

    class ReflectiveArgumentsException : public std::exception {
      private:
        std::string m_reason;

      public:
        ReflectiveArgumentsException(std::string reason);

        [[nodiscard]] const char *what(void) const noexcept override;
    };

    class ScriptLoadException : public std::exception {
      private:
        std::string msg;
      public:
        ScriptLoadException(const std::string &script_uid, const std::string &msg);

        [[nodiscard]] const char *what(void) const noexcept override;
    };

    class ScriptInvocationException : public std::exception {
      private:
        std::string msg;
      public:
        ScriptInvocationException(const std::string &fn_name, const std::string &msg);

        [[nodiscard]] const char *what(void) const noexcept override;
    };
}
