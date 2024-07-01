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

#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/error_util.hpp"
#include "argus/lowlevel/filesystem.hpp"
#include "argus/lowlevel/result.hpp"
#include "argus/lowlevel/threading/future.hpp"
#include "internal/lowlevel/crash.hpp"

#include <filesystem>
#include <future>
#include <stdexcept>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <utility>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>

#define fileno _fileno
#define fseek _fseeki64
#define fstat _fstat64
#define stat _stat64
#define stat_t struct _stat64

#ifndef S_ISDIR
#define S_ISDIR(mode) (mode & S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(mode) (mode & S_IFREG)
#endif
#elif defined __APPLE__
#include <dirent.h>
#include <mach-o/dyld.h>

#define stat_t struct stat
#elif defined __linux__

#include <features.h>

#ifndef __USE_FILE_OFFSET64
#define __USE_FILE_OFFSET64
#endif
#ifndef __USE_LARGEFILE64
#define __USE_LARGEFILE64
#endif
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#ifndef _FILE_OFFSET_BIT
#define _FILE_OFFSET_BIT 64
#endif

#define fopen fopen64
#define fseek fseeko64
#define fstat fstat64
#define stat stat64
#define stat_t struct stat64
#elif defined __NetBSD__ || defined __DragonFly__
#include <dirent.h>
#include <unistd.h>

#define stat_t struct stat
#elif defined __FreeBSD__
#include <dirent.h>
#include <sys/sysctl.h>
#include <sys/types.h>

#define stat_t struct stat
#else
#error "This OS is not supported at this time."
#endif

#define CHUNK_SIZE 4096LU

namespace argus {
    std::string FileOpenError::to_string(void) const {
        return "FileOpenError { "
                "reason = "
                + std::to_string(std::underlying_type_t<FileOpenErrorReason>(reason))
                + ", error_code = "
                + std::to_string(error_code)
                + " }";
    }

    static FileOpenErrorReason _map_file_error(int code) {
        switch (code) {
            case EPERM:
                return FileOpenErrorReason::OperationNotPermitted;
            case EIO:
                return FileOpenErrorReason::IoError;
            case ENXIO:
            case ENODEV:
                return FileOpenErrorReason::NoDevice;
            case EACCES:
                return FileOpenErrorReason::PermissionDenied;
            #ifdef ENOTBLK
            case ENOTBLK:
                return FileOpenErrorReason::NotBlockDevice;
            #endif
            case EBUSY:
            case ETXTBSY:
                return FileOpenErrorReason::Busy;
            case ENOSPC:
                return FileOpenErrorReason::NoSpace;
            case EROFS:
                return FileOpenErrorReason::ReadOnlyFilesystem;
            default:
                return FileOpenErrorReason::Generic;
        }
    }

    FileHandle::FileHandle(std::filesystem::path path, const int mode, const size_t size, void *const handle) :
        m_path(std::move(path)),
        m_mode(mode),
        m_size(size),
        m_handle(handle),
        m_valid(true) {
    }

    //TODO: use the native Windows file API, if available
    Result<FileHandle, FileOpenError> FileHandle::create(const std::filesystem::path &path, const int mode) {
        const char *std_mode;
        // we check for the following invalid cases:
        // - no modes set at all
        // - create set without any other modes
        // - read and create set without any other modes
        if ((mode == 0) || ((mode & FILE_MODE_CREATE) && !(mode & FILE_MODE_WRITE))) {
            crash("Invalid file open mode %d", mode);
        }

        if ((mode & FILE_MODE_READ) && (mode & FILE_MODE_WRITE) && (mode & FILE_MODE_CREATE)) {
            std_mode = "w+";
        } else if ((mode & FILE_MODE_READ) && (mode & FILE_MODE_WRITE)) {
            std_mode = "r+";
        } else if (mode & FILE_MODE_READ) {
            std_mode = "r";
        } else if (mode & FILE_MODE_WRITE) {
            std_mode = "w";
        } else {
            crash("Unable to determine mode string (mode = %d)", mode);
        }

        FILE *file;
        if (mode == (FILE_MODE_READ | FILE_MODE_CREATE)) {
            stat_t stat_buf {};
            int stat_rc = stat(path.string().c_str(), &stat_buf);

            if (stat_rc) {
                // don't return an error yet if it just doesn't exist
                if (errno != ENOENT) {
                    return err<FileHandle, FileOpenError>(_map_file_error(errno), errno);
                }

                // if the file doesn't exist, create it
                #ifdef _WIN32
                FILE *file_tmp;
                fopen_s(&file_tmp, path.string().c_str(), "w");
                #else
                FILE *file_tmp = fopen(path.c_str(), "w");
                #endif
                if (file_tmp == nullptr) {
                    return err<FileHandle, FileOpenError>(_map_file_error(errno), errno);
                }

                fclose(file_tmp);
            }
        }

        #ifdef _WIN32
        fopen_s(&file, path.string().c_str(), std_mode);
        #else
        file = fopen(path.string().c_str(), std_mode);
        #endif

        if (file == nullptr) {
            return err<FileHandle, FileOpenError>(_map_file_error(errno), errno);
        }

        stat_t stat_buf {};
        if (fstat(fileno(file), &stat_buf) != 0) {
            return err<FileHandle, FileOpenError>(_map_file_error(errno), errno);
        }

        assert(stat_buf.st_size >= 0);

        return ok<FileHandle, FileOpenError>(
                FileHandle(path, mode, size_t(stat_buf.st_size), static_cast<void *>(file)));
    }

    const std::filesystem::path &FileHandle::get_path(void) const {
        return m_path;
    }

    size_t FileHandle::get_size(void) const {
        return m_size;
    }

    void FileHandle::release(void) {
        validate_state(m_valid, "Invalid FileHandle");

        this->m_valid = false;
        if (fclose(static_cast<FILE *>(this->m_handle)) != 0) {
            crash("fclose failed: %d", errno);
        }
    }

    void FileHandle::remove(void) {
        this->release();
        ::remove(this->m_path.string().c_str());
    }

    Result<void, FileOpenError> FileHandle::to_istream(const off_t offset, std::ifstream &target) const {
        validate_state(m_valid, "Invalid FileHandle");

        fseek(static_cast<FILE *>(m_handle), offset, SEEK_SET);

        std::ios::openmode omode = std::ios::binary;
        if (this->m_mode & FILE_MODE_READ) {
            omode |= std::ios::in;
        }
        if (this->m_mode & FILE_MODE_WRITE) {
            omode |= std::ios::out;
        }

        target.open(m_path, omode);

        if (!target.good()) {
            return err<void, FileOpenError>(FileOpenErrorReason::Generic, -1);
        }

        //stream->seekg(offset);
        return ok<void, FileOpenError>();
    }

    Result<void, int> FileHandle::read(off_t offset, size_t read_size, unsigned char *buf) const {
        validate_state(m_valid, "Non-valid FileHandle");

        validate_arg(read_size > 0, "Invalid size parameter");

        validate_arg(offset >= 0, "Read offset must be non-negative");

        validate_arg(size_t(offset) +read_size <= this->m_size, "Invalid offset/size combination");

        fseek(static_cast<FILE *>(m_handle), off_t(offset), SEEK_SET);

        size_t read_chunks = fread(buf, read_size, 1, static_cast<FILE *>(m_handle));

        if (read_chunks == 1) {
            return ok<void, int>();
        } else {
            return err<void, int>(errno);
        }
    }

    Result<void, int> FileHandle::write(const off_t offset, const size_t write_size, unsigned char *const buf) {
        validate_state(m_valid, "Non-valid FileHandle");

        validate_arg(write_size > 0, "Invalid size parameter");

        validate_arg(offset >= -1, "Invalid offset parameter");

        if (offset == -1) {
            fseek(static_cast<FILE *>(m_handle), 0, SEEK_END);
        } else {
            fseek(static_cast<FILE *>(m_handle), offset, SEEK_SET);
        }

        size_t write_chunks = fwrite(buf, write_size, 1, static_cast<FILE *>(m_handle));
        validate_syscall(write_chunks == 1, "fwrite");

        stat_t file_stat {};
        if (fstat(fileno(static_cast<FILE *>(m_handle)), &file_stat) != 0) {
            return err<void, int>(errno);
        }

        assert(file_stat.st_size >= 0);

        this->m_size = size_t(file_stat.st_size);

        return ok<void, int>();
    }

    std::future<Result<void, int>> FileHandle::read_async(const off_t offset, const size_t read_size,
            unsigned char *const buf, const std::function<void(FileHandle &)> &callback) {
        validate_state(m_valid, "Non-valid FileHandle");

        validate_arg(callback != nullptr, "Callback must be present");
        validate_arg(read_size > 0, "Invalid size parameter");
        validate_arg(offset >= 0, "Read offset must be non-negative");
        validate_arg(size_t(offset) +read_size <= this->m_size, "Invalid offset/size combination");

        return make_future<void, int>([this, offset, buf] {
            return read(offset, m_size, buf);
        }, [callback, this] (const auto &res) {
            if (res.is_ok()) {
                callback(*this);
            } else {
                crash_ll("Async read from file failed with error code %d", res.unwrap_err());
            }
        });
    }

    std::future<Result<void, int>> FileHandle::write_async(const off_t offset, const size_t write_size,
            unsigned char *const buf, const std::function<void(FileHandle &)> &callback) {
        validate_state(m_valid, "Non-valid FileHandle");

        validate_arg(callback != nullptr, "Callback must be present");
        validate_arg(write_size > 0, "Invalid size parameter");
        validate_arg(offset >= -1, "Invalid offset parameter");

        return make_future<void, int>([this, offset, write_size, buf] { return write(offset, write_size, buf); },
                [callback, this] (const auto &res) {
                    if (res.is_ok()) {
                        callback(*this);
                    } else {
                        crash_ll("Async write to file failed with error code %d", res.unwrap_err());
                    }
                });
    }
}
