#ifndef J2ME_JAR_H
#define J2ME_JAR_H

#include "j2me_types.h"
#include "j2me_object.h"
#include <stdio.h>
#include <stdint.h>

/**
 * @file j2me_jar.h
 * @brief J2ME JAR文件解析器
 * 
 * 实现JAR文件的解析、资源提取和MIDlet管理功能
 */

// 前向声明
typedef struct j2me_jar_file j2me_jar_file_t;
typedef struct j2me_jar_entry j2me_jar_entry_t;
typedef struct j2me_midlet_suite j2me_midlet_suite_t;
typedef struct j2me_midlet j2me_midlet_t;
typedef struct j2me_midlet_executor j2me_midlet_executor_t;
typedef struct j2me_midlet_instance j2me_midlet_instance_t;

// JAR文件条目类型
typedef enum {
    JAR_ENTRY_FILE = 0,
    JAR_ENTRY_DIRECTORY,
    JAR_ENTRY_CLASS,
    JAR_ENTRY_RESOURCE,
    JAR_ENTRY_MANIFEST
} j2me_jar_entry_type_t;

// MIDlet状态
typedef enum {
    MIDLET_STATE_PAUSED = 0,
    MIDLET_STATE_ACTIVE,
    MIDLET_STATE_DESTROYED
} j2me_midlet_state_t;

// JAR文件条目结构
struct j2me_jar_entry {
    char* name;                         // 条目名称
    char* full_path;                    // 完整路径
    j2me_jar_entry_type_t type;         // 条目类型
    size_t compressed_size;             // 压缩大小
    size_t uncompressed_size;           // 未压缩大小
    uint32_t crc32;                     // CRC32校验和
    uint16_t compression_method;        // 压缩方法
    uint8_t* data;                      // 数据内容
    bool loaded;                        // 是否已加载
    
    // ZIP文件偏移信息
    long file_offset;                   // 文件中的偏移
    uint16_t filename_length;           // 文件名长度
    uint16_t extra_field_length;        // 额外字段长度
};

// MIDlet信息结构
struct j2me_midlet {
    char* name;                         // MIDlet名称
    char* class_name;                   // 主类名
    char* icon;                         // 图标路径
    char* description;                  // 描述
    j2me_midlet_state_t state;          // 当前状态
    
    // 运行时信息
    void* main_class;                   // 主类对象
    void* display;                      // 显示对象
    bool started;                       // 是否已启动
    
    // 执行器和实例
    j2me_midlet_executor_t* executor;   // MIDlet执行器
    j2me_midlet_instance_t* instance;   // MIDlet实例
    j2me_jar_file_t* jar_file;          // JAR文件引用
};

// MIDlet套件结构
struct j2me_midlet_suite {
    char* name;                         // 套件名称
    char* vendor;                       // 供应商
    char* version;                      // 版本
    char* description;                  // 描述
    char* jar_url;                      // JAR文件URL
    char* jad_url;                      // JAD文件URL (可选)
    
    // MIDlet列表
    j2me_midlet_t** midlets;            // MIDlet数组
    int midlet_count;                   // MIDlet数量
    
    // 权限和配置
    char** permissions;                 // 权限列表
    int permission_count;               // 权限数量
    char* microedition_profile;        // 配置文件
    char* microedition_configuration;   // 配置
    
    // 资源管理
    j2me_jar_file_t* jar_file;          // JAR文件引用
};

// JAR文件结构
struct j2me_jar_file {
    char* filename;                     // 文件名
    FILE* file;                         // 文件句柄
    size_t file_size;                   // 文件大小
    
    // ZIP文件信息
    uint16_t entry_count;               // 条目数量
    j2me_jar_entry_t** entries;         // 条目数组
    
    // 清单文件信息
    char* manifest_content;             // 清单文件内容
    j2me_midlet_suite_t* midlet_suite;  // MIDlet套件
    
    // 缓存和索引
    void* class_cache;                  // 类缓存
    void* resource_cache;               // 资源缓存
    bool parsed;                        // 是否已解析
};

/**
 * @brief 打开JAR文件
 * @param filename JAR文件路径
 * @return JAR文件对象指针
 */
j2me_jar_file_t* j2me_jar_open(const char* filename);

/**
 * @brief 关闭JAR文件
 * @param jar_file JAR文件对象
 */
void j2me_jar_close(j2me_jar_file_t* jar_file);

/**
 * @brief 解析JAR文件
 * @param jar_file JAR文件对象
 * @return 错误码
 */
j2me_error_t j2me_jar_parse(j2me_jar_file_t* jar_file);

/**
 * @brief 获取JAR文件条目数量
 * @param jar_file JAR文件对象
 * @return 条目数量
 */
int j2me_jar_get_entry_count(j2me_jar_file_t* jar_file);

/**
 * @brief 根据名称查找JAR条目
 * @param jar_file JAR文件对象
 * @param name 条目名称
 * @return JAR条目指针
 */
j2me_jar_entry_t* j2me_jar_find_entry(j2me_jar_file_t* jar_file, const char* name);

/**
 * @brief 根据索引获取JAR条目
 * @param jar_file JAR文件对象
 * @param index 条目索引
 * @return JAR条目指针
 */
j2me_jar_entry_t* j2me_jar_get_entry(j2me_jar_file_t* jar_file, int index);

/**
 * @brief 加载JAR条目数据
 * @param jar_file JAR文件对象
 * @param entry JAR条目
 * @return 错误码
 */
j2me_error_t j2me_jar_load_entry(j2me_jar_file_t* jar_file, j2me_jar_entry_t* entry);

/**
 * @brief 提取JAR条目到文件
 * @param jar_file JAR文件对象
 * @param entry JAR条目
 * @param output_path 输出文件路径
 * @return 错误码
 */
j2me_error_t j2me_jar_extract_entry(j2me_jar_file_t* jar_file, j2me_jar_entry_t* entry, const char* output_path);

/**
 * @brief 提取所有JAR条目
 * @param jar_file JAR文件对象
 * @param output_dir 输出目录
 * @return 错误码
 */
j2me_error_t j2me_jar_extract_all(j2me_jar_file_t* jar_file, const char* output_dir);

// MIDlet套件管理

/**
 * @brief 解析清单文件
 * @param jar_file JAR文件对象
 * @return 错误码
 */
j2me_error_t j2me_jar_parse_manifest(j2me_jar_file_t* jar_file);

/**
 * @brief 获取MIDlet套件信息
 * @param jar_file JAR文件对象
 * @return MIDlet套件指针
 */
j2me_midlet_suite_t* j2me_jar_get_midlet_suite(j2me_jar_file_t* jar_file);

/**
 * @brief 创建MIDlet套件
 * @param jar_file JAR文件对象
 * @return MIDlet套件指针
 */
j2me_midlet_suite_t* j2me_midlet_suite_create(j2me_jar_file_t* jar_file);

/**
 * @brief 销毁MIDlet套件
 * @param suite MIDlet套件
 */
void j2me_midlet_suite_destroy(j2me_midlet_suite_t* suite);

/**
 * @brief 获取MIDlet数量
 * @param suite MIDlet套件
 * @return MIDlet数量
 */
int j2me_midlet_suite_get_midlet_count(j2me_midlet_suite_t* suite);

/**
 * @brief 根据索引获取MIDlet
 * @param suite MIDlet套件
 * @param index MIDlet索引
 * @return MIDlet指针
 */
j2me_midlet_t* j2me_midlet_suite_get_midlet(j2me_midlet_suite_t* suite, int index);

/**
 * @brief 根据名称查找MIDlet
 * @param suite MIDlet套件
 * @param name MIDlet名称
 * @return MIDlet指针
 */
j2me_midlet_t* j2me_midlet_suite_find_midlet(j2me_midlet_suite_t* suite, const char* name);

// MIDlet生命周期管理

/**
 * @brief 启动MIDlet
 * @param vm 虚拟机实例
 * @param midlet MIDlet对象
 * @return 错误码
 */
j2me_error_t j2me_midlet_start(j2me_vm_t* vm, j2me_midlet_t* midlet);

/**
 * @brief 暂停MIDlet
 * @param midlet MIDlet对象
 * @return 错误码
 */
j2me_error_t j2me_midlet_pause(j2me_midlet_t* midlet);

/**
 * @brief 恢复MIDlet
 * @param midlet MIDlet对象
 * @return 错误码
 */
j2me_error_t j2me_midlet_resume(j2me_midlet_t* midlet);

/**
 * @brief 销毁MIDlet
 * @param midlet MIDlet对象
 * @return 错误码
 */
j2me_error_t j2me_midlet_destroy(j2me_midlet_t* midlet);

/**
 * @brief 获取MIDlet状态
 * @param midlet MIDlet对象
 * @return MIDlet状态
 */
j2me_midlet_state_t j2me_midlet_get_state(j2me_midlet_t* midlet);

// 工具函数

/**
 * @brief 获取条目类型名称
 * @param type 条目类型
 * @return 类型名称字符串
 */
const char* j2me_jar_get_entry_type_name(j2me_jar_entry_type_t type);

/**
 * @brief 获取MIDlet状态名称
 * @param state MIDlet状态
 * @return 状态名称字符串
 */
const char* j2me_midlet_get_state_name(j2me_midlet_state_t state);

/**
 * @brief 验证JAR文件完整性
 * @param jar_file JAR文件对象
 * @return 是否有效
 */
bool j2me_jar_verify(j2me_jar_file_t* jar_file);

/**
 * @brief 获取JAR文件统计信息
 * @param jar_file JAR文件对象
 * @param total_entries 总条目数 (输出参数)
 * @param total_size 总大小 (输出参数)
 * @param compressed_size 压缩大小 (输出参数)
 */
void j2me_jar_get_statistics(j2me_jar_file_t* jar_file, 
                             int* total_entries, size_t* total_size, size_t* compressed_size);

#endif // J2ME_JAR_H