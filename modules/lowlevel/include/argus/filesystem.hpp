#pragma once

#include "argus/threading.hpp"

#include <atomic>
#include <functional>
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

    class FileHandle;

    struct FileStreamData {
        ssize_t offset;
        size_t size;
        unsigned char *buf;
        size_t streamed_bytes;
    };

    typedef AsyncRequestHandle<FileHandle*, FileStreamData> AsyncFileRequestHandle;
    typedef AsyncRequestCallback<FileHandle*, FileStreamData> AsyncFileRequestCallback;

    class FileHandle {
        private:
            std::string path;
            size_t size;
            void *handle;

            bool valid;

            FileHandle(const std::string path, const size_t size, void *const handle);

            void read_trampoline(AsyncFileRequestHandle const &handle);

            void write_trampoline(AsyncFileRequestHandle const &handle);

        public:
            /**
             * \brief Creates a handle to the file at the provided path and stores it in
             * memory at the provided pointer.
             * 
             * \param path The path of the file to obtain a handle to.
             * \param handle A pointer to the memory address to store the handle in.
             * \return 0 on success, non-zero otherwise.
             */
            static const int create(const std::string path, const int mode, FileHandle *const handle);

            const std::string get_path(void) const;

            const size_t get_size(void) const;

            /**
             * \brief Releases this file handle. The handle will thereafter be
             * invalidated and thus ineligible for further use.
             *
             * \param handle The handle to release.
             * \return 0 on success, non-zero otherwise.
             */
            const int release(void);

            const int read(const ssize_t offset, const size_t size, unsigned char *const buf) const;
            
            const int write(const ssize_t offset, const size_t size, unsigned char *const buf) const;

            const int read_async(const ssize_t offset, const size_t size, unsigned char *const buf,
                    const AsyncFileRequestCallback callback, AsyncFileRequestHandle *const request_handle);

            const int write_async(const ssize_t offset, const size_t size, unsigned char *const buf,
                    const AsyncFileRequestCallback callback, AsyncFileRequestHandle *const request_handle);
    };

    int get_executable_path(std::string *const target);

    std::vector<std::string> list_directory_files(std::string const &directory_path);

    bool is_directory(std::string const &path);

    bool is_regfile(std::string const &path);

    std::string get_parent(std::string const &path);

    std::pair<const std::string, const std::string> get_name_and_extension(std::string const &file_name);

}
