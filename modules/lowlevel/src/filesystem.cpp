/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#include "argus/filesystem.hpp"
#include "internal/logging.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <future>
#include <system_error>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
    #include <Windows.h>

    #ifndef errno
    #define errno WSAGetLastError()
    #endif

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
    #include <dirent.h>
    #include <unistd.h>
    #include <sys/stat.h>
    #include <sys/types.h>

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

    FileHandle::FileHandle(const std::string path, const int mode, const size_t size, void *const handle):
            path(path),
            mode(mode),
            size(size),
            handle(handle),
            valid(true) {
    }

    FileHandle::FileHandle(void):
            valid(false) {
    }

    //TODO: use the native Windows file API, if available
    FileHandle FileHandle::create(const std::string path, const int mode) {
        const char *std_mode;
        // we check for the following invalid cases:
        // - no modes set at all
        // - create set without any other modes
        // - read and create set without any other modes
        if ((mode == 0) || (mode == FILE_MODE_CREATE)
                || ((mode & ~FILE_MODE_READ) == (FILE_MODE_CREATE))) {
            throw std::invalid_argument("FileHandle::create called with invalid mode");
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
            _ARGUS_FATAL("Unable to determine mode string");
        }

        FILE *file;
        if (mode == (FILE_MODE_READ | FILE_MODE_CREATE)) {
            stat_t stat_buf;
            int stat_rc = stat(path.c_str(), &stat_buf);

            if (stat_rc) {
                if (errno == ENOENT) {
                    #ifdef _WIN32
                    FILE *file_tmp;
                    fopen_s(&file_tmp, path.c_str(), "w");
                    #else
                    FILE *file_tmp = fopen(path.c_str(), "w");
                    #endif
                    if (!file_tmp) {
                        throw std::system_error(errno, std::generic_category());
                    }

                    fclose(file_tmp);
                } else {
                    throw std::system_error(errno, std::generic_category());
                }
            }
        }

        #ifdef _WIN32
        fopen_s(&file, path.c_str(), std_mode);
        #else
        file = fopen(path.c_str(), std_mode);
        #endif

        if (file == nullptr) {
            throw std::system_error(errno, std::generic_category());
        }

        stat_t stat_buf;
        fstat(fileno(file), &stat_buf);

        return FileHandle(path, mode, stat_buf.st_size, static_cast<void*>(file));
    }

    std::string const &FileHandle::get_path(void) const {
        return path;
    }

    const size_t FileHandle::get_size(void) const {
        return size;
    }

    const int FileHandle::release(void) {
        int rc = fclose(static_cast<FILE*>(this->handle));
        this->valid = false;
        if (rc == 0) {
            return 0;
        } else {
            return errno;
        }
    }

    const void FileHandle::to_istream(const ssize_t offset, std::ifstream &target) const {
        if (!valid) {
            throw std::invalid_argument("read called on non-valid FileHandle");
        }

        fseek(static_cast<FILE*>(handle), offset, SEEK_SET);

        char buf[CHUNK_SIZE];

        std::ios::openmode mode = std::ios::binary;
        if (this->mode & FILE_MODE_READ) {
            mode |= std::ios::in;
        }
        if (this->mode & FILE_MODE_WRITE) {
            mode |= std::ios::out;
        }

        target.open(path, mode);

        if (!target.good()) {
            throw std::runtime_error("Failed to create file stream");
        }

        //stream->seekg(offset);
    }

    const void FileHandle::read(const ssize_t offset, const size_t size, unsigned char *const buf) const {
        if (!valid) {
            throw std::invalid_argument("read called on non-valid FileHandle");
        }

        if (size == 0) {
            throw std::invalid_argument("read called with invalid size parameter");
        }

        if (offset + size > this->size) {
            throw std::invalid_argument("read called with invalid offset/size combination");
        }

        fseek(static_cast<FILE*>(handle), offset, SEEK_SET);

        size_t read_chunks = fread(buf, size, 1, static_cast<FILE*>(handle));

        if (read_chunks == 1) {
            throw std::system_error(errno, std::generic_category());
        }
    }

    const void FileHandle::write(const ssize_t offset, const size_t size, unsigned char *const buf) const {
        if (!valid) {
            throw std::invalid_argument("write called on non-valid FileHandle");
        }

        if (offset < -1) {
            throw std::invalid_argument("write called with invalid offset parameters");
        }

        if (offset == -1) {
            fseek(static_cast<FILE*>(handle), 0, SEEK_END);
        } else {
            fseek(static_cast<FILE*>(handle), offset, SEEK_SET);
        }

        size_t write_chunks = fwrite(buf, size, 1, static_cast<FILE*>(handle));

        if (write_chunks != 1) {
            throw std::system_error(errno, std::generic_category());
        }
    }

    const std::future<void> FileHandle::read_async(const ssize_t offset, const size_t size, unsigned char *const buf,
            const std::function<void(FileHandle&)> callback) {
        if (!valid) {
            throw std::invalid_argument("read_async called on non-valid FileHandle");
        }

        if (offset + size > this->size) {
            throw std::invalid_argument("read_async called with invalid offset/size parameters");
        }

        return make_future(std::bind(&FileHandle::read, this, offset, size, buf), std::bind(callback, *this));
    }

    const std::future<void> FileHandle::write_async(const ssize_t offset, const size_t size, unsigned char *const buf,
            std::function<void(FileHandle&)> callback) {
        if (!valid) {
            throw std::invalid_argument("write_async called on non-valid FileHandle");
        }

        return make_future(std::bind(&FileHandle::write, this, offset, size, buf), std::bind(callback, *this));
    }

    const std::string get_executable_path(void) {
        const size_t max_path_len = 4097;
        size_t path_len = max_path_len;
        char path[max_path_len];

        int rc = 0;

        #ifdef _WIN32
        GetModuleFileName(NULL, path, max_path_len);
        rc = GetLastError(); // it so happens that ERROR_SUCCESS == 0

        if (rc != 0) {
            throw std::system_error(errno, std::generic_category(), "Failed to get executable path");
            return rc;
        }
        #elif defined __APPLE__
        uint32_t size;
        rc = _NSGetExecutablePath(path, &size);

        if (rc != 0) {
            _ARGUS_WARN("Need %u bytes to store full executable path\n", size);
            throw std::runtime_error("Executable path too long for buffer");
            return rc;
        }
        #elif defined __linux__
        ssize_t size = readlink("/proc/self/exe", path, max_path_len);
        if (size == -1) {
            throw std::system_error(errno, std::generic_category(), "Failed to read /proc/self/exe");
        }
        #elif defined __FreeBSD__
        int mib[4];
        mib[0] = CTL_KERN;
        mib[1] = KERN_PROC;
        mib[2] = KERN_PROC_PATHNAME;
        mib[3] = -1;

        rc = sysctl(mib, 4, path, &path_len, NULL, 0);

        if (rc != 0) {
            throw std::runtime_error("Failed to get executable path");
        }
        #elif defined __NetBSD__
        readlink("/proc/curproc/exe", path, max_path_len);
        if (size == -1) {
            throw std::system_error(errno, std::generic_category(), "Failed to read /proc/curproc/exe");
        }
        #elif defined __DragonFly__
        readlink("/proc/curproc/file", path, max_path_len);
        if (size == -1) {
            throw std::system_error(errno, std::generic_category(), "Failed to read /proc/curproc/file");
        }
        #endif

        return std::string(path);
    }

    const std::vector<std::string> list_directory_files(std::string const &directory_path) {
        std::vector<std::string> res;

        #ifdef _WIN32
        #error "Not yet supported"
        //TODO
        #else
        DIR *dir = opendir(directory_path.c_str());
        if (dir == NULL) {
            throw std::runtime_error("Failed to open directory");
        }

        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                continue;
            }

            res.insert(res.cbegin(), std::string(ent->d_name));
        }

        closedir(dir);
        #endif

        return res;
    }

    bool is_directory(std::string const &path) {
        stat_t path_stat;
        if (stat(path.c_str(), &path_stat) != 0) {
            return false;
        }
        return S_ISDIR(path_stat.st_mode);
    }

    bool is_regfile(std::string const &path) {
        stat_t path_stat;
        if (stat(path.c_str(), &path_stat) != 0) {
            return false;
        }
        return S_ISREG(path_stat.st_mode);
    }

    std::string get_parent(std::string const &path) {
        #ifdef _WIN32
        char path_separator = '\\';
        #else
        char path_separator = '/';
        #endif

        size_t start = path.length() - 1;
        if (path[start] == path_separator) {
            start--;
        }
        return path.substr(0, path.find_last_of(path_separator, start));
    }

    std::pair<const std::string, const std::string> get_name_and_extension(std::string const &file_name) {
        size_t index = file_name.find_last_of(EXTENSION_SEPARATOR);
        if (index == std::string::npos) {
            return {file_name, ""};
        } else {
            return {file_name.substr(0, index), file_name.substr(index + 1)};
        }
    }

}
