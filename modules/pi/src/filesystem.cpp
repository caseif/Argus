#include "argus/filesystem.hpp"

#include <cstdio>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
    #include <Windows.h>

    #define fileno _fileno
    #define fstat _fstat64
    #define stat_t __stat64
#elif defined __APPLE__
    #include <mach-o/dyld.h>

    #define stat_t struct stat
#elif defined __linux__
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
    #define fstat fstat64
    #define stat_t struct stat64
#elif defined __NetBSD__ || defined __DragonFly__
    #include <unistd.h>

    #define stat_t struct stat
#elif defined __FreeBSD__
    #include <sys/sysctl.h>
    #include <sys/types.h>

    #define stat_t struct stat
#else
    #error "This OS is not supported at this time."
#endif

namespace argus {

    int get_executable_path(std::string *str) {
        size_t max_path_len = 4097;
        char path[max_path_len];

        int rc;

        #ifdef _WIN32
        GetModuleFileName(NULL, path, max_path_len);
        rc = GetLastError(); // it so happens that ERROR_SUCCESS == 0

        if (rc != 0) {
            return rc;
        }
        #elif defined __APPLE__
        uint32_t size;
        rc = _NSGetExecutablePath(path, &size);

        if (rc != 0) {
            _ARGUS_WARN("Need %u bytes to store full executable path\n", size);
            return rc;
        }
        #elif defined __linux__
        ssize_t size = readlink("/proc/self/exe", path, max_path_len);
        if (size == -1) {
            return errno;
        }
        #elif defined __FreeBSD__
        int mib[4];
        mib[0] = CTL_KERN;
        mib[1] = KERN_PROC;
        mib[2] = KERN_PROC_PATHNAME;
        mib[3] = -1;

        rc = sysctl(mib, 4, path, &max_path_len, NULL, 0);

        if (rc != 0) {
            return errno;
        }
        #elif defined __NetBSD__
        readlink("/proc/curproc/exe", path, max_path_len);
        if (size == -1) {
            return errno;
        }
        #elif defined __DragonFly__
        readlink("/proc/curproc/file", path, max_path_len);
        if (size == -1) {
            return errno;
        }
        #endif

        *str = std::string(path);

        return rc;
    }
    
    FileHandle::FileHandle(const std::string path, const size_t size, void *const handle):
        path(path),
        size(size),
        handle(handle),
        valid(false) {
    }

    const int FileHandle::create(const std::string path, const int mode, FileHandle *const handle) {
        const char *std_mode;
        // we check for the following invalid cases:
        // - no modes set at all
        // - write and append set simultaneously
        // - create set without any other modes
        // - read and create set without any other modes
        if ((mode == 0) || (mode & (FILE_MODE_WRITE | FILE_MODE_APPEND)) || (mode == FILE_MODE_CREATE)
                || ((mode & ~FILE_MODE_READ) == (FILE_MODE_CREATE))) {
            printf("FileHandle::create called with invalid mode %d\n", mode);
            return -1;
        }

        if (mode & (FILE_MODE_READ | FILE_MODE_WRITE | FILE_MODE_CREATE)) {
            std_mode = "w+";
        } else if (mode & (FILE_MODE_READ | FILE_MODE_WRITE)) {
            std_mode = "r+";
        } else if (mode & (FILE_MODE_READ | FILE_MODE_APPEND)) {
            std_mode = "a+";
        } else if (mode & FILE_MODE_READ) {
            std_mode = "r";
        } else if (mode & FILE_MODE_WRITE) {
            std_mode = "w";
        } else if (mode & FILE_MODE_APPEND) {
            std_mode = "a";
        }

        FILE *file = fopen64(path.c_str(), std_mode);
        if (file == nullptr) {
            return -1;
        }

        stat_t stat_buf;
        fstat(fileno(file), &stat_buf);

        *handle = std::move(FileHandle(path, stat_buf.st_size, static_cast<void*>(file)));
    }

    const int FileHandle::release(void) {
        fclose(static_cast<FILE*>(this->handle));
        this->valid = false;
    }

}
