#ifndef XBTOS_FS_VFS_H
#define XBTOS_FS_VFS_H

#include <types.h>
#include <string.h>

#define XBTOS_MAX_FILES       64
#define XBTOS_FILE_NAME_SIZE  32
#define XBTOS_FILE_DATA_SIZE  1024
#define XBTOS_PATH_SIZE       64

namespace fs {

enum FileResult {
    FILE_OK = 0,
    FILE_ERR_NOT_FOUND = -1,
    FILE_ERR_EXISTS = -2,
    FILE_ERR_FULL = -3,
    FILE_ERR_INVALID_NAME = -4,
    FILE_ERR_TOO_LARGE = -5
};

enum FileOpenMode {
    FILE_OPEN_READ = 1,
    FILE_OPEN_WRITE = 2,
    FILE_OPEN_APPEND = 4,
    FILE_OPEN_CREATE = 8
};

struct FileStat {
    char name[XBTOS_FILE_NAME_SIZE];
    uint32_t size;
    bool is_directory;
};

struct File {
    char name[XBTOS_FILE_NAME_SIZE];
    char data[XBTOS_FILE_DATA_SIZE];
    uint32_t size;
    bool is_used;
};

struct FileSystemInterface {
    void (*init)(void* instance);
    FileResult (*create)(void* instance, const char* path);
    FileResult (*write)(void* instance, const char* path, const char* content);
    FileResult (*append)(void* instance, const char* path, const char* content);
    FileResult (*remove)(void* instance, const char* path);
    const char* (*read)(void* instance, const char* path);
    bool (*exists)(void* instance, const char* path);
    FileResult (*stat)(void* instance, const char* path, FileStat* out_stat);
    void (*list)(void* instance, const char* path);
};

class FileSystem {
private:
    File files[XBTOS_MAX_FILES];
    uint32_t file_count;
    char current_path[XBTOS_PATH_SIZE];

    File* find(const char* path);
    const File* find(const char* path) const;
    FileResult normalize_name(const char* path, char* out_name) const;
    File* allocate_slot();

public:
    FileSystem();

    void init();
    FileResult create(const char* path);
    FileResult write(const char* path, const char* content);
    FileResult append(const char* path, const char* content);
    FileResult remove(const char* path);
    const char* read(const char* path);
    bool exists(const char* path);
    FileResult stat(const char* path, FileStat* out_stat);
    void list(const char* path = "/");

    const char* pwd() const;
};

} // namespace fs

using FileSystem = fs::FileSystem;
using FileResult = fs::FileResult;
using FileStat = fs::FileStat;

#endif

