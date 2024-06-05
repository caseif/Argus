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

#include "argus/lowlevel/logging.hpp"

#include <string>

#include <cstdio>

#define LOG_LEVEL_FATAL "FATAL"
#define LOG_LEVEL_SEVERE "SEVERE"
#define LOG_LEVEL_WARN "WARN"
#define LOG_LEVEL_INFO "INFO"
#define LOG_LEVEL_DEBUG "DEBUG"

#define DEFAULT_REALM "Argus"

#ifdef _ARGUS_DEBUG_MODE
#ifdef _WIN32
#include <intrin.h>
#define DEBUG_BREAK() __debugbreak()
#else

#include <csignal>
#include <utility>

#define DEBUG_BREAK() raise(SIGTRAP)
#endif
#else
#define DEBUG_BREAK()
#endif

#define _ABORT()          DEBUG_BREAK(); \
                          exit(1)

namespace argus {
    static Logger g_default_logger(DEFAULT_REALM);

    static void
    _log_generic(FILE *stream, const std::string &realm, const std::string &level, const char *format, va_list args) {
        fprintf(stream, "[%s][%s] ", realm.c_str(), level.c_str());
        va_list args_copy;
        va_copy(args_copy, args);
        vfprintf(stream, format, args_copy);
        va_end(args_copy);
        fprintf(stream, "\n");
        fflush(stream);
    }

    Logger &Logger::default_logger(void) {
        return g_default_logger;
    }

    Logger::Logger(FILE *target, std::string realm):
        m_target(target),
        m_realm(std::move(realm)) {
    }

    Logger::Logger(std::string realm):
        m_target(nullptr),
        m_realm(std::move(realm)) {
    }

    void Logger::log(const std::string &level, const char *format, va_list args) const {
        _log_generic(m_target != nullptr ? m_target : stdout, m_realm, level, format, args);
        if (m_target != nullptr && m_target != stdout) {
            _log_generic(m_target != nullptr ? m_target : stdout, m_realm, level, format, args);
        }
    }

    void Logger::log_va(const std::string &level, const char *format, ...) const {
        va_list args;
        va_start(args, format);
        log(level, format, args);
        va_end(args);
    }

    void Logger::log_error(const std::string &level, const char *format, va_list args) const {
        _log_generic(m_target != nullptr ? m_target : stderr, m_realm, level, format, args);
    }

    void Logger::log_error_va(const std::string &level, const char *format, ...) const {
        va_list args;
        va_start(args, format);
        log_error(level, format, args);
        va_end(args);
    }

    void Logger::debug_va(const char *format, ...) const {
        #ifdef _ARGUS_DEBUG_MODE
        va_list args;
        va_start(args, format);
        log(LOG_LEVEL_DEBUG, format, args);
        va_end(args);
        #else
        UNUSED(format);
        #endif
    }

    void Logger::info_va(const char *format, ...) const {
        va_list args;
        va_start(args, format);
        log(LOG_LEVEL_INFO, format, args);
        va_end(args);
    }

    void Logger::warn_va(const char *format, ...) const {
        va_list args;
        va_start(args, format);
        log_error(LOG_LEVEL_WARN, format, args);
        va_end(args);
    }

    void Logger::severe_va(const char *format, ...) const {
        va_list args;
        va_start(args, format);
        log_error(LOG_LEVEL_SEVERE, format, args);
        va_end(args);
    }

    [[noreturn]] void Logger::fatal_va(const std::function<void(void)> &deinit, const char *format, ...) const {
        va_list args;
        va_start(args, format);
        log_error(LOG_LEVEL_FATAL, format, args);
        va_end(args);

        if (deinit) {
            deinit();
        }

        _ABORT();
    }

    [[noreturn]] void Logger::fatal_va(const char *format, ...) const {
        fatal(std::function<void(void)>(), format);
    }
}
