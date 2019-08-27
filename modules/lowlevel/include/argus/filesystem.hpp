#pragma once

#include "argus/threading.hpp"

#include <atomic>
#include <functional>
#include <string>

#ifdef _WIN32
// builds targeting 32-bit define size_t as a 32-bit int, which won't do
#define size_t unsigned __int64
#define ssize_t __int64
#endif

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

            void *read_async_wrapper(void *ptr);

            void *write_async_wrapper(void *ptr);

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

            const std::string get_path(void);

            const size_t get_size(void);

            /**
             * \brief Releases this file handle. The handle will thereafter be
             * invalidated and thus ineligible for further use.
             *
             * \param handle The handle to release.
             * \return 0 on success, non-zero otherwise.
             */
            const int release(void);

            const int read(ssize_t offset, size_t size, unsigned char *const buf) const;

            const int read_async(ssize_t offset, size_t size, unsigned char *const buf,
                    AsyncFileRequestCallback callback, AsyncFileRequestHandle *request_handle);
            
            const int write(ssize_t offset, size_t size, unsigned char *const buf) const;

            const int write_async(ssize_t offset, size_t size, unsigned char *const buf,
                    AsyncFileRequestCallback callback, AsyncFileRequestHandle *request_handle);

    };

    std::string get_executable_path(void);

}
