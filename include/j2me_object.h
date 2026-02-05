#ifndef J2ME_OBJECT_H
#define J2ME_OBJECT_H

#include "j2me_types.h"
#include "j2me_class.h"
#include <stddef.h>

/**
 * @file j2me_object.h
 * @brief J2ME对象系统
 * 
 * 定义Java对象的内存表示和对象管理接口
 */

// 前向声明
typedef struct j2me_object j2me_object_t;
typedef struct j2me_array j2me_array_t;
typedef struct j2me_string j2me_string_t;
typedef struct j2me_gc j2me_gc_t;

// 对象头标志
#define OBJECT_FLAG_MARKED      0x01    // GC标记
#define OBJECT_FLAG_FINALIZED   0x02    // 已终结
#define OBJECT_FLAG_ARRAY       0x04    // 数组对象
#define OBJECT_FLAG_STRING      0x08    // 字符串对象

// 对象头结构
typedef struct {
    j2me_class_t* class_ptr;    // 对象的类
    uint32_t hash_code;         // 哈希码
    uint16_t flags;             // 对象标志
    uint16_t lock_count;        // 锁计数 (用于同步)
} j2me_object_header_t;

// 基础对象结构
struct j2me_object {
    j2me_object_header_t header;    // 对象头
    // 实例字段数据紧跟在对象头后面
};

// 数组对象结构
struct j2me_array {
    j2me_object_header_t header;    // 对象头
    uint32_t length;                // 数组长度
    uint8_t element_size;           // 元素大小
    uint8_t element_type;           // 元素类型
    uint16_t padding;               // 对齐填充
    // 数组数据紧跟在这里
};

// 字符串对象结构
struct j2me_string {
    j2me_object_header_t header;    // 对象头
    uint32_t length;                // 字符串长度
    uint32_t hash;                  // 缓存的哈希值
    j2me_char* chars;               // 字符数据指针
};

// 数组元素类型
typedef enum {
    ARRAY_TYPE_BOOLEAN = 4,
    ARRAY_TYPE_CHAR = 5,
    ARRAY_TYPE_FLOAT = 6,
    ARRAY_TYPE_DOUBLE = 7,
    ARRAY_TYPE_BYTE = 8,
    ARRAY_TYPE_SHORT = 9,
    ARRAY_TYPE_INT = 10,
    ARRAY_TYPE_LONG = 11,
    ARRAY_TYPE_REFERENCE = 12
} j2me_array_type_t;

/**
 * @brief 创建对象实例
 * @param vm 虚拟机实例
 * @param class_ptr 对象的类
 * @return 对象指针，失败返回NULL
 */
j2me_object_t* j2me_object_create(j2me_vm_t* vm, j2me_class_t* class_ptr);

/**
 * @brief 销毁对象
 * @param vm 虚拟机实例
 * @param obj 对象指针
 */
void j2me_object_destroy(j2me_vm_t* vm, j2me_object_t* obj);

/**
 * @brief 获取对象的类
 * @param obj 对象指针
 * @return 类指针
 */
j2me_class_t* j2me_object_get_class(j2me_object_t* obj);

/**
 * @brief 获取实例字段值
 * @param obj 对象指针
 * @param field 字段指针
 * @return 字段值
 */
j2me_int j2me_object_get_field_int(j2me_object_t* obj, j2me_field_t* field);

/**
 * @brief 设置实例字段值
 * @param obj 对象指针
 * @param field 字段指针
 * @param value 字段值
 */
void j2me_object_set_field_int(j2me_object_t* obj, j2me_field_t* field, j2me_int value);

/**
 * @brief 获取实例字段引用
 * @param obj 对象指针
 * @param field 字段指针
 * @return 引用值
 */
j2me_reference j2me_object_get_field_ref(j2me_object_t* obj, j2me_field_t* field);

/**
 * @brief 设置实例字段引用
 * @param obj 对象指针
 * @param field 字段指针
 * @param value 引用值
 */
void j2me_object_set_field_ref(j2me_object_t* obj, j2me_field_t* field, j2me_reference value);

/**
 * @brief 创建数组对象
 * @param vm 虚拟机实例
 * @param element_type 元素类型
 * @param length 数组长度
 * @return 数组对象指针，失败返回NULL
 */
j2me_array_t* j2me_array_create(j2me_vm_t* vm, j2me_array_type_t element_type, uint32_t length);

/**
 * @brief 创建引用类型数组
 * @param vm 虚拟机实例
 * @param element_class 元素类型
 * @param length 数组长度
 * @return 数组对象指针，失败返回NULL
 */
j2me_array_t* j2me_array_create_ref(j2me_vm_t* vm, j2me_class_t* element_class, uint32_t length);

/**
 * @brief 获取数组长度
 * @param array 数组对象
 * @return 数组长度
 */
uint32_t j2me_array_get_length(j2me_array_t* array);

/**
 * @brief 获取数组元素 (int)
 * @param array 数组对象
 * @param index 索引
 * @return 元素值
 */
j2me_int j2me_array_get_int(j2me_array_t* array, uint32_t index);

/**
 * @brief 设置数组元素 (int)
 * @param array 数组对象
 * @param index 索引
 * @param value 元素值
 */
void j2me_array_set_int(j2me_array_t* array, uint32_t index, j2me_int value);

/**
 * @brief 获取数组元素 (引用)
 * @param array 数组对象
 * @param index 索引
 * @return 引用值
 */
j2me_reference j2me_array_get_ref(j2me_array_t* array, uint32_t index);

/**
 * @brief 设置数组元素 (引用)
 * @param array 数组对象
 * @param index 索引
 * @param value 引用值
 */
void j2me_array_set_ref(j2me_array_t* array, uint32_t index, j2me_reference value);

/**
 * @brief 创建字符串对象
 * @param vm 虚拟机实例
 * @param chars 字符数据
 * @param length 字符串长度
 * @return 字符串对象指针，失败返回NULL
 */
j2me_string_t* j2me_string_create(j2me_vm_t* vm, const j2me_char* chars, uint32_t length);

/**
 * @brief 从C字符串创建字符串对象
 * @param vm 虚拟机实例
 * @param cstr C字符串
 * @return 字符串对象指针，失败返回NULL
 */
j2me_string_t* j2me_string_create_from_cstr(j2me_vm_t* vm, const char* cstr);

/**
 * @brief 获取字符串长度
 * @param str 字符串对象
 * @return 字符串长度
 */
uint32_t j2me_string_get_length(j2me_string_t* str);

/**
 * @brief 获取字符串字符数据
 * @param str 字符串对象
 * @return 字符数据指针
 */
const j2me_char* j2me_string_get_chars(j2me_string_t* str);

/**
 * @brief 字符串比较
 * @param str1 字符串1
 * @param str2 字符串2
 * @return 比较结果 (0表示相等)
 */
int j2me_string_compare(j2me_string_t* str1, j2me_string_t* str2);

/**
 * @brief 计算对象大小
 * @param class_ptr 对象的类
 * @return 对象大小 (字节)
 */
size_t j2me_object_calculate_size(j2me_class_t* class_ptr);

/**
 * @brief 计算数组大小
 * @param element_type 元素类型
 * @param length 数组长度
 * @return 数组大小 (字节)
 */
size_t j2me_array_calculate_size(j2me_array_type_t element_type, uint32_t length);

/**
 * @brief 检查对象类型
 * @param obj 对象指针
 * @param class_ptr 类指针
 * @return 是否为指定类型的实例
 */
bool j2me_object_instanceof(j2me_object_t* obj, j2me_class_t* class_ptr);

/**
 * @brief 对象类型转换检查
 * @param obj 对象指针
 * @param target_class 目标类
 * @return 转换是否合法
 */
bool j2me_object_checkcast(j2me_object_t* obj, j2me_class_t* target_class);

#endif // J2ME_OBJECT_H