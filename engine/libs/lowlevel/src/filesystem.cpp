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

#include "argus/lowlevel/error_util.hpp"
#include "argus/lowlevel/filesystem.hpp"
#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/threading/future.hpp"

#include <filesystem>
#include <future>
#include <stdexcept>

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <utility>
#include <sys/stat.h>

#ifdef _WIN32
#include <Windows.h>

#define fileno _fileno
#define fseek _fseeki64
#define fstat _fstat64
#define stat _stat64
#define stat_t struct _stat64

#define S_ISDIR(mode) (mode & S_IFDIR)
#define S_ISREG(mode) (mode & S_IFREG)
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

    FileHandle::FileHandle(std::filesystem::path path, const int mode, const size_t size, void *const handle) :
            path(std::move(path)),
            mode(mode),
            size(size),
            handle(handle),
            valid(true) {
    }

    //TODO: use the native Windows file API, if available
    FileHandle FileHandle::create(const std::filesystem::path &path, const int mode) {
        const char *std_mode;
        // we check for the following invalid cases:
        // - no modes set at all
        // - create set without any other modes
        // - read and create set without any other modes
        validate_arg_not((mode == 0) || ((mode & FILE_MODE_CREATE) && !(mode & FILE_MODE_WRITE)), "Invalid mode");

        if ((mode & FILE_MODE_READ) && (mode & FILE_MODE_WRITE) && (mode & FILE_MODE_CREATE)) {
            std_mode = "w+";
        } else if ((mode & FILE_MODE_READ) && (mode & FILE_MODE_WRITE)) {
            std_mode = "r+";
        } else if (mode & FILE_MODE_READ) {
            std_mode = "r";
        } else if (mode & FILE_MODE_WRITE) {
            std_mode = "w";
        } else {
            Logger::default_logger().fatal("Unable to determine mode string");
        }

        FILE *file;
        if (mode == (FILE_MODE_READ | FILE_MODE_CREATE)) {
            stat_t stat_buf{};
            int stat_rc = stat(path.string().c_str(), &stat_buf);

            if (stat_rc) {
                // throw the error directly if it's not ENOENT (file not found)
                validate_syscall(errno == ENOENT, "stat");

                // if the file doesn't exist, create it
                #ifdef _WIN32
                FILE *file_tmp;
                fopen_s(&file_tmp, path.string().c_str(), "w");
                #else
                FILE *file_tmp = fopen(path.c_str(), "w");
                #endif
                validate_syscall(file_tmp, "fopen");

                fclose(file_tmp);
            }
        }

        #ifdef _WIN32
        fopen_s(&file, path.string().c_str(), std_mode);
        #else
        file = fopen(path.string().c_str(), std_mode);
        #endif

        validate_syscall(file != nullptr, "fopen");

        stat_t stat_buf{};
        validate_syscall(fstat(fileno(file), &stat_buf), "fstat");

        assert(stat_buf.st_size >= 0);

        return FileHandle(path, mode, size_t(stat_buf.st_size), static_cast<void *>(file));
    }

    const std::filesystem::path &FileHandle::get_path(void) const {
        return path;
    }

    size_t FileHandle::get_size(void) const {
        return size;
    }

    void FileHandle::release(void) {
        validate_state(this->valid, "Non-valid FileHandle");

        this->valid = false;
        validate_syscall(fclose(static_cast<FILE *>(this->handle)), "fclose");
    }

    void FileHandle::remove(void) {
        this->release();
        ::remove(this->path.string().c_str());
    }

    void FileHandle::to_istream(const off_t offset, std::ifstream &target) const {
        validate_state(valid, "Non-valid FileHandle");

        fseek(static_cast<FILE *>(handle), offset, SEEK_SET);

        std::ios::openmode omode = std::ios::binary;
        if (this->mode & FILE_MODE_READ) {
            omode |= std::ios::in;
        }
        if (this->mode & FILE_MODE_WRITE) {
            omode |= std::ios::out;
        }

        target.open(path, omode);

        if (!target.good()) {
            throw std::runtime_error("Failed to create file stream");
        }

        //stream->seekg(offset);
    }

    void FileHandle::read(off_t offset, size_t read_size, unsigned char *buf) const {
        validate_state(valid, "Non-valid FileHandle");

        validate_arg(read_size > 0, "Invalid size parameter");

        validate_arg(offset >= 0, "Read offset must be non-negative");

        validate_arg(size_t(offset) +read_size <= this->size, "Invalid offset/size combination");

        fseek(static_cast<FILE *>(handle), off_t(offset), SEEK_SET);

        size_t read_chunks = fread(buf, read_size, 1, static_cast<FILE *>(handle));

        validate_syscall(read_chunks == 1, "fread");
    }

    void FileHandle::write(const off_t offset, const size_t write_size, unsigned char *const buf) {
        validate_state(valid, "Non-valid FileHandle");

        validate_arg(write_size > 0, "Invalid size parameter");

        validate_arg(offset >= -1, "Invalid offset parameter");

        if (offset == -1) {
            fseek(static_cast<FILE *>(handle), 0, SEEK_END);
        } else {
            fseek(static_cast<FILE *>(handle), offset, SEEK_SET);
        }

        size_t write_chunks = fwrite(buf, write_size, 1, static_cast<FILE *>(handle));
        validate_syscall(write_chunks == 1, "fwrite");

        stat_t file_stat{};
        validate_syscall(fstat(fileno(static_cast<FILE *>(handle)), &file_stat), "fstat");

        assert(file_stat.st_size >= 0);

        this->size = size_t(file_stat.st_size);
    }

    std::future<void> FileHandle::read_async(const off_t offset, const size_t read_size, unsigned char *const buf,
            const std::function<void(FileHandle &)> callback) {
        validate_state(valid, "Non-valid FileHandle");

        validate_arg(read_size > 0, "Invalid size parameter");

        validate_arg(offset >= 0, "Read offset must be non-negative");

        validate_arg(size_t(offset) +read_size <= this->size, "Invalid offset/size combintation");

        return make_future([this, offset, buf] { read(offset, size, buf); }, std::bind(callback, *this));
    }

    std::future<void> FileHandle::write_async(const off_t offset, const size_t write_size,
            unsigned char *const buf, std::function<void(FileHandle &)> callback) {
        validate_state(valid, "Non-valid FileHandle");

        validate_arg(write_size > 0, "Invalid size parameter");

        validate_arg(offset >= -1, "Invalid offset parameter");

        return make_future([this, offset, write_size, buf] { write(offset, write_size, buf); },
                std::bind(callback, *this));
    }
}
