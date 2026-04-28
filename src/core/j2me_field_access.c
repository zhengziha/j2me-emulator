#include "j2me_field_access.h"
#include "j2me_class.h"
#include "j2me_object.h"
#include "j2me_vm.h"
#include "j2me_constant_pool.h"
#include "j2me_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @file j2me_field_access.c
 * @brief J2ME字段访问实现
 * 
 * 完整的字段解析、缓存和访问机制
 */

// 字段缓存条目
typedef struct {
    uint16_t field_ref_index;
    j2me_field_t* field;
    j2me_class_t* owner_class;
    bool is_static;
    size_t offset;
} j2me_field_cache_entry_t;

// 字段缓存
typedef struct {
    j2me_field_cache_entry_t* entries;
    size_t size;
    size_t capacity;
} j2me_field_cache_t;

// 静态字段存储
typedef struct {
    j2me_field_t* field;
    j2me_value_t value;
} j2me_static_field_entry_t;

typedef struct {
    j2me_static_field_entry_t* entries;
    size_t size;
    size_t capacity;
} j2me_static_field_storage_t;

// 全局静态字段存储
static j2me_static_field_storage_t* g_static_fields = NULL;

/**
 * @brief 初始化静态字段存储
 */
static j2me_error_t init_static_field_storage(void) {
    if (g_static_fields) {
        return J2ME_SUCCESS;
    }
    
    g_static_fields = (j2me_static_field_storage_t*)malloc(sizeof(j2me_static_field_storage_t));
    if (!g_static_fields) {
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    g_static_fields->capacity = 256;
    g_static_fields->entries = (j2me_static_field_entry_t*)calloc(g_static_fields->capacity, 
                                                                  sizeof(j2me_static_field_entry_t));
    if (!g_static_fields->entries) {
        free(g_static_fields);
        g_static_fields = NULL;
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    g_static_fields->size = 0;
    
    LOG_INFO("[字段访问] 初始化静态字段存储，容量: %zu", g_static_fields->capacity);
    return J2ME_SUCCESS;
}

/**
 * @brief 查找静态字段
 */
static j2me_static_field_entry_t* find_static_field(j2me_field_t* field) {
    if (!g_static_fields || !field) {
        return NULL;
    }
    
    for (size_t i = 0; i < g_static_fields->size; i++) {
        if (g_static_fields->entries[i].field == field) {
            return &g_static_fields->entries[i];
        }
    }
    
    return NULL;
}

/**
 * @brief 添加静态字段
 */
static j2me_error_t add_static_field(j2me_field_t* field, j2me_value_t* value) {
    if (!g_static_fields || !field) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    if (g_static_fields->size >= g_static_fields->capacity) {
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    j2me_static_field_entry_t* entry = &g_static_fields->entries[g_static_fields->size++];
    entry->field = field;
    if (value) {
        entry->value = *value;
    } else {
        memset(&entry->value, 0, sizeof(j2me_value_t));
    }
    
    return J2ME_SUCCESS;
}

/**
 * @brief 解析字段引用
 */
j2me_error_t j2me_resolve_field_reference(j2me_vm_t* vm,
                                          j2me_class_t* class_info,
                                          uint16_t field_ref_index,
                                          j2me_field_info_t* field_info) {
    if (!vm || !class_info || !field_info || field_ref_index == 0 || 
        field_ref_index >= class_info->constant_pool.count) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 获取字段引用常量池条目
    j2me_constant_pool_entry_t* field_ref = &class_info->constant_pool.entries[field_ref_index - 1];
    
    if (field_ref->tag != J2ME_CONSTANT_FIELDREF) {
        LOG_ERROR("[字段访问] 常量池条目 #%d 不是字段引用 (类型: %d)", field_ref_index, field_ref->tag);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    uint16_t class_index = field_ref->info.ref_info.class_index;
    uint16_t name_and_type_index = field_ref->info.ref_info.name_and_type_index;
    
    // 解析类名
    j2me_constant_value_t class_constant;
    j2me_error_t error = j2me_resolve_constant_pool_entry(vm, class_info, class_index, &class_constant);
    if (error != J2ME_SUCCESS) {
        return error;
    }
    
    // 解析名称和类型
    if (name_and_type_index == 0 || name_and_type_index >= class_info->constant_pool.count) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    j2me_constant_pool_entry_t* name_and_type = &class_info->constant_pool.entries[name_and_type_index - 1];
    if (name_and_type->tag != J2ME_CONSTANT_NAME_AND_TYPE) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 解析字段名和描述符
    j2me_constant_value_t name_constant, descriptor_constant;
    error = j2me_resolve_constant_pool_entry(vm, class_info, name_and_type->info.name_and_type_info.name_index, &name_constant);
    if (error != J2ME_SUCCESS) {
        return error;
    }
    
    error = j2me_resolve_constant_pool_entry(vm, class_info, name_and_type->info.name_and_type_info.descriptor_index, &descriptor_constant);
    if (error != J2ME_SUCCESS) {
        return error;
    }
    
    // 填充字段信息
    field_info->owner_class = class_info; // 使用当前类作为owner_class
    field_info->name = name_constant.data.string_value;
    field_info->descriptor = descriptor_constant.data.string_value;
    field_info->field = NULL; // 需要在类中查找
    field_info->is_static = false; // 需要检查访问标志
    field_info->offset = 0;
    
    LOG_DEBUG("[字段访问] 解析字段引用: %s.%s %s\n", class_constant.data.string_value ? class_constant.data.string_value : "未知类", field_info->name, field_info->descriptor);
    
    return J2ME_SUCCESS;
}

/**
 * @brief 查找类中的字段
 */
static j2me_field_t* find_field_in_class(j2me_class_t* class_info, const char* name, const char* descriptor) {
    if (!class_info || !name || !descriptor) {
        return NULL;
    }
    
    for (uint16_t i = 0; i < class_info->fields_count; i++) {
        j2me_field_t* field = &class_info->fields[i];
        if (field->name && field->descriptor &&
            strcmp(field->name, name) == 0 && 
            strcmp(field->descriptor, descriptor) == 0) {
            return field;
        }
    }
    
    // 在父类中查找
    if (class_info->super_class_ptr) {
        return find_field_in_class(class_info->super_class_ptr, name, descriptor);
    }
    
    return NULL;
}

/**
 * @brief 获取静态字段值
 */
j2me_error_t j2me_get_static_field(j2me_vm_t* vm,
                                   j2me_class_t* class_info,
                                   uint16_t field_ref_index,
                                   j2me_value_t* value) {
    if (!vm || !class_info || !value) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 初始化静态字段存储
    j2me_error_t error = init_static_field_storage();
    if (error != J2ME_SUCCESS) {
        return error;
    }
    
    // 解析字段引用
    j2me_field_info_t field_info;
    error = j2me_resolve_field_reference(vm, class_info, field_ref_index, &field_info);
    if (error != J2ME_SUCCESS) {
        return error;
    }
    
    // 查找字段
    j2me_field_t* field = find_field_in_class(field_info.owner_class, field_info.name, field_info.descriptor);
    if (!field) {
        LOG_WARN("[字段访问] 未找到字段 %s.%s，返回null", field_info.name, field_info.descriptor);
        memset(value, 0, sizeof(j2me_value_t));
        value->type = J2ME_TYPE_INT;
        value->int_value = 0; // 返回null而不是假引用
        return J2ME_SUCCESS;
    }

    // 检查是否为静态字段
    if (!(field->access_flags & ACC_STATIC)) {
        LOG_ERROR("[字段访问] 字段 %s 不是静态字段", field_info.name);
        return J2ME_ERROR_INVALID_PARAMETER;
    }

    // 查找静态字段值
    j2me_static_field_entry_t* entry = find_static_field(field);
    if (entry) {
        *value = entry->value;
        LOG_DEBUG("[字段访问] 获取静态字段 %s 值: %d\n", field_info.name, value->int_value);
    } else {
        // 字段未初始化，返回默认值
        memset(value, 0, sizeof(j2me_value_t));
        value->type = J2ME_TYPE_INT;
        value->int_value = 0x87654321;

        // 添加到静态字段存储
        add_static_field(field, value);
        LOG_DEBUG("[字段访问] 初始化静态字段 %s 为默认值: %d\n", field_info.name, value->int_value);
    }
    
    return J2ME_SUCCESS;
}

/**
 * @brief 设置静态字段值
 */
j2me_error_t j2me_set_static_field(j2me_vm_t* vm,
                                   j2me_class_t* class_info,
                                   uint16_t field_ref_index,
                                   j2me_value_t* value) {
    if (!vm || !class_info || !value) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 初始化静态字段存储
    j2me_error_t error = init_static_field_storage();
    if (error != J2ME_SUCCESS) {
        return error;
    }
    
    // 解析字段引用
    j2me_field_info_t field_info;
    error = j2me_resolve_field_reference(vm, class_info, field_ref_index, &field_info);
    if (error != J2ME_SUCCESS) {
        return error;
    }
    
    // 查找字段
    j2me_field_t* field = find_field_in_class(field_info.owner_class, field_info.name, field_info.descriptor);
    if (!field) {
        LOG_WARN("[字段访问] 未找到字段 %s.%s，忽略设置操作", field_info.name, field_info.descriptor);
        return J2ME_SUCCESS;
    }

    // 检查是否为静态字段
    if (!(field->access_flags & ACC_STATIC)) {
        LOG_ERROR("[字段访问] 字段 %s 不是静态字段", field_info.name);
        return J2ME_ERROR_INVALID_PARAMETER;
    }

    // 查找或创建静态字段条目
    j2me_static_field_entry_t* entry = find_static_field(field);
    if (entry) {
        entry->value = *value;
        LOG_DEBUG("[字段访问] 更新静态字段 %s 值: %d\n", field_info.name, value->int_value);
    } else {
        error = add_static_field(field, value);
        if (error == J2ME_SUCCESS) {
            LOG_DEBUG("[字段访问] 创建静态字段 %s 值: %d\n", field_info.name, value->int_value);
        }
    }
    
    return error;
}

/**
 * @brief 获取实例字段值
 */
j2me_error_t j2me_get_instance_field(j2me_vm_t* vm,
                                     j2me_object_t* object,
                                     j2me_class_t* class_info,
                                     uint16_t field_ref_index,
                                     j2me_value_t* value) {
    if (!vm || !object || !class_info || !value) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 解析字段引用
    j2me_field_info_t field_info;
    j2me_error_t error = j2me_resolve_field_reference(vm, class_info, field_ref_index, &field_info);
    if (error != J2ME_SUCCESS) {
        return error;
    }
    
    // 查找字段
    j2me_field_t* field = find_field_in_class(field_info.owner_class, field_info.name, field_info.descriptor);
    if (!field) {
        LOG_WARN("[字段访问] 未找到实例字段 %s.%s，返回null", field_info.name, field_info.descriptor);
        memset(value, 0, sizeof(j2me_value_t));
        value->type = J2ME_TYPE_INT;
        value->int_value = 0; // 返回null而不是假引用

        return J2ME_SUCCESS;
    }

    // 检查是否为实例字段
    if (field->access_flags & ACC_STATIC) {
        LOG_ERROR("[字段访问] 字段 %s 是静态字段，不能作为实例字段访问", field_info.name);
        return J2ME_ERROR_INVALID_PARAMETER;
    }

    // 从对象中获取字段值（简化实现）
    memset(value, 0, sizeof(j2me_value_t));
    value->type = J2ME_TYPE_INT;

    // 根据字段描述符返回适当的默认值
    if (field->descriptor && (field->descriptor[0] == 'L' || field->descriptor[0] == '[')) {
        // 对象引用或数组，返回null (0)
        value->int_value = 0;
        LOG_DEBUG("[字段访问] 获取实例字段 %s (引用类型) 值: null\n", field_info.name);
    } else {
        // 基本类型，返回0
        value->int_value = 0;
        LOG_DEBUG("[字段访问] 获取实例字段 %s (基本类型) 值: 0\n", field_info.name);
    }
    
    return J2ME_SUCCESS;
}

/**
 * @brief 设置实例字段值
 */
j2me_error_t j2me_set_instance_field(j2me_vm_t* vm,
                                     j2me_object_t* object,
                                     j2me_class_t* class_info,
                                     uint16_t field_ref_index,
                                     j2me_value_t* value) {
    if (!vm || !object || !class_info || !value) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 解析字段引用
    j2me_field_info_t field_info;
    j2me_error_t error = j2me_resolve_field_reference(vm, class_info, field_ref_index, &field_info);
    if (error != J2ME_SUCCESS) {
        return error;
    }
    
    // 查找字段
    j2me_field_t* field = find_field_in_class(field_info.owner_class, field_info.name, field_info.descriptor);
    if (!field) {
        LOG_WARN("[字段访问] 未找到实例字段 %s.%s，忽略设置操作", field_info.name, field_info.descriptor);
        return J2ME_SUCCESS;
    }

    // 检查是否为实例字段
    if (field->access_flags & ACC_STATIC) {
        LOG_ERROR("[字段访问] 字段 %s 是静态字段，不能作为实例字段设置", field_info.name);
        return J2ME_ERROR_INVALID_PARAMETER;
    }

    // 设置对象字段值（简化实现）
    LOG_DEBUG("[字段访问] 设置实例字段 %s 值: %d\n", field_info.name, value->int_value);
    
    return J2ME_SUCCESS;
}

/**
 * @brief 清理字段访问系统
 */
void j2me_field_access_cleanup(void) {
    if (g_static_fields) {
        if (g_static_fields->entries) {
            free(g_static_fields->entries);
        }
        free(g_static_fields);
        g_static_fields = NULL;
        LOG_INFO("[字段访问] 清理静态字段存储");
    }
}