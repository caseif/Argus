#pragma once

#include "argus/threading.hpp"

#include <atomic>
#include <functional>
#include <string>

#define FILE_MODE_READ 1
#define FILE_MODE_WRITE 2
#define FILE_MODE_APPEND 4
#define FILE_MODE_CREATE 8

namespace argus {

    class FileHandle;
    class AsyncFileRequestHandle;

    typedef std::function<void(AsyncFileRequestHandle)> AsyncFileRequestCallback;

    class AsyncFileRequestHandle {
        friend class FileHandle;

        private:
            FileHandle *file_handle;
            size_t offset;
            size_t size;
            unsigned char *buf;
            AsyncFileRequestCallback callback;

            size_t streamed_bytes;
            bool success;

            std::atomic_bool result_valid;

            Thread *thread;

            AsyncFileRequestHandle operator =(AsyncFileRequestHandle const &rhs);

            AsyncFileRequestHandle(AsyncFileRequestHandle const &rhs);

            AsyncFileRequestHandle(AsyncFileRequestHandle const &&rhs);

            AsyncFileRequestHandle(FileHandle &file_handle, const size_t offset, const size_t size,
                    unsigned char *const buf, AsyncFileRequestCallback callback);

        public:
            void join(void);

            void cancel(void);

            size_t get_streamed_bytes(void);

            bool was_successful(void);

            bool is_result_valid(void);
    };

    std::string get_executable_path(void);

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

            const int read(size_t offset, size_t size, unsigned char *const buf) const;

            const int read_async(size_t offset, size_t size, unsigned char *const buf,
                    AsyncFileRequestCallback callback, AsyncFileRequestHandle *request_handle);
            
            const int write(size_t offset, size_t size, unsigned char *const buf) const;

            const int write_async(size_t offset, size_t size, unsigned char *const buf,
                    AsyncFileRequestCallback callback, AsyncFileRequestHandle *request_handle);

    };

}
