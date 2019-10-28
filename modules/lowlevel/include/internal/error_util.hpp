#pragma once

#include <stdexcept>
#include <system_error>

#ifdef _WIN32
    #include <Windows.h>

    #ifndef errno
    #define errno WSAGetLastError()
    #endif
#endif

#define validate_arg(cond, what) _validate_arg(cond, __func__, what)
#define validate_arg_not(cond, what) validate_arg(!(cond), what)
#define throw_errno(syscall) _throw_errno(__func__, syscall)
#define validate_syscall(cond, syscall) _validate_syscall(cond, __func__, syscall)

inline void _validate_arg(bool cond, const std::string &caller, const std::string &what) {
    if (!cond) {
        throw std::invalid_argument(caller + ": " + what);
    }
}

inline void _throw_errno(const std::string &caller, const std::string &syscall) {
    throw std::system_error(errno, std::generic_category(), caller + ": " + syscall + " failed");
}

inline void _validate_syscall(bool cond, const std::string &caller, const std::string &syscall) {
    if (!cond) {
        _throw_errno(caller, syscall);
    }
}

inline void _validate_syscall(int rc, const std::string &caller, const std::string &syscall) {
    _validate_syscall(rc == 0, caller, syscall);
}
