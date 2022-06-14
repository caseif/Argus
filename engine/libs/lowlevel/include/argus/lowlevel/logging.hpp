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
            FILE *target;
            const std::string realm;

            void log(const std::string &level, std::string format, va_list args) const;

            void log_error(const std::string &level, std::string format, va_list args) const;
        public:
            static Logger &default_logger(void);

            Logger(FILE *target, const std::string &realm);

            Logger(const std::string &realm);

            void log(const std::string &level, std::string format, ...) const;

            void log_error(const std::string &level, std::string format, ...) const;

            void debug(std::string format, ...) const;

            void info(std::string format, ...) const;

            void warn(std::string format, ...) const;

            [[noreturn]] void fatal(std::function<void(void)> deinit, std::string format, ...) const;

            [[noreturn]] void fatal(std::string format, ...) const;
    };
}
