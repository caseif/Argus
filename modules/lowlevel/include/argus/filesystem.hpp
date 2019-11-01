/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include "argus/threading.hpp"

#include <atomic>
#include <fstream>
#include <functional>
#include <future>
#include <string>
#include <vector>

#ifdef _WIN32
// builds targeting 32-bit define size_t as a 32-bit int, which won't do
#define size_t unsigned __int64
#define ssize_t __int64

#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

#define EXTENSION_SEPARATOR "."

#define FILE_MODE_READ 1
#define FILE_MODE_WRITE 2
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

            FileHandle(const std::string path, const int mode, const size_t size, void *const handle);

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
            static FileHandle create(const std::string path, const int mode);

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
            const size_t get_size(void) const;

            /**
             * \brief Releases this file handle.
             *
             * \attention The handle will thereafter be invalidated and thus
             *            ineligible for further use.
             *
             * \throw std::invalid_argument If this handle is not valid.
             * \throw std::system_error If an error occurs while closing the
             *        file.
             */
            void release(void);

            /**
             * \brief Removes the file referenced by this handle.
             *
             * \attention This operation implicitly releases the handle,
             *            invalidating it.
             *
             * \throw std::invalid_argument If this handle is not valid.
             * \throw std::system_error If an error occurs while removing the
             *        file or closing its handle.
             *
             * \sa FileHandle::release
             */
            void remove(void);

            /**
             * \brief Creates an std::istream from this file handle.
             *
             * \param offset The offset in bytes at which to open the
             *        std::istream.
             * \param target The object to use when opening the stream.
             *
             * \throw std::invalid_argument If this handle is not valid.
             */
            const void to_istream(const ssize_t offset, std::ifstream &target) const;

            /**
             * \brief Reads data from the file referenced by this handle.
             *
             * \param offset The offset in bytes from which to begin reading.
             * \param size The number of bytes to read.
             * \param buf The buffer to store data into. This buffer _must_ be
             *            at least `size` bytes in length to avoid a buffer
             *            overflow.
             *
             * \throw std::invalid_argument If this handle is not valid, if the
             *        current mode does not support reading, or if `size` or
             *        `offset` are nonsensical (individiually or in
             *        conjunction).
             * \throw std::system_error If an error occurs while reading from
             *        the file.
             */
            const void read(const size_t offset, const size_t size, unsigned char *const buf) const;

            /**
             * \brief Writes data into the file referenced by this handle.
             *
             * \param offset The offset in bytes at which to begin writing, or
             *        `-1` to append to the end of the file.
             * \param size The number of bytes to write.
             * \param buf A buffer containing the data to be written. This
             *            buffer _must_ be at least `size` bytes in length to
             *            avoid a buffer over-read.
             *
             * \throw std::invalid_argument If this handle is not valid, if the
             *        current mode does not support reading, or if `size` or
             *        `offset` are nonsensical (individiually or in
             *        conjunction).
             * \throw std::system_error If an error occurs while writing to the
             *        file.
             */
            const void write(const ssize_t offset, const size_t size, unsigned char *const buf);

            /**
             * \brief Reads data from the file referenced by this handle
             * asynchronously.
             *
             * \param offset The offset in bytes from which to begin reading.
             * \param size The number of bytes to read.
             * \param buf The buffer to store data into. This buffer _must_ be
             *            at least `size` bytes in length to avoid a buffer
             *            overflow.
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
             * \throw std::invalid_argument If this handle is not valid, if the
             *        current mode does not support reading, or if `size` or
             *        `offset` are nonsensical (individiually or in
             *        conjunction).
             *
             * \attention Any exceptions thrown by the read operation itself (not
             *            including parameter validation) are exposed through the
             *            returned std::future.
             *
             * \sa FileHandle::read
             */
            const std::future<void> read_async(const size_t offset, const size_t size,
                    unsigned char *const buf, const std::function<void(FileHandle&)> callback);

            /**
             * \brief Writes data into the file referenced by this handle
             * asynchronously.
             *
             * \param offset The offset in bytes at which to begin writing, or
             *        `-1` to append to the end of the file.
             * \param size The number of bytes to write.
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
             * \throw std::invalid_argument If this handle is not valid, if the
             *        current mode does not support reading, or if `size` or
             *        `offset` are nonsensical (individiually or in
             *        conjunction).
             *
             * \attention Any exceptions thrown by the read operation itself (not
             *            including parameter validation) are exposed through the
             *            returned std::future.
             *
             * \sa FileHandle::write
             */
            const std::future<void> write_async(const ssize_t offset, const size_t size,
                    unsigned char *const buf, const std::function<void(FileHandle&)> callback);
    };

    /**
     * \brief Gets the path to the current executable. This function has various
     * implementations depending on the target platform.
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
     * \return A std::vector containing the names of all entries in the
     *         specified directory.
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
