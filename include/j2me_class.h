#ifndef J2ME_CLASS_H
#define J2ME_CLASS_H

#include "j2me_types.h"
#include <stddef.h>

/**
 * @file j2me_class.h
 * @brief J2ME类加载和管理系统
 * 
 * 定义Java类的内存表示和类加载器接口
 */

// 前向声明
typedef struct j2me_class j2me_class_t;
typedef struct j2me_method j2me_method_t;
typedef struct j2me_field j2me_field_t;
typedef struct j2me_constant_pool j2me_constant_pool_t;
typedef struct j2me_class_loader j2me_class_loader_t;

// 访问标志
#define ACC_PUBLIC      0x0001
#define ACC_PRIVATE     0x0002
#define ACC_PROTECTED   0x0004
#define ACC_STATIC      0x0008
#define ACC_FINAL       0x0010
#define ACC_SUPER       0x0020
#define ACC_SYNCHRONIZED 0x0020
#define ACC_VOLATILE    0x0040
#define ACC_TRANSIENT   0x0080
#define ACC_NATIVE      0x0100
#define ACC_INTERFACE   0x0200
#define ACC_ABSTRACT    0x0400
#define ACC_STRICT      0x0800

// 常量池条目
typedef struct {
    j2me_constant_type_t tag;
    union {
        struct {
            uint16_t length;
            char* bytes;
        } utf8;
        
        struct {
            uint32_t value;
        } integer;
        
        struct {
            float value;
        } float_val;
        
        struct {
            uint64_t value;
        } long_val;
        
        struct {
            double value;
        } double_val;
        
        struct {
            uint16_t name_index;
        } class_info;
        
        struct {
            uint16_t string_index;
        } string_info;
        
        struct {
            uint16_t class_index;
            uint16_t name_and_type_index;
        } ref_info;
        
        struct {
            uint16_t name_index;
            uint16_t descriptor_index;
        } name_and_type_info;
    } info;
} j2me_constant_pool_entry_t;

// 常量池
struct j2me_constant_pool {
    uint16_t count;
    j2me_constant_pool_entry_t* entries;
};

// 字段信息
struct j2me_field {
    uint16_t access_flags;
    uint16_t name_index;
    uint16_t descriptor_index;
    uint16_t attributes_count;
    
    // 解析后的信息
    const char* name;
    const char* descriptor;
    size_t offset;              // 在对象中的偏移量
    j2me_class_t* owner_class;  // 所属类
};

// 方法信息
struct j2me_method {
    uint16_t access_flags;
    uint16_t name_index;
    uint16_t descriptor_index;
    uint16_t attributes_count;
    
    // 解析后的信息
    const char* name;
    const char* descriptor;
    uint8_t* bytecode;          // 字节码
    uint32_t bytecode_length;   // 字节码长度
    uint16_t max_stack;         // 最大栈深度
    uint16_t max_locals;        // 最大局部变量数
    j2me_class_t* owner_class;  // 所属类
    
    // 运行时信息
    uint32_t invocation_count;  // 调用次数 (用于JIT优化)
    bool is_native;             // 是否为本地方法
    void* native_function;      // 本地方法指针
};

// 类状态
typedef enum {
    CLASS_LOADED = 0,           // 已加载
    CLASS_LINKED,               // 已链接
    CLASS_INITIALIZED           // 已初始化
} j2me_class_state_t;

// 类信息
struct j2me_class {
    // Class文件信息
    uint32_t magic;
    uint16_t minor_version;
    uint16_t major_version;
    uint16_t access_flags;
    uint16_t this_class;
    uint16_t super_class;
    uint16_t interfaces_count;
    uint16_t* interfaces;
    
    // 常量池
    j2me_constant_pool_t constant_pool;
    void* constant_cache;       // 常量池缓存 (j2me_constant_cache_t*)
    
    // 字段和方法
    uint16_t fields_count;
    j2me_field_t* fields;
    uint16_t methods_count;
    j2me_method_t* methods;
    
    // 解析后的信息
    const char* name;           // 类名
    const char* super_name;     // 父类名
    j2me_class_t* super_class_ptr; // 父类指针
    j2me_class_state_t state;   // 类状态
    
    // 运行时信息
    size_t instance_size;       // 实例大小
    j2me_method_t* clinit;      // 类初始化方法
    j2me_class_loader_t* loader; // 类加载器
    
    // 链表节点 (用于类加载器管理)
    j2me_class_t* next;
};

// 类加载器
struct j2me_class_loader {
    j2me_class_t* loaded_classes; // 已加载的类链表
    char* classpath;              // 类路径
    j2me_vm_t* vm;               // 虚拟机实例
    void* jar_file;              // JAR文件对象 (j2me_jar_file_t*)
};

/**
 * @brief 创建类加载器
 * @param vm 虚拟机实例
 * @param classpath 类路径
 * @return 类加载器指针
 */
j2me_class_loader_t* j2me_class_loader_create(j2me_vm_t* vm, const char* classpath);

/**
 * @brief 设置类加载器的JAR文件
 * @param loader 类加载器
 * @param jar_file JAR文件对象
 * @return 错误码
 */
j2me_error_t j2me_class_loader_set_jar_file(j2me_class_loader_t* loader, void* jar_file);

/**
 * @brief 销毁类加载器
 * @param loader 类加载器
 */
void j2me_class_loader_destroy(j2me_class_loader_t* loader);

/**
 * @brief 加载类
 * @param loader 类加载器
 * @param class_name 类名
 * @return 类指针，失败返回NULL
 */
j2me_class_t* j2me_class_loader_load_class(j2me_class_loader_t* loader, const char* class_name);

/**
 * @brief 查找已加载的类
 * @param loader 类加载器
 * @param class_name 类名
 * @return 类指针，未找到返回NULL
 */
j2me_class_t* j2me_class_loader_find_class(j2me_class_loader_t* loader, const char* class_name);

/**
 * @brief 链接类
 * @param class_ptr 类指针
 * @return 错误码
 */
j2me_error_t j2me_class_link(j2me_class_t* class_ptr);

/**
 * @brief 初始化类
 * @param class_ptr 类指针
 * @return 错误码
 */
j2me_error_t j2me_class_initialize(j2me_class_t* class_ptr);

/**
 * @brief 查找方法
 * @param class_ptr 类指针
 * @param name 方法名
 * @param descriptor 方法描述符
 * @return 方法指针，未找到返回NULL
 */
j2me_method_t* j2me_class_find_method(j2me_class_t* class_ptr, const char* name, const char* descriptor);

/**
 * @brief 查找字段
 * @param class_ptr 类指针
 * @param name 字段名
 * @param descriptor 字段描述符
 * @return 字段指针，未找到返回NULL
 */
j2me_field_t* j2me_class_find_field(j2me_class_t* class_ptr, const char* name, const char* descriptor);

/**
 * @brief 从常量池获取UTF-8字符串
 * @param pool 常量池
 * @param index 索引
 * @return 字符串指针，失败返回NULL
 */
const char* j2me_constant_pool_get_utf8(j2me_constant_pool_t* pool, uint16_t index);

/**
 * @brief 从常量池获取类名
 * @param pool 常量池
 * @param index 索引
 * @return 类名字符串，失败返回NULL
 */
const char* j2me_constant_pool_get_class_name(j2me_constant_pool_t* pool, uint16_t index);

/**
 * @brief 解析Class文件
 * @param data Class文件数据
 * @param size 数据大小
 * @return 类指针，失败返回NULL
 */
j2me_class_t* j2me_class_parse(const uint8_t* data, size_t size);

/**
 * @brief 销毁类
 * @param class_ptr 类指针
 */
void j2me_class_destroy(j2me_class_t* class_ptr);

#endif // J2ME_CLASS_H