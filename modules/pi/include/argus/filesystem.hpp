#pragma once

#include <string>

#define FILE_MODE_READ 1
#define FILE_MODE_WRITE 2
#define FILE_MODE_APPEND 4
#define FILE_MODE_CREATE 8

namespace argus {
    
    std::string get_executable_path(void);

    class FileHandle {
        private:
            std::string path;
            size_t size;
            void *handle;

            bool valid;

            FileHandle(const std::string path, const size_t size, void *const handle);

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
    };

}
