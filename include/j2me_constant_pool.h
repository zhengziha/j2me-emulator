#ifndef J2ME_CONSTANT_POOL_H
#define J2ME_CONSTANT_POOL_H

#include "j2me_types.h"
#include "j2me_class.h"
#include "j2me_object.h"

/**
 * @file j2me_constant_pool.h
 * @brief J2ME常量池处理接口
 * 
 * 提供完整的常量池解析、缓存和访问功能
 */

// 常量值联合体
typedef struct {
    j2me_constant_type_t type;
    union {
        j2me_int int_value;
        j2me_float float_value;
        j2me_long long_value;
        j2me_double double_value;
        j2me_object_t* object_ref;
        j2me_class_t* class_ref;
        const char* string_value;
    };
} j2me_constant_value_t;

/**
 * @brief 解析常量池条目
 * @param vm 虚拟机实例
 * @param class_info 类信息
 * @param index 常量池索引 (1-based)
 * @param value 输出的常量值
 * @return 错误码
 */
j2me_error_t j2me_resolve_constant_pool_entry(j2me_vm_t* vm,
                                              j2me_class_t* class_info,
                                              uint16_t index,
                                              j2me_constant_value_t* value);

/**
 * @brief 初始化类的常量池缓存
 * @param class_info 类信息
 * @return 错误码
 */
j2me_error_t j2me_constant_pool_init_cache(j2me_class_t* class_info);

/**
 * @brief 清理类的常量池缓存
 * @param class_info 类信息
 */
void j2me_constant_pool_cleanup_cache(j2me_class_t* class_info);

/**
 * @brief 预加载常用常量
 * @param vm 虚拟机实例
 * @param class_info 类信息
 * @return 错误码
 */
j2me_error_t j2me_constant_pool_preload(j2me_vm_t* vm, j2me_class_t* class_info);

#endif // J2ME_CONSTANT_POOL_H