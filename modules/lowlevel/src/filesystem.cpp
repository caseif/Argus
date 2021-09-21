/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
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

// module lowlevel
#include "argus/lowlevel/filesystem.hpp"
#include "argus/lowlevel/threading/future.hpp"
#include "internal/lowlevel/error_util.hpp"
#include "internal/lowlevel/logging.hpp"

#include <future>
#include <stdexcept>

#include <cerrno>
#include <cstdio>
#include <cstring>
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
    #include <dirent.h>
    #include <features.h>
    #include <unistd.h>

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

    FileHandle::FileHandle(const std::string &path, const int mode, const size_t size, void *const handle):
            path(path),
            mode(mode),
            size(size),
            handle(handle),
            valid(true) {
    }

    //TODO: use the native Windows file API, if available
    FileHandle FileHandle::create(const std::string &path, const int mode) {
        const char *std_mode = "";
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
            _ARGUS_FATAL("Unable to determine mode string");
        }

        FILE *file = NULL;
        if (mode == (FILE_MODE_READ | FILE_MODE_CREATE)) {
            stat_t stat_buf;
            int stat_rc = stat(path.c_str(), &stat_buf);

            if (stat_rc) {
                // throw the error directly if it's not ENOENT (file not found)
                validate_syscall(errno == ENOENT, "stat");

                // if the file doesn't exist, create it
                #ifdef _WIN32
                FILE *file_tmp;
                fopen_s(&file_tmp, path.c_str(), "w");
                #else
                FILE *file_tmp = fopen(path.c_str(), "w");
                #endif
                validate_syscall(file_tmp, "fopen");

                fclose(file_tmp);
            }
        }

        #ifdef _WIN32
        fopen_s(&file, path.c_str(), std_mode);
        #else
        file = fopen(path.c_str(), std_mode);
        #endif

        validate_syscall(file != nullptr, "fopen");

        stat_t stat_buf;
        validate_syscall(fstat(fileno(file), &stat_buf), "fstat");

        return FileHandle(path, mode, stat_buf.st_size, static_cast<void*>(file));
    }

    const std::string &FileHandle::get_path(void) const {
        return path;
    }

    size_t FileHandle::get_size(void) const {
        return size;
    }

    void FileHandle::release(void) {
        validate_state(this->valid, "Non-valid FileHandle");

        this->valid = false;
        validate_syscall(fclose(static_cast<FILE*>(this->handle)), "fclose");
    }

    void FileHandle::remove(void) {
        this->release();
        ::remove(this->path.c_str());
    }

    void FileHandle::to_istream(const off_t offset, std::ifstream &target) const {
        validate_state(valid, "Non-valid FileHandle");

        fseek(static_cast<FILE*>(handle), offset, SEEK_SET);

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

    void FileHandle::read(const off_t offset, const size_t read_size, unsigned char *const buf) const {
        validate_state(valid, "Non-valid FileHandle");

        validate_arg(read_size > 0, "Invalid size parameter");

        validate_arg(offset + read_size <= this->size, "Invalid offset/size combination");

        fseek(static_cast<FILE*>(handle), offset, SEEK_SET);

        size_t read_chunks = fread(buf, read_size, 1, static_cast<FILE*>(handle));

        validate_syscall(read_chunks == 1, "fread");
    }

    void FileHandle::write(const off_t offset, const size_t write_size, unsigned char *const buf) {
        validate_state(valid, "Non-valid FileHandle");

        validate_arg(write_size > 0, "Invalid size parameter");

        validate_arg(offset >= -1, "Invalid offset parameter");

        if (offset == -1) {
            fseek(static_cast<FILE*>(handle), 0, SEEK_END);
        } else {
            fseek(static_cast<FILE*>(handle), offset, SEEK_SET);
        }

        size_t write_chunks = fwrite(buf, write_size, 1, static_cast<FILE*>(handle));
        validate_syscall(write_chunks == 1, "fwrite");

        stat_t file_stat;
        validate_syscall(fstat(fileno(static_cast<FILE*>(handle)), &file_stat), "fstat");

        this->size = file_stat.st_size;
    }

    const std::future<void> FileHandle::read_async(const off_t offset, const size_t read_size, unsigned char *const buf,
            const std::function<void(FileHandle&)> callback) {
        validate_state(valid, "Non-valid FileHandle");

        validate_arg(read_size > 0, "Invalid size parameter");

        validate_arg(offset + read_size <= this->size, "Invalid offset/size combintation");

        return make_future(std::bind(&FileHandle::read, this, offset, size, buf), std::bind(callback, *this));
    }

    const std::future<void> FileHandle::write_async(const off_t offset, const size_t write_size,
            unsigned char *const buf, std::function<void(FileHandle&)> callback) {
        validate_state(valid, "Non-valid FileHandle");

        validate_arg(write_size > 0, "Invalid size parameter");

        validate_arg(offset >= -1, "Invalid offset parameter");

        return make_future(std::bind(&FileHandle::write, this, offset, write_size, buf), std::bind(callback, *this));
    }

    const std::string get_executable_path(void) {
        const size_t max_path_len = 4097;
        char path[max_path_len];

        #ifdef _WIN32
        int rc = 0;
        GetModuleFileNameA(NULL, path, max_path_len);
        rc = GetLastError(); // it so happens that ERROR_SUCCESS == 0

        validate_syscall(rc, "GetModuleFileName");
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
        validate_syscall(size != -1, "readlink");
        #elif defined __FreeBSD__
        int mib[4];
        mib[0] = CTL_KERN;
        mib[1] = KERN_PROC;
        mib[2] = KERN_PROC_PATHNAME;
        mib[3] = -1;

        validate_syscall(sysctl(mib, 4, path, &path_len, NULL, 0), "sysctl");
        #elif defined __NetBSD__
        readlink("/proc/curproc/exe", path, max_path_len);
        validate_syscall(size != -1, "readlink");
        #elif defined __DragonFly__
        readlink("/proc/curproc/file", path, max_path_len);
        validate_syscall(size != -1, "readlink");
        #endif

        return std::string(path);
    }

    const std::vector<std::string> list_directory_entries(const std::string &directory_path) {
        std::vector<std::string> res;

        #ifdef _WIN32
        WIN32_FIND_DATAA find_data;

        HANDLE find_handle = FindFirstFileA(directory_path.c_str(), &find_data);
        if (find_handle == INVALID_HANDLE_VALUE) {
            return res;
        }

        do {
            res.insert(res.end(), find_data.cFileName);
        } while (FindNextFileA(find_handle, &find_data) != 0);
        #else
        DIR *dir = opendir(directory_path.c_str());
        validate_syscall(dir != nullptr, "opendir");

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

    bool is_directory(const std::string &path) {
        stat_t path_stat;
        if (stat(path.c_str(), &path_stat) != 0) {
            return false;
        }
        return S_ISDIR(path_stat.st_mode);
    }

    bool is_regfile(const std::string &path) {
        stat_t path_stat;
        if (stat(path.c_str(), &path_stat) != 0) {
            return false;
        }
        return S_ISREG(path_stat.st_mode);
    }

    std::string get_parent(const std::string &path) {
        #ifdef _WIN32
        char path_separator = '\\';
        #else
        char path_separator = '/';
        #endif

        validate_arg(path.length() > 0, "Cannot get parent of zero-length path");

        size_t start = path.length() - 1;
        // skip any trailing slashes
        if (path[start] == path_separator) {
            start--;
        }

        //TOOD: this probably doesn't handle Windows paths too well right now
        size_t index = path.find_last_of(path_separator, start);
        if (index == std::string::npos) {
            return path;
        }

        return path.substr(0, index);
    }

    std::pair<const std::string, const std::string> get_name_and_extension(const std::string &file_name) {
        size_t index = file_name.find_last_of(EXTENSION_SEPARATOR);
        if (index == std::string::npos) {
            return {file_name, ""};
        } else {
            return {file_name.substr(0, index), file_name.substr(index + 1)};
        }
    }

}
