#include "argus/error.hpp"
#include "argus/filesystem.hpp"

#include <algorithm>
#include <cstdio>
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

namespace argus {

    FileHandle::FileHandle(const std::string path, const size_t size, void *const handle) :
            path(path),
            size(size),
            handle(handle),
            valid(true) {
    }

    //TODO: use the native Windows file API, if available
    const int FileHandle::create(const std::string path, const int mode, FileHandle *const handle) {
        const char *std_mode;
        // we check for the following invalid cases:
        // - no modes set at all
        // - create set without any other modes
        // - read and create set without any other modes
        if ((mode == 0) || (mode == FILE_MODE_CREATE)
                || ((mode & ~FILE_MODE_READ) == (FILE_MODE_CREATE))) {
            set_error("FileHandle::create called with invalid mode");
            return -1;
        }

        if (mode & (FILE_MODE_READ | FILE_MODE_WRITE | FILE_MODE_CREATE)) {
            std_mode = "w+";
        } else if (mode & (FILE_MODE_READ | FILE_MODE_WRITE)) {
            std_mode = "r+";
        } else if (mode & FILE_MODE_READ) {
            std_mode = "r";
        } else if (mode & FILE_MODE_WRITE) {
            std_mode = "w";
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
                        return errno;
                    }

                    fclose(file_tmp);
                } else {
                    return errno;
                }
            }
        }

        #ifdef _WIN32
        fopen_s(&file, path.c_str(), std_mode);
        #else
        file = fopen(path.c_str(), std_mode);
        #endif

        if (file == nullptr) {
            return errno;
        }

        stat_t stat_buf;
        fstat(fileno(file), &stat_buf);

        *handle = std::move(FileHandle(path, stat_buf.st_size, static_cast<void*>(file)));

        return 0;
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

    const int FileHandle::read(const ssize_t offset, const size_t size, unsigned char *const buf) const {
        if (!valid) {
            set_error("read called on invalid FileHandle");
            return -1;
        }

        if (size == 0) {
            set_error("read called with invalid size parameter");
        }

        if (offset + size > this->size) {
            set_error("read called with invalid offset/size combination");
            return -1;
        }

        fseek(static_cast<FILE*>(handle), offset, SEEK_SET);

        size_t read_chunks = fread(buf, size, 1, static_cast<FILE*>(handle));

        if (read_chunks == 1) {
            return errno;
        }

        return 0;
    }

    const int FileHandle::write(const ssize_t offset, const size_t size, unsigned char *const buf) const {
        if (!valid) {
            set_error("write called on invalid FileHandle");
            return -1;
        }

        if (offset < -1) {
            set_error("write called with invalid offset parameters");
            return -1;
        }

        if (offset == -1) {
            fseek(static_cast<FILE*>(handle), 0, SEEK_END);
        } else {
            fseek(static_cast<FILE*>(handle), offset, SEEK_SET);
        }

        size_t read_chunks = fwrite(buf, size, 1, static_cast<FILE*>(handle));

        if (read_chunks == 1) {
            return errno;
        }

        return 0;
    }

    const int FileHandle::read_async(const ssize_t offset, const size_t size, unsigned char *const buf,
            const AsyncFileRequestCallback callback, AsyncFileRequestHandle *const request_handle) {
        if (!valid) {
            set_error("read_async called on invalid FileHandle");
            return -1;
        }

        if (offset + size > this->size) {
            set_error("read_async called with invalid offset/size parameters");
            return -1;
        }

        AsyncFileRequestHandle handle_local = AsyncFileRequestHandle(this, FileStreamData{offset, size, buf, 0}, callback);

        handle_local.execute(std::bind(&FileHandle::read_trampoline, this, std::placeholders::_1));

        *request_handle = handle_local;
        
        return 0;
    }

    const int FileHandle::write_async(const ssize_t offset, const size_t size, unsigned char *const buf,
            const AsyncFileRequestCallback callback, AsyncFileRequestHandle *const request_handle) {
        if (!valid) {
            set_error("write_async called on invalid FileHandle");
            return -1;
        }

        AsyncFileRequestHandle handle_local = AsyncFileRequestHandle(this, FileStreamData{offset, size, buf}, callback);

        handle_local.execute(std::bind(&FileHandle::write_trampoline, this, std::placeholders::_1));

        *request_handle = handle_local;
        
        return 0;
    }

    void FileHandle::read_trampoline(AsyncFileRequestHandle const &handle) {
        handle.item->read(handle.data.offset, handle.data.size, handle.data.buf);
    }

    void FileHandle::write_trampoline(AsyncFileRequestHandle const &handle) {
        handle.item->write(handle.data.offset, handle.data.size, handle.data.buf);
    }
    
    int get_executable_path(std::string *const str) {
        const size_t max_path_len = 4097;
        size_t path_len = max_path_len;
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

        rc = sysctl(mib, 4, path, &path_len, NULL, 0);

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

    int list_directory_files(std::string const &directory_path, std::vector<std::string> *const target) {
        #ifdef _WIN32
        #error "Not yet supported"
        //TODO
        #else
        DIR *dir = opendir(directory_path.c_str());
        if (dir == NULL) {
            set_error("Failed to open directory");
            return -1;
        }

        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            target->insert(target->cbegin(), std::string(ent->d_name));
        }

        closedir(dir);
        #endif
    }

    bool is_directory(std::string const &path) {
        stat_t path_stat;
        if (stat(path.c_str(), &path_stat) != 0) {
            set_error("Failed to stat path");
            return false;
        }
        return S_ISDIR(path_stat.st_mode);
    }
    
    bool is_regfile(std::string const &path) {
        stat_t path_stat;
        if (stat(path.c_str(), &path_stat) != 0) {
            set_error("Failed to stat path");
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
