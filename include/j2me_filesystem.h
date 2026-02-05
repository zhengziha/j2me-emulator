#ifndef J2ME_FILESYSTEM_H
#define J2ME_FILESYSTEM_H

#include "j2me_types.h"
#include "j2me_object.h"
#include <stdio.h>
#include <sys/types.h>
#include <pthread.h>
#include <zlib.h>

/**
 * @file j2me_filesystem.h
 * @brief J2ME文件系统
 * 
 * 实现JSR-75 FileConnection API 的文件系统功能
 */

// 前向声明
typedef struct j2me_filesystem_manager j2me_filesystem_manager_t;
typedef struct j2me_file_connection j2me_file_connection_t;

// 文件锁类型
typedef enum {
    FILE_LOCK_NONE = 0,
    FILE_LOCK_SHARED,           // 共享锁 (读锁)
    FILE_LOCK_EXCLUSIVE         // 排他锁 (写锁)
} j2me_file_lock_type_t;

// 文件事件类型
typedef enum {
    FILE_EVENT_CREATED = 1,
    FILE_EVENT_MODIFIED = 2,
    FILE_EVENT_DELETED = 4,
    FILE_EVENT_MOVED = 8
} j2me_file_event_type_t;

// 压缩类型
typedef enum {
    COMPRESSION_NONE = 0,
    COMPRESSION_GZIP,
    COMPRESSION_ZIP
} j2me_compression_type_t;

// 文件事件回调函数
typedef void (*j2me_file_event_callback_t)(const char* path, j2me_file_event_type_t event, void* user_data);

// 文件类型
typedef enum {
    FILE_TYPE_UNKNOWN = 0,
    FILE_TYPE_REGULAR,
    FILE_TYPE_DIRECTORY,
    FILE_TYPE_SYMLINK
} j2me_file_type_t;

// 文件访问模式
typedef enum {
    FILE_MODE_READ = 1,
    FILE_MODE_WRITE = 2,
    FILE_MODE_READ_WRITE = 3
} j2me_file_mode_t;

// 文件连接状态
typedef enum {
    FILE_CONNECTION_CLOSED = 0,
    FILE_CONNECTION_OPEN,
    FILE_CONNECTION_ERROR
} j2me_file_connection_state_t;

// 文件信息结构
typedef struct {
    char* name;                     // 文件名
    char* path;                     // 完整路径
    j2me_file_type_t type;          // 文件类型
    size_t size;                    // 文件大小
    int64_t last_modified;          // 最后修改时间
    bool readable;                  // 是否可读
    bool writable;                  // 是否可写
    bool executable;                // 是否可执行
    bool hidden;                    // 是否隐藏
    
    // 高级属性
    j2me_file_lock_type_t lock_type; // 锁类型
    j2me_compression_type_t compression; // 压缩类型
    mode_t permissions;             // POSIX权限
    uid_t owner_uid;                // 拥有者用户ID
    gid_t owner_gid;                // 拥有者组ID
} j2me_file_info_t;

// 文件连接结构
struct j2me_file_connection {
    j2me_object_header_t header;        // 对象头
    j2me_file_connection_state_t state; // 连接状态
    j2me_file_mode_t mode;              // 访问模式
    char* url;                          // 文件URL
    char* path;                         // 本地路径
    FILE* file_handle;                  // 文件句柄
    int fd;                             // 文件描述符 (用于锁定)
    j2me_file_info_t info;              // 文件信息
    
    // 目录遍历
    void* dir_handle;                   // 目录句柄 (DIR*)
    char** file_list;                   // 文件列表
    int file_count;                     // 文件数量
    int current_index;                  // 当前索引
    
    // 高级功能
    j2me_file_lock_type_t lock_type;    // 当前锁类型
    bool compressed;                    // 是否压缩
    z_stream* compression_stream;       // 压缩流
};

// 文件系统管理器
struct j2me_filesystem_manager {
    bool initialized;                   // 是否已初始化
    int max_connections;                // 最大连接数
    j2me_file_connection_t** connections; // 连接数组
    int active_connections;             // 活跃连接数
    
    // 安全设置
    char** allowed_roots;               // 允许的根目录
    int root_count;                     // 根目录数量
    bool security_enabled;              // 是否启用安全检查
    
    // 高级功能
    void* file_locks;                   // 文件锁数组
    int lock_count;                     // 锁数量
    void* file_monitors;                // 文件监控数组
    int monitor_count;                  // 监控数量
    pthread_mutex_t lock_mutex;         // 锁管理互斥量
    
    // 统计信息
    size_t bytes_read;                  // 读取字节数
    size_t bytes_written;               // 写入字节数
    int files_opened;                   // 打开文件数
    int files_created;                  // 创建文件数
    int files_deleted;                  // 删除文件数
};

/**
 * @brief 创建文件系统管理器
 * @param vm 虚拟机实例
 * @return 文件系统管理器指针
 */
j2me_filesystem_manager_t* j2me_filesystem_manager_create(j2me_vm_t* vm);

/**
 * @brief 销毁文件系统管理器
 * @param manager 文件系统管理器
 */
void j2me_filesystem_manager_destroy(j2me_filesystem_manager_t* manager);

/**
 * @brief 初始化文件系统
 * @param manager 文件系统管理器
 * @return 错误码
 */
j2me_error_t j2me_filesystem_initialize(j2me_filesystem_manager_t* manager);

/**
 * @brief 关闭文件系统
 * @param manager 文件系统管理器
 */
void j2me_filesystem_shutdown(j2me_filesystem_manager_t* manager);

/**
 * @brief 添加允许的根目录
 * @param manager 文件系统管理器
 * @param root_path 根目录路径
 * @return 错误码
 */
j2me_error_t j2me_filesystem_add_root(j2me_filesystem_manager_t* manager, const char* root_path);

/**
 * @brief 打开文件连接
 * @param vm 虚拟机实例
 * @param manager 文件系统管理器
 * @param url 文件URL
 * @param mode 访问模式
 * @return 文件连接指针
 */
j2me_file_connection_t* j2me_file_connection_open(j2me_vm_t* vm, j2me_filesystem_manager_t* manager,
                                                  const char* url, j2me_file_mode_t mode);

/**
 * @brief 关闭文件连接
 * @param connection 文件连接
 */
void j2me_file_connection_close(j2me_file_connection_t* connection);

/**
 * @brief 获取连接状态
 * @param connection 文件连接
 * @return 连接状态
 */
j2me_file_connection_state_t j2me_file_connection_get_state(j2me_file_connection_t* connection);

/**
 * @brief 检查文件是否存在
 * @param connection 文件连接
 * @return 是否存在
 */
bool j2me_file_exists(j2me_file_connection_t* connection);

/**
 * @brief 检查是否为目录
 * @param connection 文件连接
 * @return 是否为目录
 */
bool j2me_file_is_directory(j2me_file_connection_t* connection);

/**
 * @brief 获取文件大小
 * @param connection 文件连接
 * @return 文件大小
 */
size_t j2me_file_get_size(j2me_file_connection_t* connection);

/**
 * @brief 获取最后修改时间
 * @param connection 文件连接
 * @return 修改时间 (毫秒时间戳)
 */
int64_t j2me_file_get_last_modified(j2me_file_connection_t* connection);

/**
 * @brief 设置最后修改时间
 * @param connection 文件连接
 * @param timestamp 时间戳 (毫秒)
 * @return 错误码
 */
j2me_error_t j2me_file_set_last_modified(j2me_file_connection_t* connection, int64_t timestamp);

/**
 * @brief 检查文件权限
 * @param connection 文件连接
 * @param readable 是否可读 (输出参数)
 * @param writable 是否可写 (输出参数)
 * @param executable 是否可执行 (输出参数)
 */
void j2me_file_get_permissions(j2me_file_connection_t* connection, 
                               bool* readable, bool* writable, bool* executable);

/**
 * @brief 设置文件权限
 * @param connection 文件连接
 * @param readable 是否可读
 * @param writable 是否可写
 * @param executable 是否可执行
 * @return 错误码
 */
j2me_error_t j2me_file_set_permissions(j2me_file_connection_t* connection,
                                       bool readable, bool writable, bool executable);

/**
 * @brief 创建文件
 * @param connection 文件连接
 * @return 错误码
 */
j2me_error_t j2me_file_create(j2me_file_connection_t* connection);

/**
 * @brief 创建目录
 * @param connection 文件连接
 * @return 错误码
 */
j2me_error_t j2me_file_mkdir(j2me_file_connection_t* connection);

/**
 * @brief 删除文件或目录
 * @param connection 文件连接
 * @return 错误码
 */
j2me_error_t j2me_file_delete(j2me_file_connection_t* connection);

/**
 * @brief 重命名文件
 * @param connection 文件连接
 * @param new_name 新名称
 * @return 错误码
 */
j2me_error_t j2me_file_rename(j2me_file_connection_t* connection, const char* new_name);

/**
 * @brief 截断文件
 * @param connection 文件连接
 * @param size 新大小
 * @return 错误码
 */
j2me_error_t j2me_file_truncate(j2me_file_connection_t* connection, size_t size);

// 文件读写操作

/**
 * @brief 读取文件数据
 * @param connection 文件连接
 * @param buffer 读取缓冲区
 * @param size 要读取的字节数
 * @param bytes_read 实际读取字节数 (输出参数)
 * @return 错误码
 */
j2me_error_t j2me_file_read(j2me_file_connection_t* connection, uint8_t* buffer, 
                            size_t size, size_t* bytes_read);

/**
 * @brief 写入文件数据
 * @param connection 文件连接
 * @param data 要写入的数据
 * @param size 数据大小
 * @param bytes_written 实际写入字节数 (输出参数)
 * @return 错误码
 */
j2me_error_t j2me_file_write(j2me_file_connection_t* connection, const uint8_t* data,
                             size_t size, size_t* bytes_written);

/**
 * @brief 刷新文件缓冲区
 * @param connection 文件连接
 * @return 错误码
 */
j2me_error_t j2me_file_flush(j2me_file_connection_t* connection);

/**
 * @brief 设置文件位置
 * @param connection 文件连接
 * @param position 新位置
 * @return 错误码
 */
j2me_error_t j2me_file_seek(j2me_file_connection_t* connection, size_t position);

/**
 * @brief 获取文件位置
 * @param connection 文件连接
 * @return 当前位置
 */
size_t j2me_file_tell(j2me_file_connection_t* connection);

// 目录操作

/**
 * @brief 列出目录内容
 * @param connection 文件连接 (必须是目录)
 * @param filter 文件名过滤器 (可选)
 * @param include_hidden 是否包含隐藏文件
 * @return 错误码
 */
j2me_error_t j2me_file_list_directory(j2me_file_connection_t* connection, 
                                      const char* filter, bool include_hidden);

/**
 * @brief 获取目录中的文件数量
 * @param connection 文件连接
 * @return 文件数量
 */
int j2me_file_get_file_count(j2me_file_connection_t* connection);

/**
 * @brief 获取目录中的文件名
 * @param connection 文件连接
 * @param index 文件索引
 * @return 文件名 (需要释放)
 */
char* j2me_file_get_file_name(j2me_file_connection_t* connection, int index);

/**
 * @brief 检查是否还有更多文件
 * @param connection 文件连接
 * @return 是否有更多文件
 */
bool j2me_file_has_more_files(j2me_file_connection_t* connection);

/**
 * @brief 获取下一个文件名
 * @param connection 文件连接
 * @return 文件名 (需要释放)
 */
char* j2me_file_get_next_file(j2me_file_connection_t* connection);

// 路径工具函数

/**
 * @brief 解析文件URL
 * @param url 文件URL
 * @param path 本地路径 (输出参数，需要释放)
 * @return 错误码
 */
j2me_error_t j2me_filesystem_parse_url(const char* url, char** path);

/**
 * @brief 规范化路径
 * @param path 输入路径
 * @param normalized 规范化路径 (输出参数，需要释放)
 * @return 错误码
 */
j2me_error_t j2me_filesystem_normalize_path(const char* path, char** normalized);

/**
 * @brief 检查路径是否安全
 * @param manager 文件系统管理器
 * @param path 要检查的路径
 * @return 是否安全
 */
bool j2me_filesystem_is_path_safe(j2me_filesystem_manager_t* manager, const char* path);

/**
 * @brief 获取文件扩展名
 * @param filename 文件名
 * @return 扩展名 (不需要释放)
 */
const char* j2me_filesystem_get_extension(const char* filename);

/**
 * @brief 获取文件名 (不含路径)
 * @param path 完整路径
 * @return 文件名 (不需要释放)
 */
const char* j2me_filesystem_get_filename(const char* path);

/**
 * @brief 获取目录路径
 * @param path 完整路径
 * @return 目录路径 (需要释放)
 */
char* j2me_filesystem_get_directory(const char* path);

/**
 * @brief 连接路径
 * @param dir 目录路径
 * @param filename 文件名
 * @return 完整路径 (需要释放)
 */
char* j2me_filesystem_join_path(const char* dir, const char* filename);

// 系统信息

/**
 * @brief 获取可用磁盘空间
 * @param path 路径
 * @return 可用空间 (字节)
 */
size_t j2me_filesystem_get_available_space(const char* path);

/**
 * @brief 获取总磁盘空间
 * @param path 路径
 * @return 总空间 (字节)
 */
size_t j2me_filesystem_get_total_space(const char* path);

/**
 * @brief 获取已用磁盘空间
 * @param path 路径
 * @return 已用空间 (字节)
 */
size_t j2me_filesystem_get_used_space(const char* path);

/**
 * @brief 获取文件系统统计信息
 * @param manager 文件系统管理器
 * @param bytes_read 读取字节数 (输出参数)
 * @param bytes_written 写入字节数 (输出参数)
 * @param files_opened 打开文件数 (输出参数)
 * @param files_created 创建文件数 (输出参数)
 * @param files_deleted 删除文件数 (输出参数)
 */
void j2me_filesystem_get_statistics(j2me_filesystem_manager_t* manager,
                                    size_t* bytes_read, size_t* bytes_written,
                                    int* files_opened, int* files_created, int* files_deleted);

/**
 * @brief 更新文件系统 (每帧调用)
 * @param manager 文件系统管理器
 */
void j2me_filesystem_update(j2me_filesystem_manager_t* manager);

/**
 * @brief 关闭所有文件连接
 * @param manager 文件系统管理器
 */
void j2me_filesystem_close_all(j2me_filesystem_manager_t* manager);

// 高级功能 - 文件锁定

/**
 * @brief 锁定文件
 * @param connection 文件连接
 * @param lock_type 锁类型
 * @return 错误码
 */
j2me_error_t j2me_file_lock(j2me_file_connection_t* connection, j2me_file_lock_type_t lock_type);

/**
 * @brief 解锁文件
 * @param connection 文件连接
 * @return 错误码
 */
j2me_error_t j2me_file_unlock(j2me_file_connection_t* connection);

/**
 * @brief 检查文件是否被锁定
 * @param connection 文件连接
 * @return 锁类型
 */
j2me_file_lock_type_t j2me_file_get_lock_type(j2me_file_connection_t* connection);

// 高级功能 - 文件压缩

/**
 * @brief 启用文件压缩
 * @param connection 文件连接
 * @param compression_type 压缩类型
 * @return 错误码
 */
j2me_error_t j2me_file_enable_compression(j2me_file_connection_t* connection, 
                                          j2me_compression_type_t compression_type);

/**
 * @brief 禁用文件压缩
 * @param connection 文件连接
 * @return 错误码
 */
j2me_error_t j2me_file_disable_compression(j2me_file_connection_t* connection);

/**
 * @brief 压缩文件
 * @param source_path 源文件路径
 * @param dest_path 目标文件路径
 * @param compression_type 压缩类型
 * @return 错误码
 */
j2me_error_t j2me_file_compress(const char* source_path, const char* dest_path, 
                                j2me_compression_type_t compression_type);

/**
 * @brief 解压文件
 * @param source_path 源文件路径
 * @param dest_path 目标文件路径
 * @return 错误码
 */
j2me_error_t j2me_file_decompress(const char* source_path, const char* dest_path);

// 高级功能 - 文件监控

/**
 * @brief 添加文件监控
 * @param manager 文件系统管理器
 * @param path 监控路径
 * @param events 监控事件类型
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return 错误码
 */
j2me_error_t j2me_filesystem_add_monitor(j2me_filesystem_manager_t* manager,
                                         const char* path, int events,
                                         j2me_file_event_callback_t callback,
                                         void* user_data);

/**
 * @brief 移除文件监控
 * @param manager 文件系统管理器
 * @param path 监控路径
 * @return 错误码
 */
j2me_error_t j2me_filesystem_remove_monitor(j2me_filesystem_manager_t* manager, const char* path);

// 高级功能 - 扩展属性

/**
 * @brief 设置文件扩展属性
 * @param connection 文件连接
 * @param name 属性名
 * @param value 属性值
 * @param size 值大小
 * @return 错误码
 */
j2me_error_t j2me_file_set_attribute(j2me_file_connection_t* connection,
                                     const char* name, const void* value, size_t size);

/**
 * @brief 获取文件扩展属性
 * @param connection 文件连接
 * @param name 属性名
 * @param value 属性值缓冲区
 * @param size 缓冲区大小
 * @return 实际属性值大小，-1表示错误
 */
ssize_t j2me_file_get_attribute(j2me_file_connection_t* connection,
                                const char* name, void* value, size_t size);

/**
 * @brief 删除文件扩展属性
 * @param connection 文件连接
 * @param name 属性名
 * @return 错误码
 */
j2me_error_t j2me_file_remove_attribute(j2me_file_connection_t* connection, const char* name);

/**
 * @brief 列出文件扩展属性
 * @param connection 文件连接
 * @param names 属性名列表缓冲区
 * @param size 缓冲区大小
 * @return 实际列表大小，-1表示错误
 */
ssize_t j2me_file_list_attributes(j2me_file_connection_t* connection, char* names, size_t size);

#endif // J2ME_FILESYSTEM_H