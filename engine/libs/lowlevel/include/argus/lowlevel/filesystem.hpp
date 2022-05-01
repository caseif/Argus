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

/**
 * \file argus/lowlevel/filesystem.hpp
 *
 * Low-level filesystem API.
 */

#pragma once

#include <fstream>
#include <functional>
#include <future>
#include <string>
#include <utility>
#include <vector>

#include <cstdio>

#ifdef _WIN32
#define ssize_t signed __int64

/**
 * \brief The separator for elements in filesystem paths.
 */
#define PATH_SEPARATOR "\\"
#else
#include <sys/types.h>

/**
 * \brief The separator for elements in filesystem paths.
 */
#define PATH_SEPARATOR "/"
#endif

/**
 * \brief The separator between a file's name and extension.
 */
#define EXTENSION_SEPARATOR '.'

/**
 * \brief File mode mask denoting read access.
 */
#define FILE_MODE_READ 1
/**
 * \brief File mode mask denoting write access.
 */
#define FILE_MODE_WRITE 2
/**
 * \brief File mode mask denoting the file should be created if necessary.
 */
#define FILE_MODE_CREATE 4

namespace argus {

    /**
     * \brief Represents a handle to a file on the disk.
     *
     * A FileHandle may be used to create, read from, and write to files on the
     * disk in a high-level manner. Additionally, it provides a high-level
     * interface for asynchronous file I/O.
     */
    class FileHandle {
        private:
            std::string path;
            int mode;
            size_t size;
            void *handle;

            bool valid;

            FileHandle(const std::string &path, int mode, size_t size, void *handle);

        public:
            /**
             * \brief Creates a handle to the file at the given path.
             *
             * If the file does not yet exist, it will be created as an empty
             * file.
             *
             * \param path The path of the file to obtain a handle to.
             * \param mode The mode to open the file with.
             *
             * \return The created FileHandle.
             *
             * \throw std::invalid_argument If the given mode is invalid. This
             *        occurs if no mode is set, or if CREATE is set without
             *        WRITE.
             * \throw std::system_error If an error occurs while opening or
             *        creating the file.
             */
            static FileHandle create(const std::string &path, int mode);

            /**
             * \brief Gets the absolute path of the file referenced by this
             *        handle.
             *
             * \return The absolute path of the file referenced by this handle.
             */
            const std::string &get_path(void) const;

            /**
             * \brief Gets the size of the file referenced by this handle.
             *
             * If the file did not exist prior to the handle being opened, this
             * function will return `0`.
             *
             * \return The size of the file referenced by this handle.
             */
            size_t get_size(void) const;

            /**
             * \brief Releases the file handle.
             *
             * \attention The handle will thereafter be invalidated and thus
             *            ineligible for further use.
             *
             * \throw std::runtime_error If the handle is not valid.
             * \throw std::system_error If an error occurs while closing the
             *        file.
             */
            void release(void);

            /**
             * \brief Removes the file referenced by the handle.
             *
             * \attention This operation implicitly releases the handle,
             *            invalidating it.
             *
             * \throw std::runtime_error If the handle is not valid.
             * \throw std::system_error If an error occurs while removing the
             *        file or closing its handle.
             *
             * \sa FileHandle::release
             */
            void remove(void);

            /**
             * \brief Creates an std::istream from the file handle.
             *
             * \param offset The offset in bytes at which to open the
             *        std::istream.
             * \param target The object to use when opening the stream.
             *
             * \throw std::runtime_error If the handle is not valid.
             */
            void to_istream(off_t offset, std::ifstream &target) const;

            /**
             * \brief Reads data from the file referenced by the handle.
             *
             * \param offset The offset in bytes from which to begin reading.
             * \param read_size The number of bytes to read.
             * \param buf The buffer to store data into. This buffer _must_ be
             *            at least `size` bytes in length to avoid a buffer
             *            overflow.
             *
             * \throw std::runtime_error If the handle is not valid.
             * \throw std::invalid_argument If the current mode does not support
             *        reading, or if `size` or `offset` are nonsensical
             *        (individiually or in conjunction).
             * \throw std::system_error If an error occurs while reading from
             *        the file.
             */
            void read(off_t offset, size_t read_size, unsigned char *buf) const;

            /**
             * \brief Writes data into the file referenced by the handle.
             *
             * \param offset The offset in bytes at which to begin writing, or
             *        `-1` to append to the end of the file.
             * \param write_size The number of bytes to write.
             * \param buf A buffer containing the data to be written. This
             *            buffer _must_ be at least `size` bytes in length to
             *            avoid a buffer over-read.
             *
             * \throw std::runtime_error If the handle is not valid.
             * \throw std::invalid_argument If the current mode does not support
             *        writing, or if `size` or `offset` are nonsensical
             *        (individiually or in conjunction).
             * \throw std::system_error If an error occurs while writing to the
             *        file.
             */
            void write(off_t offset, size_t write_size, unsigned char *buf);

            /**
             * \brief Reads data from the file referenced by this handle
             *        asynchronously.
             *
             * \param offset The offset in bytes from which to begin reading.
             * \param read_size The number of bytes to read.
             * \param buf The buffer to store data into. This buffer _must_ be
             *        at least `size` bytes in length to avoid a buffer
             *        overflow.
             * \param callback The callback to execute upon completion of the
             *        read operation. The function handle may be empty. Note
             *        that this callback is not guaranteed to run (e.g. if the
             *        read operation generates an exception), so it should not
             *        contain any critical code.
             *
             * \return A std::future which will be completed after the data has
             *         been fully read, or after the read operation generates an
             *         exception.
             *
             * \throw std::invalid_argument If the current mode does not support
             *        reading, or if `size` or `offset` are nonsensical
             *        (individiually or in conjunction).
             *
             * \attention Any exceptions thrown by the read operation itself (not
             *            including parameter validation) are exposed through the
             *            returned std::future.
             *
             * \sa FileHandle::read
             */
            const std::future<void> read_async(off_t offset, size_t read_size,
                    unsigned char *buf, std::function<void(FileHandle&)> callback);

            /**
             * \brief Writes data into the file referenced by the handle
             *        asynchronously.
             *
             * \param offset The offset in bytes at which to begin writing, or
             *        `-1` to append to the end of the file.
             * \param write_size The number of bytes to write.
             * \param buf A buffer containing the data to be written. This
             *            buffer _must_ be at least `size` bytes in length to
             *            avoid a buffer overflow.
             * \param callback The callback to execute upon completion of the
             *        write operation. The function handle may be empty. Note
             *        that this callback is not guaranteed to run (e.g. if the
             *        write operation generates an exception), so it should not
             *        contain any critical code.
             *
             * \return A std::future which will be completed after the data has
             *         been fully written, or after the write operation
             *         generates an exception.
             *
             * \throw std::runtime_error If the handle is not valid.
             * \throw std::invalid_argument If the current mode does not support
             *        writing, or if `size` or `offset` are nonsensical
             *        (individiually or in conjunction).
             *
             * \attention Any exceptions thrown by the read operation itself (not
             *            including parameter validation) are exposed through the
             *            returned std::future.
             *
             * \sa FileHandle::write
             */
            const std::future<void> write_async(off_t offset, size_t write_size,
                    unsigned char *buf, std::function<void(FileHandle&)> callback);
    };

    /**
     * \brief Gets the path to the current executable. This function has various
     *        implementations depending on the target platform.
     *
     * \return The path to the current executable.
     *
     * \throw std::runtime_error (Windows only) If the executable path is too
     *        large for the engine's buffer.
     * \throw std::system_error If an error occurs while obtaining the
     *        executable path.
     */
    const std::string get_executable_path(void);

    /**
     * \brief Returns a list of entries in the current directory.
     *
     * \param directory_path The relative or absolute path to the directory to
     *                       query.
     *
     * \return A std::vector containing the unqualified names of all entries in
     *         the specified directory.
     *
     * \throw std::system_error If an error occurs while opening the directory.
     */
    const std::vector<std::string> list_directory_entries(const std::string &directory_path);

    /**
     * \brief Gets whether the entry at the given path is a directory.
     *
     * If the entry does not exist, this function will return `false`.
     *
     * \param path The path to query.
     *
     * \return Whether the entry at the path is a directory.
     */
    bool is_directory(const std::string &path);

    /**
     * \brief Gets whether the entry at the given path is a regular file.
     *
     * If the entry does not exist, this function will return `false`.
     *
     * \param path The path to query.
     *
     * \return Whether the entry at the path is a regular file.
     */
    bool is_regfile(const std::string &path);

    /**
     * \brief Parses the parent of the entry at the given path.
     *
     * If the given path is at the root of a filesystem, the original string
     * will be returned.
     *
     * \param path The path to query.
     *
     * \return The parent of the given path.
     *
     * \throw std::system_error If the `path` parameter is an empty string.
     */
    std::string get_parent(const std::string &path);

    /**
     * \brief Parses the name and extension from the given file name.
     *
     * \param file_name The file name to parse.
     *
     * \return A std::pair containing the name and extension of the given file
     *         name, respectively.
     *
     * \warning This function does not operate on paths (relative or absolute)
     *          and will not handle them correctly.
     */
    std::pair<const std::string, const std::string> get_name_and_extension(const std::string &file_name);

}
