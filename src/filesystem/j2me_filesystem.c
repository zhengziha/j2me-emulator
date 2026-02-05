#include "j2me_filesystem.h"
#include "j2me_vm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <libgen.h>

/**
 * @file j2me_filesystem.c
 * @brief J2ME文件系统实现
 * 
 * 基于POSIX的文件系统操作
 */

#define MAX_FILE_CONNECTIONS    32
#define MAX_PATH_LENGTH         1024
#define MAX_FILENAME_LENGTH     256

j2me_filesystem_manager_t* j2me_filesystem_manager_create(j2me_vm_t* vm) {
    if (!vm) {
        return NULL;
    }
    
    j2me_filesystem_manager_t* manager = (j2me_filesystem_manager_t*)malloc(sizeof(j2me_filesystem_manager_t));
    if (!manager) {
        return NULL;
    }
    
    memset(manager, 0, sizeof(j2me_filesystem_manager_t));
    
    manager->max_connections = MAX_FILE_CONNECTIONS;
    manager->connections = (j2me_file_connection_t**)calloc(MAX_FILE_CONNECTIONS, sizeof(j2me_file_connection_t*));
    if (!manager->connections) {
        free(manager);
        return NULL;
    }
    
    manager->security_enabled = true;
    
    printf("[文件系统] 文件系统管理器创建成功\n");
    return manager;
}

void j2me_filesystem_manager_destroy(j2me_filesystem_manager_t* manager) {
    if (!manager) {
        return;
    }
    
    // 关闭所有文件连接
    for (int i = 0; i < manager->max_connections; i++) {
        if (manager->connections[i]) {
            j2me_file_connection_close(manager->connections[i]);
            manager->connections[i] = NULL; // 清空指针避免重复释放
        }
    }
    
    // 释放根目录列表
    for (int i = 0; i < manager->root_count; i++) {
        if (manager->allowed_roots[i]) {
            free(manager->allowed_roots[i]);
        }
    }
    if (manager->allowed_roots) {
        free(manager->allowed_roots);
    }
    
    free(manager->connections);
    free(manager);
    
    printf("[文件系统] 文件系统管理器已销毁\n");
}

j2me_error_t j2me_filesystem_initialize(j2me_filesystem_manager_t* manager) {
    if (!manager || manager->initialized) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 添加默认的安全根目录
    j2me_filesystem_add_root(manager, "/tmp");
    j2me_filesystem_add_root(manager, "./");
    
    manager->initialized = true;
    
    printf("[文件系统] 文件系统初始化成功\n");
    return J2ME_SUCCESS;
}

void j2me_filesystem_shutdown(j2me_filesystem_manager_t* manager) {
    if (!manager || !manager->initialized) {
        return;
    }
    
    // 关闭所有文件连接
    j2me_filesystem_close_all(manager);
    
    manager->initialized = false;
    
    printf("[文件系统] 文件系统已关闭\n");
}

j2me_error_t j2me_filesystem_add_root(j2me_filesystem_manager_t* manager, const char* root_path) {
    if (!manager || !root_path) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 扩展根目录数组
    manager->allowed_roots = (char**)realloc(manager->allowed_roots, 
                                             (manager->root_count + 1) * sizeof(char*));
    if (!manager->allowed_roots) {
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    manager->allowed_roots[manager->root_count] = strdup(root_path);
    if (!manager->allowed_roots[manager->root_count]) {
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    manager->root_count++;
    
    printf("[文件系统] 添加允许的根目录: %s\n", root_path);
    return J2ME_SUCCESS;
}

/**
 * @brief 在连接数组中找到空位
 */
static int find_free_connection_slot(j2me_filesystem_manager_t* manager) {
    for (int i = 0; i < manager->max_connections; i++) {
        if (manager->connections[i] == NULL) {
            return i;
        }
    }
    return -1;
}

j2me_error_t j2me_filesystem_parse_url(const char* url, char** path) {
    if (!url || !path) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    *path = NULL;
    
    // 检查file://前缀
    if (strncmp(url, "file://", 7) == 0) {
        *path = strdup(url + 7);
    } else if (url[0] == '/') {
        // 绝对路径
        *path = strdup(url);
    } else {
        // 相对路径
        *path = strdup(url);
    }
    
    if (!*path) {
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    return J2ME_SUCCESS;
}

j2me_error_t j2me_filesystem_normalize_path(const char* path, char** normalized) {
    if (!path || !normalized) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 简化实现：直接复制路径
    *normalized = strdup(path);
    if (!*normalized) {
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    return J2ME_SUCCESS;
}

bool j2me_filesystem_is_path_safe(j2me_filesystem_manager_t* manager, const char* path) {
    if (!manager || !path || !manager->security_enabled) {
        return !manager->security_enabled; // 如果安全检查关闭，则允许所有路径
    }
    
    // 检查路径是否在允许的根目录下
    for (int i = 0; i < manager->root_count; i++) {
        if (strncmp(path, manager->allowed_roots[i], strlen(manager->allowed_roots[i])) == 0) {
            return true;
        }
    }
    
    return false;
}

j2me_file_connection_t* j2me_file_connection_open(j2me_vm_t* vm, j2me_filesystem_manager_t* manager,
                                                  const char* url, j2me_file_mode_t mode) {
    if (!vm || !manager || !url) {
        return NULL;
    }
    
    int slot = find_free_connection_slot(manager);
    if (slot < 0) {
        printf("[文件系统] 文件连接数量已达上限\n");
        return NULL;
    }
    
    j2me_file_connection_t* connection = (j2me_file_connection_t*)malloc(sizeof(j2me_file_connection_t));
    if (!connection) {
        return NULL;
    }
    
    memset(connection, 0, sizeof(j2me_file_connection_t));
    
    // 解析URL
    char* path = NULL;
    j2me_error_t result = j2me_filesystem_parse_url(url, &path);
    if (result != J2ME_SUCCESS) {
        free(connection);
        return NULL;
    }
    
    // 安全检查
    if (!j2me_filesystem_is_path_safe(manager, path)) {
        printf("[文件系统] 路径不安全: %s\n", path);
        free(path);
        free(connection);
        return NULL;
    }
    
    connection->url = strdup(url);
    connection->path = path;
    connection->mode = mode;
    connection->state = FILE_CONNECTION_CLOSED;
    connection->file_handle = NULL;
    connection->dir_handle = NULL;
    connection->current_index = 0;
    
    // 获取文件信息
    struct stat st;
    if (stat(path, &st) == 0) {
        connection->info.size = st.st_size;
        connection->info.last_modified = st.st_mtime * 1000; // 转换为毫秒
        connection->info.readable = (st.st_mode & S_IRUSR) != 0;
        connection->info.writable = (st.st_mode & S_IWUSR) != 0;
        connection->info.executable = (st.st_mode & S_IXUSR) != 0;
        
        if (S_ISREG(st.st_mode)) {
            connection->info.type = FILE_TYPE_REGULAR;
        } else if (S_ISDIR(st.st_mode)) {
            connection->info.type = FILE_TYPE_DIRECTORY;
        } else if (S_ISLNK(st.st_mode)) {
            connection->info.type = FILE_TYPE_SYMLINK;
        } else {
            connection->info.type = FILE_TYPE_UNKNOWN;
        }
    } else {
        // 文件不存在
        connection->info.type = FILE_TYPE_UNKNOWN;
        connection->info.size = 0;
        connection->info.last_modified = 0;
        connection->info.readable = false;
        connection->info.writable = false;
        connection->info.executable = false;
    }
    
    connection->info.name = strdup(j2me_filesystem_get_filename(path));
    connection->info.path = strdup(path);
    
    manager->connections[slot] = connection;
    manager->active_connections++;
    manager->files_opened++;
    
    connection->state = FILE_CONNECTION_OPEN;
    
    printf("[文件系统] 打开文件连接 #%d: %s\n", slot, url);
    
    return connection;
}

void j2me_file_connection_close(j2me_file_connection_t* connection) {
    if (!connection) {
        return;
    }
    
    // 关闭文件句柄
    if (connection->file_handle) {
        fclose(connection->file_handle);
        connection->file_handle = NULL;
    }
    
    // 关闭目录句柄
    if (connection->dir_handle) {
        closedir((DIR*)connection->dir_handle);
        connection->dir_handle = NULL;
    }
    
    // 释放文件列表
    if (connection->file_list) {
        for (int i = 0; i < connection->file_count; i++) {
            if (connection->file_list[i]) {
                free(connection->file_list[i]);
            }
        }
        free(connection->file_list);
        connection->file_list = NULL;
    }
    
    // 释放内存
    if (connection->url) {
        free(connection->url);
        connection->url = NULL;
    }
    if (connection->path) {
        free(connection->path);
        connection->path = NULL;
    }
    if (connection->info.name) {
        free(connection->info.name);
        connection->info.name = NULL;
    }
    if (connection->info.path) {
        free(connection->info.path);
        connection->info.path = NULL;
    }
    
    connection->state = FILE_CONNECTION_CLOSED;
    
    printf("[文件系统] 文件连接已关闭\n");
    free(connection);
}

j2me_file_connection_state_t j2me_file_connection_get_state(j2me_file_connection_t* connection) {
    return connection ? connection->state : FILE_CONNECTION_CLOSED;
}

bool j2me_file_exists(j2me_file_connection_t* connection) {
    if (!connection || !connection->path) {
        return false;
    }
    
    return access(connection->path, F_OK) == 0;
}

bool j2me_file_is_directory(j2me_file_connection_t* connection) {
    return connection && connection->info.type == FILE_TYPE_DIRECTORY;
}

size_t j2me_file_get_size(j2me_file_connection_t* connection) {
    return connection ? connection->info.size : 0;
}

int64_t j2me_file_get_last_modified(j2me_file_connection_t* connection) {
    return connection ? connection->info.last_modified : 0;
}

j2me_error_t j2me_file_set_last_modified(j2me_file_connection_t* connection, int64_t timestamp) {
    if (!connection || !connection->path) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 简化实现：不支持设置修改时间
    printf("[文件系统] 设置修改时间功能未实现\n");
    return J2ME_ERROR_NOT_IMPLEMENTED;
}

void j2me_file_get_permissions(j2me_file_connection_t* connection, 
                               bool* readable, bool* writable, bool* executable) {
    if (!connection) {
        if (readable) *readable = false;
        if (writable) *writable = false;
        if (executable) *executable = false;
        return;
    }
    
    if (readable) *readable = connection->info.readable;
    if (writable) *writable = connection->info.writable;
    if (executable) *executable = connection->info.executable;
}

j2me_error_t j2me_file_set_permissions(j2me_file_connection_t* connection,
                                       bool readable, bool writable, bool executable) {
    if (!connection || !connection->path) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    mode_t mode = 0;
    if (readable) mode |= S_IRUSR;
    if (writable) mode |= S_IWUSR;
    if (executable) mode |= S_IXUSR;
    
    if (chmod(connection->path, mode) != 0) {
        return J2ME_ERROR_IO_EXCEPTION;
    }
    
    // 更新缓存的权限信息
    connection->info.readable = readable;
    connection->info.writable = writable;
    connection->info.executable = executable;
    
    printf("[文件系统] 设置文件权限: r=%d w=%d x=%d\n", readable, writable, executable);
    return J2ME_SUCCESS;
}

j2me_error_t j2me_file_create(j2me_file_connection_t* connection) {
    if (!connection || !connection->path) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 检查文件是否已存在
    if (j2me_file_exists(connection)) {
        return J2ME_ERROR_IO_EXCEPTION; // 文件已存在
    }
    
    // 创建文件
    FILE* file = fopen(connection->path, "w");
    if (!file) {
        return J2ME_ERROR_IO_EXCEPTION;
    }
    
    fclose(file);
    
    // 更新文件信息
    connection->info.type = FILE_TYPE_REGULAR;
    connection->info.size = 0;
    connection->info.last_modified = time(NULL) * 1000;
    
    printf("[文件系统] 创建文件: %s\n", connection->path);
    return J2ME_SUCCESS;
}

j2me_error_t j2me_file_mkdir(j2me_file_connection_t* connection) {
    if (!connection || !connection->path) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    if (mkdir(connection->path, 0755) != 0) {
        return J2ME_ERROR_IO_EXCEPTION;
    }
    
    // 更新文件信息
    connection->info.type = FILE_TYPE_DIRECTORY;
    connection->info.size = 0;
    connection->info.last_modified = time(NULL) * 1000;
    
    printf("[文件系统] 创建目录: %s\n", connection->path);
    return J2ME_SUCCESS;
}

j2me_error_t j2me_file_delete(j2me_file_connection_t* connection) {
    if (!connection || !connection->path) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    int result;
    if (j2me_file_is_directory(connection)) {
        result = rmdir(connection->path);
    } else {
        result = unlink(connection->path);
    }
    
    if (result != 0) {
        return J2ME_ERROR_IO_EXCEPTION;
    }
    
    printf("[文件系统] 删除文件: %s\n", connection->path);
    return J2ME_SUCCESS;
}

j2me_error_t j2me_file_rename(j2me_file_connection_t* connection, const char* new_name) {
    if (!connection || !connection->path || !new_name) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 构建新路径
    char* dir = j2me_filesystem_get_directory(connection->path);
    char* new_path = j2me_filesystem_join_path(dir, new_name);
    
    if (rename(connection->path, new_path) != 0) {
        free(dir);
        free(new_path);
        return J2ME_ERROR_IO_EXCEPTION;
    }
    
    // 更新连接信息
    free(connection->path);
    connection->path = new_path;
    
    if (connection->info.name) {
        free(connection->info.name);
    }
    connection->info.name = strdup(new_name);
    
    free(dir);
    
    printf("[文件系统] 重命名文件: %s\n", new_name);
    return J2ME_SUCCESS;
}

j2me_error_t j2me_file_truncate(j2me_file_connection_t* connection, size_t size) {
    if (!connection || !connection->path) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    if (truncate(connection->path, size) != 0) {
        return J2ME_ERROR_IO_EXCEPTION;
    }
    
    connection->info.size = size;
    
    printf("[文件系统] 截断文件到 %zu bytes\n", size);
    return J2ME_SUCCESS;
}

j2me_error_t j2me_file_read(j2me_file_connection_t* connection, uint8_t* buffer, 
                            size_t size, size_t* bytes_read) {
    if (!connection || !buffer || !bytes_read) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    *bytes_read = 0;
    
    // 打开文件 (如果还未打开)
    if (!connection->file_handle) {
        connection->file_handle = fopen(connection->path, "rb");
        if (!connection->file_handle) {
            return J2ME_ERROR_IO_EXCEPTION;
        }
    }
    
    *bytes_read = fread(buffer, 1, size, connection->file_handle);
    
    return J2ME_SUCCESS;
}

j2me_error_t j2me_file_write(j2me_file_connection_t* connection, const uint8_t* data,
                             size_t size, size_t* bytes_written) {
    if (!connection || !data || !bytes_written) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    *bytes_written = 0;
    
    // 打开文件 (如果还未打开)
    if (!connection->file_handle) {
        const char* mode = (connection->mode == FILE_MODE_WRITE) ? "wb" : "ab";
        connection->file_handle = fopen(connection->path, mode);
        if (!connection->file_handle) {
            return J2ME_ERROR_IO_EXCEPTION;
        }
    }
    
    *bytes_written = fwrite(data, 1, size, connection->file_handle);
    
    return J2ME_SUCCESS;
}

j2me_error_t j2me_file_flush(j2me_file_connection_t* connection) {
    if (!connection || !connection->file_handle) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    if (fflush(connection->file_handle) != 0) {
        return J2ME_ERROR_IO_EXCEPTION;
    }
    
    return J2ME_SUCCESS;
}

j2me_error_t j2me_file_seek(j2me_file_connection_t* connection, size_t position) {
    if (!connection || !connection->file_handle) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    if (fseek(connection->file_handle, position, SEEK_SET) != 0) {
        return J2ME_ERROR_IO_EXCEPTION;
    }
    
    return J2ME_SUCCESS;
}

size_t j2me_file_tell(j2me_file_connection_t* connection) {
    if (!connection || !connection->file_handle) {
        return 0;
    }
    
    long pos = ftell(connection->file_handle);
    return (pos >= 0) ? (size_t)pos : 0;
}

j2me_error_t j2me_file_list_directory(j2me_file_connection_t* connection, 
                                      const char* filter, bool include_hidden) {
    if (!connection || !j2me_file_is_directory(connection)) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 释放之前的文件列表
    if (connection->file_list) {
        for (int i = 0; i < connection->file_count; i++) {
            if (connection->file_list[i]) {
                free(connection->file_list[i]);
            }
        }
        free(connection->file_list);
        connection->file_list = NULL;
        connection->file_count = 0;
    }
    
    DIR* dir = opendir(connection->path);
    if (!dir) {
        return J2ME_ERROR_IO_EXCEPTION;
    }
    
    // 计算文件数量
    struct dirent* entry;
    int count = 0;
    while ((entry = readdir(dir)) != NULL) {
        // 跳过 . 和 ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // 跳过隐藏文件 (如果不包含)
        if (!include_hidden && entry->d_name[0] == '.') {
            continue;
        }
        
        // 应用过滤器 (简化实现)
        if (filter && strstr(entry->d_name, filter) == NULL) {
            continue;
        }
        
        count++;
    }
    
    // 分配文件列表
    connection->file_list = (char**)malloc(count * sizeof(char*));
    if (!connection->file_list) {
        closedir(dir);
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    // 重新读取并存储文件名
    rewinddir(dir);
    connection->file_count = 0;
    while ((entry = readdir(dir)) != NULL && connection->file_count < count) {
        // 应用相同的过滤条件
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        if (!include_hidden && entry->d_name[0] == '.') {
            continue;
        }
        
        if (filter && strstr(entry->d_name, filter) == NULL) {
            continue;
        }
        
        connection->file_list[connection->file_count] = strdup(entry->d_name);
        connection->file_count++;
    }
    
    closedir(dir);
    connection->current_index = 0;
    
    printf("[文件系统] 列出目录内容: %d 个文件\n", connection->file_count);
    return J2ME_SUCCESS;
}

int j2me_file_get_file_count(j2me_file_connection_t* connection) {
    return connection ? connection->file_count : 0;
}

char* j2me_file_get_file_name(j2me_file_connection_t* connection, int index) {
    if (!connection || index < 0 || index >= connection->file_count) {
        return NULL;
    }
    
    return strdup(connection->file_list[index]);
}

bool j2me_file_has_more_files(j2me_file_connection_t* connection) {
    return connection && connection->current_index < connection->file_count;
}

char* j2me_file_get_next_file(j2me_file_connection_t* connection) {
    if (!j2me_file_has_more_files(connection)) {
        return NULL;
    }
    
    char* filename = strdup(connection->file_list[connection->current_index]);
    connection->current_index++;
    
    return filename;
}

const char* j2me_filesystem_get_extension(const char* filename) {
    if (!filename) {
        return "";
    }
    
    const char* dot = strrchr(filename, '.');
    return dot ? dot + 1 : "";
}

const char* j2me_filesystem_get_filename(const char* path) {
    if (!path) {
        return "";
    }
    
    const char* slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

char* j2me_filesystem_get_directory(const char* path) {
    if (!path) {
        return NULL;
    }
    
    char* path_copy = strdup(path);
    if (!path_copy) {
        return NULL;
    }
    
    char* dir = dirname(path_copy);
    char* result = strdup(dir);
    
    free(path_copy);
    return result;
}

char* j2me_filesystem_join_path(const char* dir, const char* filename) {
    if (!dir || !filename) {
        return NULL;
    }
    
    size_t dir_len = strlen(dir);
    size_t filename_len = strlen(filename);
    size_t total_len = dir_len + filename_len + 2; // +1 for '/', +1 for '\0'
    
    char* result = (char*)malloc(total_len);
    if (!result) {
        return NULL;
    }
    
    strcpy(result, dir);
    if (dir_len > 0 && dir[dir_len - 1] != '/') {
        strcat(result, "/");
    }
    strcat(result, filename);
    
    return result;
}

size_t j2me_filesystem_get_available_space(const char* path) {
    if (!path) {
        return 0;
    }
    
    struct statvfs st;
    if (statvfs(path, &st) != 0) {
        return 0;
    }
    
    return st.f_bavail * st.f_frsize;
}

size_t j2me_filesystem_get_total_space(const char* path) {
    if (!path) {
        return 0;
    }
    
    struct statvfs st;
    if (statvfs(path, &st) != 0) {
        return 0;
    }
    
    return st.f_blocks * st.f_frsize;
}

size_t j2me_filesystem_get_used_space(const char* path) {
    size_t total = j2me_filesystem_get_total_space(path);
    size_t available = j2me_filesystem_get_available_space(path);
    
    return (total > available) ? (total - available) : 0;
}

void j2me_filesystem_get_statistics(j2me_filesystem_manager_t* manager,
                                    size_t* bytes_read, size_t* bytes_written,
                                    int* files_opened, int* files_created, int* files_deleted) {
    if (!manager) {
        return;
    }
    
    if (bytes_read) *bytes_read = manager->bytes_read;
    if (bytes_written) *bytes_written = manager->bytes_written;
    if (files_opened) *files_opened = manager->files_opened;
    if (files_created) *files_created = manager->files_created;
    if (files_deleted) *files_deleted = manager->files_deleted;
}

void j2me_filesystem_update(j2me_filesystem_manager_t* manager) {
    if (!manager) {
        return;
    }
    
    // 检查文件连接状态
    for (int i = 0; i < manager->max_connections; i++) {
        j2me_file_connection_t* connection = manager->connections[i];
        if (connection && connection->state == FILE_CONNECTION_OPEN) {
            // 更新文件信息 (如果需要)
        }
    }
}

void j2me_filesystem_close_all(j2me_filesystem_manager_t* manager) {
    if (!manager) {
        return;
    }
    
    for (int i = 0; i < manager->max_connections; i++) {
        if (manager->connections[i]) {
            j2me_file_connection_close(manager->connections[i]);
            manager->connections[i] = NULL;
        }
    }
    
    manager->active_connections = 0;
    
    printf("[文件系统] 所有文件连接已关闭\n");
}