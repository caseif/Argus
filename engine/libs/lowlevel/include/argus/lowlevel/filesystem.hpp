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

/**
 * @file argus/lowlevel/filesystem.hpp
 *
 * Low-level filesystem API.
 */

#pragma once

#include "argus/lowlevel/result.hpp"

#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <string>
#include <utility>
#include <vector>

#include <cstdio>

#ifdef _WIN32
#define ssize_t signed __int64
#else

#include <sys/types.h>

#endif

/**
 * @brief The separator between a file's name and extension.
 */
#define EXTENSION_SEPARATOR "."
#define EXTENSION_SEPARATOR_CHAR '.'

/**
 * @brief File mode mask denoting read access.
 */
#define FILE_MODE_READ 1
/**
 * @brief File mode mask denoting write access.
 */
#define FILE_MODE_WRITE 2
/**
 * @brief File mode mask denoting the file should be created if necessary.
 */
#define FILE_MODE_CREATE 4

namespace argus {
    enum class FileOpenErrorReason {
        OperationNotPermitted,
        NotFound,
        PermissionDenied,
        Busy,
        NotBlockDevice,
        NoDevice,
        IoError,
        NoSpace,
        ReadOnlyFilesystem,
        Generic,
    };

    struct FileOpenError {
        FileOpenErrorReason reason;
        int error_code;
    };

    /**
     * @brief Represents a handle to a file on the disk.
     *
     * A FileHandle may be used to create, read from, and write to files on the
     * disk in a high-level manner. Additionally, it provides a high-level
     * interface for asynchronous file I/O.
     */
    class FileHandle {
      private:
        std::filesystem::path m_path;
        int m_mode;
        size_t m_size;
        void *m_handle;

        bool m_valid;

        FileHandle(std::filesystem::path path, int mode, size_t size, void *handle);

      public:
        /**
         * @brief Creates a handle to the file at the given path.
         *
         * If the file does not yet exist, it will be created as an empty
         * file.
         *
         * @param path The path of the file to obtain a handle to.
         * @param mode The mode to open the file with.
         *
         * @return The created FileHandle.
         */
        static Result<FileHandle, FileOpenError> create(const std::filesystem::path &path, int mode);

        /**
         * @brief Gets the path of the file referenced by this handle.
         *
         * @return The path of the file referenced by this handle.
         */
        [[nodiscard]] const std::filesystem::path &get_path(void) const;

        /**
         * @brief Gets the size of the file referenced by this handle.
         *
         * If the file did not exist prior to the handle being opened, this
         * function will return `0`.
         *
         * @return The size of the file referenced by this handle.
         */
        [[nodiscard]] size_t get_size(void) const;

        /**
         * @brief Releases the file handle.
         *
         * @attention The handle will thereafter be invalidated and thus
         *            ineligible for further use.
         */
        void release(void);

        /**
         * @brief Removes the file referenced by the handle.
         *
         * @attention This operation implicitly releases the handle,
         *            invalidating it.
         *
         * @sa FileHandle::release
         */
        void remove(void);

        /**
         * @brief Creates an std::istream from the file handle.
         *
         * @param offset The offset in bytes at which to open the
         *        std::istream.
         * @param target The object to use when opening the stream.
         */
        Result<void, FileOpenError> to_istream(off_t offset, std::ifstream &target) const;

        /**
         * @brief Reads data from the file referenced by the handle.
         *
         * @param offset The offset in bytes from which to begin reading.
         * @param read_size The number of bytes to read.
         * @param buf The buffer to store data into. This buffer _must_ be
         *            at least `size` bytes in length to avoid a buffer
         *            overflow.
         */
        void read(off_t offset, size_t read_size, unsigned char *buf) const;

        /**
         * @brief Writes data into the file referenced by the handle.
         *
         * @param offset The offset in bytes at which to begin writing, or
         *        `-1` to append to the end of the file.
         * @param write_size The number of bytes to write.
         * @param buf A buffer containing the data to be written. This
         *            buffer _must_ be at least `size` bytes in length to
         *            avoid a buffer over-read.
         */
        void write(off_t offset, size_t write_size, unsigned char *buf);

        /**
         * @brief Reads data from the file referenced by this handle
         *        asynchronously.
         *
         * @param offset The offset in bytes from which to begin reading.
         * @param read_size The number of bytes to read.
         * @param buf The buffer to store data into. This buffer _must_ be
         *        at least `size` bytes in length to avoid a buffer
         *        overflow.
         * @param callback The callback to execute upon completion of the
         *        read operation. The function handle may be empty. Note
         *        that this callback is not guaranteed to run (e.g. if the
         *        read operation generates an exception), so it should not
         *        contain any critical code.
         *
         * @return A std::future which will be completed after the data has
         *         been fully read, or after the read operation generates an
         *         exception.
         *
         * @attention Any errors returned the read operation itself (not
         *            including parameter validation) are exposed through the
         *            returned std::future.
         *
         * @sa FileHandle::read
         */
        std::future<void> read_async(off_t offset, size_t read_size,
                unsigned char *buf, const std::function<void(FileHandle &)> &callback);

        /**
         * @brief Writes data into the file referenced by the handle
         *        asynchronously.
         *
         * @param offset The offset in bytes at which to begin writing, or
         *        `-1` to append to the end of the file.
         * @param write_size The number of bytes to write.
         * @param buf A buffer containing the data to be written. This
         *            buffer _must_ be at least `size` bytes in length to
         *            avoid a buffer overflow.
         * @param callback The callback to execute upon completion of the
         *        write operation. The function handle may be empty. Note
         *        that this callback is not guaranteed to run (e.g. if the
         *        write operation generates an exception), so it should not
         *        contain any critical code.
         *
         * @return A std::future which will be completed after the data has
         *         been fully written, or after the write operation
         *         generates an exception.
         *
         * @attention Any errors returned by the read operation itself (not
         *            including parameter validation) are exposed through the
         *            returned std::future.
         *
         * @sa FileHandle::write
         */
        std::future<void> write_async(off_t offset, size_t write_size,
                unsigned char *buf, const std::function<void(FileHandle &)> &callback);
    };
}
