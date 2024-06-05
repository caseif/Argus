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

#pragma once

#include <functional>
#include <string>
#include <utility>

#include <cstdio>
#include <cstdlib>
#include <cstdarg>

namespace argus {
    class Logger {
      private:
        FILE *m_target;
        const std::string m_realm;

        void log(const std::string &level, const char *format, va_list args) const;

        void log_error(const std::string &level, const char *format, va_list args) const;

        void log_va(const std::string &level, const char *format, ...) const;

        void log_error_va(const std::string &level, const char *format, ...) const;

        void debug_va(const char *format, ...) const;

        void info_va(const char *format, ...) const;

        void warn_va(const char *format, ...) const;

        void severe_va(const char *format, ...) const;

        [[noreturn]] void fatal_va(const std::function<void(void)> &deinit, const char *format, ...) const;

        [[noreturn]] void fatal_va(const char *format, ...) const;

      public:
        static Logger &default_logger(void);

        Logger(FILE *target, std::string realm);

        Logger(std::string realm);

        template<typename... Args>
        void log(const std::string &level, const std::string &format, Args... args) const {
            log_va(level, format.c_str(), args...);
        }

        template<typename... Args>
        void log_error(const std::string &level, const std::string &format, Args... args) const {
            log_error_va(level, format.c_str(), args...);
        }

        template<typename... Args>
        void debug(const std::string &format, Args... args) const {
            debug_va(format.c_str(), args...);
        }

        template<typename... Args>
        void info(const std::string &format, Args... args) const {
            info_va(format.c_str(), args...);
        }

        template<typename... Args>
        void warn(const std::string &format, Args... args) const {
            warn_va(format.c_str(), args...);
        }

        template<typename... Args>
        void severe(const std::string &format, Args... args) const {
            severe_va(format.c_str(), args...);
        }

        template<typename... Args>
        [[noreturn]] void fatal(const std::function<void(void)> &deinit, const std::string &format,
                Args... args) const {
            fatal_va(deinit, format.c_str(), args...);
        }

        template<typename... Args>
        [[noreturn]] void fatal(const std::string &format, Args... args) const {
            fatal_va(format.c_str(), args...);
        }
    };
}
