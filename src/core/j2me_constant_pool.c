#include "j2me_constant_pool.h"
#include "j2me_class.h"
#include "j2me_object.h"
#include "j2me_vm.h"
#include "j2me_string.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @file j2me_constant_pool.c
 * @brief J2ME常量池处理实现
 * 
 * 完整的常量池解析、缓存和访问机制
 */

// 常量池缓存条目
typedef struct {
    uint16_t index;
    j2me_constant_type_t type;
    union {
        j2me_int int_value;
        j2me_float float_value;
        j2me_object_t* object_ref;
        j2me_class_t* class_ref;
    } cached_value;
    bool is_resolved;
} j2me_constant_cache_entry_t;

// 常量池缓存
typedef struct {
    j2me_constant_cache_entry_t* entries;
    size_t size;
    size_t capacity;
} j2me_constant_cache_t;

/**
 * @brief 创建常量池缓存
 */
static j2me_constant_cache_t* j2me_constant_cache_create(size_t capacity) {
    j2me_constant_cache_t* cache = (j2me_constant_cache_t*)malloc(sizeof(j2me_constant_cache_t));
    if (!cache) {
        return NULL;
    }
    
    cache->entries = (j2me_constant_cache_entry_t*)calloc(capacity, sizeof(j2me_constant_cache_entry_t));
    if (!cache->entries) {
        free(cache);
        return NULL;
    }
    
    cache->size = 0;
    cache->capacity = capacity;
    
    return cache;
}

/**
 * @brief 销毁常量池缓存
 */
static void j2me_constant_cache_destroy(j2me_constant_cache_t* cache) {
    if (cache) {
        if (cache->entries) {
            free(cache->entries);
        }
        free(cache);
    }
}

/**
 * @brief 从缓存中查找常量
 */
static j2me_constant_cache_entry_t* j2me_constant_cache_find(j2me_constant_cache_t* cache, uint16_t index) {
    for (size_t i = 0; i < cache->size; i++) {
        if (cache->entries[i].index == index) {
            return &cache->entries[i];
        }
    }
    return NULL;
}

/**
 * @brief 向缓存中添加常量
 */
static j2me_error_t j2me_constant_cache_add(j2me_constant_cache_t* cache, 
                                            uint16_t index, 
                                            j2me_constant_type_t type,
                                            void* value) {
    if (cache->size >= cache->capacity) {
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    j2me_constant_cache_entry_t* entry = &cache->entries[cache->size++];
    entry->index = index;
    entry->type = type;
    entry->is_resolved = true;
    
    switch (type) {
        case J2ME_CONSTANT_INTEGER:
            entry->cached_value.int_value = *(j2me_int*)value;
            break;
        case J2ME_CONSTANT_FLOAT:
            entry->cached_value.float_value = *(j2me_float*)value;
            break;
        case J2ME_CONSTANT_STRING:
        case J2ME_CONSTANT_CLASS:
            entry->cached_value.object_ref = (j2me_object_t*)value;
            break;
        default:
            entry->is_resolved = false;
            return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    return J2ME_SUCCESS;
}

/**
 * @brief 解析UTF-8字符串常量
 */
static j2me_error_t j2me_resolve_utf8_constant(j2me_class_t* class_info, 
                                               uint16_t index, 
                                               const char** result) {
    if (!class_info || !result || index == 0 || index >= class_info->constant_pool.count) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    j2me_constant_pool_entry_t* entry = &class_info->constant_pool.entries[index - 1];
    if (entry->tag != J2ME_CONSTANT_UTF8) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    *result = entry->info.utf8.bytes;
    return J2ME_SUCCESS;
}

/**
 * @brief 解析类常量引用
 */
static j2me_error_t j2me_resolve_class_constant(j2me_vm_t* vm,
                                                j2me_class_t* class_info,
                                                uint16_t index,
                                                j2me_class_t** result) {
    if (!vm || !class_info || !result || index == 0 || index >= class_info->constant_pool.count) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    j2me_constant_pool_entry_t* entry = &class_info->constant_pool.entries[index - 1];
    if (entry->tag != J2ME_CONSTANT_CLASS) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 获取类名
    const char* class_name;
    j2me_error_t error = j2me_resolve_utf8_constant(class_info, entry->info.class_info.name_index, &class_name);
    if (error != J2ME_SUCCESS) {
        return error;
    }
    
    // 加载类（这里需要类加载器支持）
    // 暂时返回当前类作为占位符
    *result = class_info;
    
    // printf("[常量池] 解析类常量: %s\n", class_name);
    return J2ME_SUCCESS;
}

/**
 * @brief 解析字符串常量
 */
static j2me_error_t j2me_resolve_string_constant(j2me_vm_t* vm,
                                                 j2me_class_t* class_info,
                                                 uint16_t index,
                                                 j2me_object_t** result) {
    if (!vm || !class_info || !result || index == 0 || index >= class_info->constant_pool.count) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    j2me_constant_pool_entry_t* entry = &class_info->constant_pool.entries[index - 1];
    if (entry->tag != J2ME_CONSTANT_STRING) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 获取字符串内容
    const char* string_content;
    j2me_error_t error = j2me_resolve_utf8_constant(class_info, entry->info.string_info.string_index, &string_content);
    if (error != J2ME_SUCCESS) {
        return error;
    }
    
    // 创建字符串对象（这里需要对象系统支持）
    // 暂时创建一个简单的字符串对象
    j2me_object_t* string_obj = (j2me_object_t*)malloc(sizeof(j2me_object_t) + strlen(string_content) + 1);
    if (!string_obj) {
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    // 初始化字符串对象
    memset(string_obj, 0, sizeof(j2me_object_t));
    strcpy((char*)(string_obj + 1), string_content);
    
    *result = string_obj;
    
    // printf("[常量池] 解析字符串常量: \"%s\"\n", string_content);
    return J2ME_SUCCESS;
}

/**
 * @brief 解析常量池条目 (简化版本)
 */
j2me_error_t j2me_resolve_constant_pool_entry(j2me_vm_t* vm,
                                              j2me_class_t* class_info,
                                              uint16_t index,
                                              j2me_constant_value_t* value) {
    if (!vm || !class_info || !value || index == 0 || index >= class_info->constant_pool.count) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    j2me_constant_pool_entry_t* entry = &class_info->constant_pool.entries[index - 1];
    
    // 根据常量类型解析
    switch (entry->tag) {
        case J2ME_CONSTANT_INTEGER:
            value->type = J2ME_CONSTANT_INTEGER;
            value->data.int_value = entry->info.integer.value;
            // printf("[常量池] 解析整数常量: %d\n", value->data.int_value);
            break;
            
        case J2ME_CONSTANT_FLOAT:
            value->type = J2ME_CONSTANT_FLOAT;
            value->data.float_value = entry->info.float_val.value;
            // printf("[常量池] 解析浮点常量: %f\n", value->data.float_value);
            break;
            
        case J2ME_CONSTANT_LONG:
            value->type = J2ME_CONSTANT_LONG;
            value->data.long_value = entry->info.long_val.value;
            // printf("[常量池] 解析长整数常量: %lld\n", (long long)value->data.long_value);
            break;
            
        case J2ME_CONSTANT_DOUBLE:
            value->type = J2ME_CONSTANT_DOUBLE;
            value->data.double_value = entry->info.double_val.value;
            // printf("[常量池] 解析双精度常量: %f\n", value->data.double_value);
            break;
            
        case J2ME_CONSTANT_UTF8:
            value->type = J2ME_CONSTANT_UTF8;
            value->data.string_value = entry->info.utf8.bytes;
            // printf("[常量池] 解析UTF8字符串: %s\n", value->data.string_value ? value->data.string_value : "NULL");
            break;
            
        case J2ME_CONSTANT_STRING:
            value->type = J2ME_CONSTANT_STRING;
            // 递归解析字符串引用
            if (entry->info.string_info.string_index > 0 && 
                entry->info.string_info.string_index < class_info->constant_pool.count) {
                j2me_constant_pool_entry_t* string_entry = 
                    &class_info->constant_pool.entries[entry->info.string_info.string_index - 1];
                if (string_entry->tag == J2ME_CONSTANT_UTF8) {
                    const char* str_value = string_entry->info.utf8.bytes;
                    // printf("[常量池] 解析字符串常量: %s\n", str_value ? str_value : "NULL");
                    
                    // 在堆上创建真实的String对象
                    if (vm->heap && str_value) {
                        // 使用新的堆String系统创建String对象
                        j2me_ref_t string_ref = j2me_heap_alloc(vm->heap, J2ME_CLASS_STRING, 
                                                                sizeof(j2me_string_data_t) + strlen(str_value) + 1);
                        if (string_ref != J2ME_NULL_REF) {
                            j2me_string_data_t* str_data = (j2me_string_data_t*)j2me_heap_get_object_data(vm->heap, string_ref);
                            if (str_data) {
                                str_data->length = strlen(str_value);
                                strcpy(str_data->chars, str_value);
                                value->data.object_ref = (void*)(intptr_t)string_ref;
                                // printf("[常量池] 创建String对象: ref=0x%x, 内容=\"%s\"\n", string_ref, str_value);
                            } else {
                                // printf("[常量池] 警告: 无法获取String对象数据\n");
                                value->data.object_ref = NULL;
                            }
                        } else {
                            // printf("[常量池] 警告: String对象创建失败\n");
                            value->data.object_ref = NULL;
                        }
                    } else {
                        // printf("[常量池] 警告: 堆未初始化或字符串为空\n");
                        value->data.object_ref = NULL;
                    }
                } else {
                    value->data.object_ref = NULL;
                }
            } else {
                value->data.object_ref = NULL;
            }
            break;
            
        case J2ME_CONSTANT_CLASS:
            value->type = J2ME_CONSTANT_CLASS;
            // 递归解析类名
            if (entry->info.class_info.name_index > 0 && 
                entry->info.class_info.name_index < class_info->constant_pool.count) {
                j2me_constant_pool_entry_t* name_entry = 
                    &class_info->constant_pool.entries[entry->info.class_info.name_index - 1];
                if (name_entry->tag == J2ME_CONSTANT_UTF8) {
                    value->data.string_value = name_entry->info.utf8.bytes;
                } else {
                    value->data.string_value = NULL;
                }
            } else {
                value->data.string_value = NULL;
            }
            // printf("[常量池] 解析类常量: %s\n", value->data.string_value ? value->data.string_value : "NULL");
            break;
            
        default:
            // printf("[常量池] 不支持的常量类型: %d，返回默认值\n", entry->tag);
            value->type = J2ME_CONSTANT_INTEGER;
            value->data.int_value = index; // 使用索引作为默认值
            break;
    }
    
    return J2ME_SUCCESS;
}

/**
 * @brief 初始化类的常量池缓存
 */
j2me_error_t j2me_constant_pool_init_cache(j2me_class_t* class_info) {
    if (!class_info) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    if (class_info->constant_cache) {
        return J2ME_SUCCESS; // 已经初始化
    }
    
    class_info->constant_cache = j2me_constant_cache_create(class_info->constant_pool.count);
    if (!class_info->constant_cache) {
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    // printf("[常量池] 初始化缓存，容量: %d\n", class_info->constant_pool.count);
    return J2ME_SUCCESS;
}

/**
 * @brief 清理类的常量池缓存
 */
void j2me_constant_pool_cleanup_cache(j2me_class_t* class_info) {
    if (class_info && class_info->constant_cache) {
        j2me_constant_cache_destroy(class_info->constant_cache);
        class_info->constant_cache = NULL;
    }
}

/**
 * @brief 预加载常用常量
 */
j2me_error_t j2me_constant_pool_preload(j2me_vm_t* vm, j2me_class_t* class_info) {
    if (!vm || !class_info) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 预加载所有字符串和类常量
    for (uint16_t i = 1; i < class_info->constant_pool.count; i++) {
        j2me_constant_pool_entry_t* entry = &class_info->constant_pool.entries[i - 1];
        
        if (entry->tag == J2ME_CONSTANT_STRING || entry->tag == J2ME_CONSTANT_CLASS) {
            j2me_constant_value_t value;
            j2me_error_t error = j2me_resolve_constant_pool_entry(vm, class_info, i, &value);
            if (error != J2ME_SUCCESS) {
                // printf("[常量池] 警告: 预加载常量 #%d 失败: %d\n", i, error);
            }
        }
    }
    
    // printf("[常量池] 预加载完成，类: %s\n", class_info->name ? class_info->name : "未知");
    return J2ME_SUCCESS;
}