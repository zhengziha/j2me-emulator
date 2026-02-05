#ifndef J2ME_FIELD_ACCESS_H
#define J2ME_FIELD_ACCESS_H

#include "j2me_types.h"
#include "j2me_class.h"

/**
 * @file j2me_field_access.h
 * @brief J2ME字段访问接口
 * 
 * 提供完整的字段解析、缓存和访问功能
 */

// 前向声明
typedef struct j2me_vm j2me_vm_t;
typedef struct j2me_class j2me_class_t;
typedef struct j2me_field j2me_field_t;
typedef struct j2me_object j2me_object_t;

// 字段信息
typedef struct {
    j2me_class_t* owner_class;
    const char* name;
    const char* descriptor;
    j2me_field_t* field;
    bool is_static;
    size_t offset;
} j2me_field_info_t;

// 值类型
typedef enum {
    J2ME_TYPE_INT,
    J2ME_TYPE_FLOAT,
    J2ME_TYPE_LONG,
    J2ME_TYPE_DOUBLE,
    J2ME_TYPE_REFERENCE
} j2me_value_type_t;

// 值联合体
typedef struct {
    j2me_value_type_t type;
    union {
        j2me_int int_value;
        j2me_float float_value;
        j2me_long long_value;
        j2me_double double_value;
        j2me_object_t* object_ref;
    };
} j2me_value_t;

/**
 * @brief 解析字段引用
 * @param vm 虚拟机实例
 * @param class_info 类信息
 * @param field_ref_index 字段引用索引
 * @param field_info 输出的字段信息
 * @return 错误码
 */
j2me_error_t j2me_resolve_field_reference(j2me_vm_t* vm,
                                          j2me_class_t* class_info,
                                          uint16_t field_ref_index,
                                          j2me_field_info_t* field_info);

/**
 * @brief 获取静态字段值
 * @param vm 虚拟机实例
 * @param class_info 类信息
 * @param field_ref_index 字段引用索引
 * @param value 输出的字段值
 * @return 错误码
 */
j2me_error_t j2me_get_static_field(j2me_vm_t* vm,
                                   j2me_class_t* class_info,
                                   uint16_t field_ref_index,
                                   j2me_value_t* value);

/**
 * @brief 设置静态字段值
 * @param vm 虚拟机实例
 * @param class_info 类信息
 * @param field_ref_index 字段引用索引
 * @param value 字段值
 * @return 错误码
 */
j2me_error_t j2me_set_static_field(j2me_vm_t* vm,
                                   j2me_class_t* class_info,
                                   uint16_t field_ref_index,
                                   j2me_value_t* value);

/**
 * @brief 获取实例字段值
 * @param vm 虚拟机实例
 * @param object 对象实例
 * @param class_info 类信息
 * @param field_ref_index 字段引用索引
 * @param value 输出的字段值
 * @return 错误码
 */
j2me_error_t j2me_get_instance_field(j2me_vm_t* vm,
                                     j2me_object_t* object,
                                     j2me_class_t* class_info,
                                     uint16_t field_ref_index,
                                     j2me_value_t* value);

/**
 * @brief 设置实例字段值
 * @param vm 虚拟机实例
 * @param object 对象实例
 * @param class_info 类信息
 * @param field_ref_index 字段引用索引
 * @param value 字段值
 * @return 错误码
 */
j2me_error_t j2me_set_instance_field(j2me_vm_t* vm,
                                     j2me_object_t* object,
                                     j2me_class_t* class_info,
                                     uint16_t field_ref_index,
                                     j2me_value_t* value);

/**
 * @brief 清理字段访问系统
 */
void j2me_field_access_cleanup(void);

#endif // J2ME_FIELD_ACCESS_H