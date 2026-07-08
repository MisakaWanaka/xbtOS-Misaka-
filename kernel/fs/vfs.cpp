#include "vfs.h"
#include <hal/console.h>

namespace fs {

static bool is_path_separator(char c) {
    return c == '/' || c == '\\';
}

FileSystem::FileSystem() : file_count(0) {
    kmemset(files, 0, sizeof(files));
    current_path[0] = '/';
    current_path[1] = '\0';
}

void FileSystem::init() {
    file_count = 0;
    kmemset(files, 0, sizeof(files));
    current_path[0] = '/';
    current_path[1] = '\0';
    hal::print_string("VFS Initialized.\n");
}

FileResult FileSystem::normalize_name(const char* path, char* out_name) const {
    if (!path || path[0] == '\0') {
        return FILE_ERR_INVALID_NAME;
    }

    while (is_path_separator(*path)) {
        ++path;
    }

    if (path[0] == '\0' || path[0] == '.') {
        return FILE_ERR_INVALID_NAME;
    }

    uint32_t len = 0;
    while (path[len] != '\0') {
        if (is_path_separator(path[len])) {
            return FILE_ERR_INVALID_NAME;
        }
        if (len >= XBTOS_FILE_NAME_SIZE - 1) {
            return FILE_ERR_INVALID_NAME;
        }
        out_name[len] = path[len];
        ++len;
    }

    out_name[len] = '\0';
    return FILE_OK;
}

File* FileSystem::find(const char* path) {
    char name[XBTOS_FILE_NAME_SIZE];
    if (normalize_name(path, name) != FILE_OK) {
        return nullptr;
    }

    for (uint32_t i = 0; i < file_count; ++i) {
        if (files[i].is_used && kstrcmp(files[i].name, name) == 0) {
            return &files[i];
        }
    }
    return nullptr;
}

const File* FileSystem::find(const char* path) const {
    char name[XBTOS_FILE_NAME_SIZE];
    if (normalize_name(path, name) != FILE_OK) {
        return nullptr;
    }

    for (uint32_t i = 0; i < file_count; ++i) {
        if (files[i].is_used && kstrcmp(files[i].name, name) == 0) {
            return &files[i];
        }
    }
    return nullptr;
}

File* FileSystem::allocate_slot() {
    if (file_count >= XBTOS_MAX_FILES) {
        return nullptr;
    }
    return &files[file_count++];
}

FileResult FileSystem::create(const char* path) {
    char name[XBTOS_FILE_NAME_SIZE];
    FileResult normalized = normalize_name(path, name);
    if (normalized != FILE_OK) {
        return normalized;
    }
    if (find(name)) {
        return FILE_ERR_EXISTS;
    }

    File* file = allocate_slot();
    if (!file) {
        return FILE_ERR_FULL;
    }

    kmemset(file, 0, sizeof(File));
    kstrcpy(file->name, name);
    file->is_used = true;
    return FILE_OK;
}

FileResult FileSystem::write(const char* path, const char* content) {
    if (!content) {
        content = "";
    }

    File* file = find(path);
    if (!file) {
        FileResult created = create(path);
        if (created != FILE_OK && created != FILE_ERR_EXISTS) {
            return created;
        }
        file = find(path);
    }
    if (!file) {
        return FILE_ERR_NOT_FOUND;
    }

    uint32_t len = (uint32_t)kstrlen(content);
    if (len >= XBTOS_FILE_DATA_SIZE) {
        return FILE_ERR_TOO_LARGE;
    }

    kmemcpy(file->data, content, len);
    file->data[len] = '\0';
    file->size = len;
    return FILE_OK;
}

FileResult FileSystem::append(const char* path, const char* content) {
    if (!content) {
        return FILE_OK;
    }

    File* file = find(path);
    if (!file) {
        FileResult created = create(path);
        if (created != FILE_OK) {
            return created;
        }
        file = find(path);
    }
    if (!file) {
        return FILE_ERR_NOT_FOUND;
    }

    uint32_t add_len = (uint32_t)kstrlen(content);
    if (file->size + add_len >= XBTOS_FILE_DATA_SIZE) {
        return FILE_ERR_TOO_LARGE;
    }

    kmemcpy(file->data + file->size, content, add_len);
    file->size += add_len;
    file->data[file->size] = '\0';
    return FILE_OK;
}

const char* FileSystem::read(const char* path) {
    File* file = find(path);
    return file ? file->data : nullptr;
}

bool FileSystem::exists(const char* path) {
    return find(path) != nullptr;
}

FileResult FileSystem::stat(const char* path, FileStat* out_stat) {
    if (!out_stat) {
        return FILE_ERR_INVALID_NAME;
    }

    const File* file = find(path);
    if (!file) {
        return FILE_ERR_NOT_FOUND;
    }

    kmemset(out_stat, 0, sizeof(FileStat));
    kstrcpy(out_stat->name, file->name);
    out_stat->size = file->size;
    out_stat->is_directory = false;
    return FILE_OK;
}

void FileSystem::list(const char* path) {
    (void)path;
    bool found = false;
    for (uint32_t i = 0; i < file_count; ++i) {
        if (files[i].is_used) {
            found = true;
            hal::print_string(" - ");
            hal::print_string(files[i].name);
            hal::print_string("  ");
            char size_text[12];
            uint32_t size = files[i].size;
            uint32_t pos = 0;
            if (size == 0) {
                size_text[pos++] = '0';
            } else {
                char reversed[12];
                uint32_t count = 0;
                while (size > 0 && count < sizeof(reversed)) {
                    reversed[count++] = (char)('0' + (size % 10));
                    size /= 10;
                }
                while (count > 0) {
                    size_text[pos++] = reversed[--count];
                }
            }
            size_text[pos] = '\0';
            hal::print_string(size_text);
            hal::print_string(" bytes\n");
        }
    }

    if (!found) {
        hal::print_string(" (empty)\n");
    }
}

FileResult FileSystem::remove(const char* path) {
    char name[XBTOS_FILE_NAME_SIZE];
    FileResult normalized = normalize_name(path, name);
    if (normalized != FILE_OK) {
        return normalized;
    }

    for (uint32_t i = 0; i < file_count; ++i) {
        if (files[i].is_used && kstrcmp(files[i].name, name) == 0) {
            kmemset(&files[i], 0, sizeof(File));
            if (i != file_count - 1) {
                files[i] = files[file_count - 1];
                kmemset(&files[file_count - 1], 0, sizeof(File));
            }
            --file_count;
            return FILE_OK;
        }
    }

    return FILE_ERR_NOT_FOUND;
}

const char* FileSystem::pwd() const {
    return current_path;
}

} // namespace fs

