#include "j2me_class.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @file j2me_class_parser.c
 * @brief J2ME Class文件解析器实现
 * 
 * 解析Java Class文件格式，构建内存中的类表示
 */

// 读取辅助宏
#define READ_U1(data, offset) (data[offset])
#define READ_U2(data, offset) ((data[offset] << 8) | data[offset + 1])
#define READ_U4(data, offset) ((data[offset] << 24) | (data[offset + 1] << 16) | \
                               (data[offset + 2] << 8) | data[offset + 3])

/**
 * @brief 解析常量池
 * @param data Class文件数据
 * @param offset 当前偏移量指针
 * @param pool 常量池结构
 * @return 错误码
 */
static j2me_error_t parse_constant_pool(const uint8_t* data, size_t* offset, j2me_constant_pool_t* pool) {
    pool->count = READ_U2(data, *offset);
    *offset += 2;
    
    if (pool->count == 0) {
        pool->entries = NULL;
        return J2ME_SUCCESS;
    }
    
    // 分配常量池条目数组 (索引从1开始，所以需要count-1个条目)
    pool->entries = (j2me_constant_pool_entry_t*)calloc(pool->count - 1, sizeof(j2me_constant_pool_entry_t));
    if (!pool->entries) {
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    // printf("[类解析器] 解析常量池，条目数: %d\n", pool->count - 1);
    
    for (uint16_t i = 1; i < pool->count; i++) {
        j2me_constant_pool_entry_t* entry = &pool->entries[i - 1];
        entry->tag = (j2me_constant_type_t)READ_U1(data, *offset);
        (*offset)++;
        
        switch (entry->tag) {
            case J2ME_CONSTANT_UTF8: {
                entry->info.utf8.length = READ_U2(data, *offset);
                *offset += 2;
                
                entry->info.utf8.bytes = (char*)malloc(entry->info.utf8.length + 1);
                if (!entry->info.utf8.bytes) {
                    return J2ME_ERROR_OUT_OF_MEMORY;
                }
                
                memcpy(entry->info.utf8.bytes, data + *offset, entry->info.utf8.length);
                entry->info.utf8.bytes[entry->info.utf8.length] = '\0';
                *offset += entry->info.utf8.length;
                
                // 只在调试模式下打印
                // printf("[类解析器] UTF-8常量 #%d: %s\n", i, entry->info.utf8.bytes);
                break;
            }
            
            case J2ME_CONSTANT_INTEGER:
                entry->info.integer.value = READ_U4(data, *offset);
                *offset += 4;
                // printf("[类解析器] 整数常量 #%d: %d\n", i, entry->info.integer.value);
                break;
                
            case J2ME_CONSTANT_FLOAT:
                // 简化处理，直接读取为uint32_t
                entry->info.float_val.value = *(float*)&data[*offset];
                *offset += 4;
                // printf("[类解析器] 浮点常量 #%d: %f\n", i, entry->info.float_val.value);
                break;
                
            case J2ME_CONSTANT_LONG:
                entry->info.long_val.value = ((uint64_t)READ_U4(data, *offset) << 32) | READ_U4(data, *offset + 4);
                *offset += 8;
                i++; // Long和Double占用两个常量池位置
                // printf("[类解析器] 长整数常量 #%d: %lld\n", i-1, entry->info.long_val.value);
                break;
                
            case J2ME_CONSTANT_DOUBLE:
                // 简化处理
                entry->info.double_val.value = *(double*)&data[*offset];
                *offset += 8;
                i++; // Long和Double占用两个常量池位置
                // printf("[类解析器] 双精度常量 #%d: %f\n", i-1, entry->info.double_val.value);
                break;
                
            case J2ME_CONSTANT_CLASS:
                entry->info.class_info.name_index = READ_U2(data, *offset);
                *offset += 2;
                // printf("[类解析器] 类常量 #%d: name_index=%d\n", i, entry->info.class_info.name_index);
                break;
                
            case J2ME_CONSTANT_STRING:
                entry->info.string_info.string_index = READ_U2(data, *offset);
                *offset += 2;
               // printf("[类解析器] 字符串常量 #%d: string_index=%d\n", i, entry->info.string_info.string_index);
                break;
                
            case J2ME_CONSTANT_FIELDREF:
            case J2ME_CONSTANT_METHODREF:
            case J2ME_CONSTANT_INTERFACE_METHODREF:
                entry->info.ref_info.class_index = READ_U2(data, *offset);
                entry->info.ref_info.name_and_type_index = READ_U2(data, *offset + 2);
                *offset += 4;
               // printf("[类解析器] 引用常量 #%d: class_index=%d, name_and_type_index=%d\n", i, entry->info.ref_info.class_index, entry->info.ref_info.name_and_type_index);
                break;
                
            case J2ME_CONSTANT_NAME_AND_TYPE:
                entry->info.name_and_type_info.name_index = READ_U2(data, *offset);
                entry->info.name_and_type_info.descriptor_index = READ_U2(data, *offset + 2);
                *offset += 4;
               // printf("[类解析器] 名称和类型常量 #%d: name_index=%d, descriptor_index=%d\n", i, entry->info.name_and_type_info.name_index, entry->info.name_and_type_info.descriptor_index);
                break;
                
            default:
               // printf("[类解析器] 错误: 未知的常量池类型 %d\n", entry->tag);
                return J2ME_ERROR_INVALID_PARAMETER;
        }
    }
    
    return J2ME_SUCCESS;
}

/**
 * @brief 解析字段
 * @param data Class文件数据
 * @param offset 当前偏移量指针
 * @param class_ptr 类指针
 * @param size 数据总大小
 * @return 错误码
 */
static j2me_error_t parse_fields(const uint8_t* data, size_t* offset, j2me_class_t* class_ptr, size_t size) {
    if (*offset + 2 > size) {
       // printf("[类解析器] 错误: 读取字段数量时越界\n");
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    class_ptr->fields_count = READ_U2(data, *offset);
    *offset += 2;
    
   // printf("[类解析器] 解析字段，数量: %d\n", class_ptr->fields_count);
    
    if (class_ptr->fields_count == 0) {
        class_ptr->fields = NULL;
        return J2ME_SUCCESS;
    }
    
    class_ptr->fields = (j2me_field_t*)calloc(class_ptr->fields_count, sizeof(j2me_field_t));
    if (!class_ptr->fields) {
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    for (uint16_t i = 0; i < class_ptr->fields_count; i++) {
        j2me_field_t* field = &class_ptr->fields[i];
        
        field->access_flags = READ_U2(data, *offset);
        field->name_index = READ_U2(data, *offset + 2);
        field->descriptor_index = READ_U2(data, *offset + 4);
        field->attributes_count = READ_U2(data, *offset + 6);
        *offset += 8;
        
        // 解析字段名和描述符
        field->name = j2me_constant_pool_get_utf8(&class_ptr->constant_pool, field->name_index);
        field->descriptor = j2me_constant_pool_get_utf8(&class_ptr->constant_pool, field->descriptor_index);
        field->owner_class = class_ptr;
        
       // printf("[类解析器] 字段 #%d: %s %s (访问标志: 0x%04x)\n", 
            //    i, field->name ? field->name : "unknown", 
            //    field->descriptor ? field->descriptor : "unknown", 
            //    field->access_flags);
        
        // 跳过属性 (简化处理)
        for (uint16_t j = 0; j < field->attributes_count; j++) {
            uint16_t attr_name_index = READ_U2(data, *offset);
            uint32_t attr_length = READ_U4(data, *offset + 2);
            *offset += 6 + attr_length;
            
            const char* attr_name = j2me_constant_pool_get_utf8(&class_ptr->constant_pool, attr_name_index);
           // printf("[类解析器] 跳过字段属性: %s (长度: %d)\n", 
                //    attr_name ? attr_name : "unknown", attr_length);
        }
    }
    
    return J2ME_SUCCESS;
}

/**
 * @brief 解析方法
 * @param data Class文件数据
 * @param offset 当前偏移量指针
 * @param class_ptr 类指针
 * @param size 数据总大小
 * @return 错误码
 */
static j2me_error_t parse_methods(const uint8_t* data, size_t* offset, j2me_class_t* class_ptr, size_t size) {
    if (*offset + 2 > size) {
       // printf("[类解析器] 错误: 读取方法数量时越界\n");
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    class_ptr->methods_count = READ_U2(data, *offset);
    *offset += 2;
    
   // printf("[类解析器] 解析方法，数量: %d\n", class_ptr->methods_count);
    
    if (class_ptr->methods_count == 0) {
        class_ptr->methods = NULL;
        return J2ME_SUCCESS;
    }
    
    class_ptr->methods = (j2me_method_t*)calloc(class_ptr->methods_count, sizeof(j2me_method_t));
    if (!class_ptr->methods) {
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    for (uint16_t i = 0; i < class_ptr->methods_count; i++) {
        // printf("[类解析器] 开始解析方法 #%d, 当前偏移: %zu, 文件大小: %zu\n", i, *offset, size);
        
        if (*offset + 8 > size) {
           // printf("[类解析器] 错误: 读取方法 #%d 时越界\n", i);
            return J2ME_ERROR_INVALID_PARAMETER;
        }
        
        j2me_method_t* method = &class_ptr->methods[i];
        
        method->access_flags = READ_U2(data, *offset);
        method->name_index = READ_U2(data, *offset + 2);
        method->descriptor_index = READ_U2(data, *offset + 4);
        method->attributes_count = READ_U2(data, *offset + 6);
        *offset += 8;
        
        // printf("[类解析器] 方法 #%d 头部: access=0x%04x, name_idx=%d, desc_idx=%d, attr_count=%d\n",
        //        i, method->access_flags, method->name_index, method->descriptor_index, method->attributes_count);
        
        // 解析方法名和描述符
        method->name = j2me_constant_pool_get_utf8(&class_ptr->constant_pool, method->name_index);
        method->descriptor = j2me_constant_pool_get_utf8(&class_ptr->constant_pool, method->descriptor_index);
        method->owner_class = class_ptr;
        method->is_native = (method->access_flags & ACC_NATIVE) != 0;
        
        // 只打印简短信息，避免长字符串导致问题
        // printf("[类解析器] 方法 #%d: %s (属性数: %d)\n", 
        //        i, method->name ? method->name : "?", method->attributes_count);
        
        // 检查是否为类初始化方法
        if (method->name && strcmp(method->name, "<clinit>") == 0) {
            class_ptr->clinit = method;
        }
        
        // 解析方法属性 (主要是Code属性)
        for (uint16_t j = 0; j < method->attributes_count; j++) {
            if (*offset + 6 > size) {
               // printf("[类解析器] 错误: 读取方法属性 #%d 时越界\n", j);
                return J2ME_ERROR_INVALID_PARAMETER;
            }
            
            uint16_t attr_name_index = READ_U2(data, *offset);
            uint32_t attr_length = READ_U4(data, *offset + 2);
            *offset += 6;
            
            if (*offset + attr_length > size) {
               // printf("[类解析器] 错误: 属性长度 %d 超出文件大小\n", attr_length);
                return J2ME_ERROR_INVALID_PARAMETER;
            }
            
            const char* attr_name = j2me_constant_pool_get_utf8(&class_ptr->constant_pool, attr_name_index);
            
            if (attr_name && strcmp(attr_name, "Code") == 0) {
                if (*offset + 8 > size) {
                   // printf("[类解析器] 错误: 读取Code属性头时越界\n");
                    return J2ME_ERROR_INVALID_PARAMETER;
                }
                
                // 解析Code属性
                method->max_stack = READ_U2(data, *offset);
                method->max_locals = READ_U2(data, *offset + 2);
                method->bytecode_length = READ_U4(data, *offset + 4);
                *offset += 8;
                
                // printf("[类解析器] Code属性: max_stack=%d, max_locals=%d, code_length=%d, 当前偏移=%zu\n", 
                //        method->max_stack, method->max_locals, method->bytecode_length, *offset);
                
                if (*offset + method->bytecode_length > size) {
                   // printf("[类解析器] 错误: 字节码长度 %d 超出文件大小\n", method->bytecode_length);
                    return J2ME_ERROR_INVALID_PARAMETER;
                }
                
                if (method->bytecode_length > 0) {
                    method->bytecode = (uint8_t*)malloc(method->bytecode_length);
                    if (method->bytecode) {
                        memcpy(method->bytecode, data + *offset, method->bytecode_length);
                    }
                }
                *offset += method->bytecode_length;
                
                // printf("[类解析器] 字节码已复制, 当前偏移=%zu\n", *offset);
                
                // 跳过异常表
                if (*offset + 2 > size) {
                   // printf("[类解析器] 错误: 读取异常表长度时越界\n");
                    return J2ME_ERROR_INVALID_PARAMETER;
                }
                uint16_t exception_table_length = READ_U2(data, *offset);
                *offset += 2;
                
                // printf("[类解析器] 异常表长度: %d, 当前偏移=%zu\n", exception_table_length, *offset);
                
                if (*offset + exception_table_length * 8 > size) {
                   // printf("[类解析器] 错误: 异常表长度 %d 超出文件大小\n", exception_table_length);
                    return J2ME_ERROR_INVALID_PARAMETER;
                }
                *offset += exception_table_length * 8;
                
                // printf("[类解析器] 异常表已跳过, 当前偏移=%zu\n", *offset);
                
                // 跳过Code属性的属性
                if (*offset + 2 > size) {
                   // printf("[类解析器] 错误: 读取Code属性数量时越界\n");
                    return J2ME_ERROR_INVALID_PARAMETER;
                }
                uint16_t code_attributes_count = READ_U2(data, *offset);
                *offset += 2;
                
                // printf("[类解析器] Code属性数量: %d, 当前偏移=%zu\n", code_attributes_count, *offset);
                
                for (uint16_t k = 0; k < code_attributes_count; k++) {
                    if (*offset + 6 > size) {
                       // printf("[类解析器] 错误: 读取Code子属性 #%d 时越界\n", k);
                        return J2ME_ERROR_INVALID_PARAMETER;
                    }
                    uint16_t code_attr_name_index = READ_U2(data, *offset);
                    uint32_t code_attr_length = READ_U4(data, *offset + 2);
                    *offset += 6;
                    
                    // printf("[类解析器] Code子属性 #%d: name_idx=%d, length=%d, 当前偏移=%zu\n",
                    //        k, code_attr_name_index, code_attr_length, *offset);
                    
                    if (*offset + code_attr_length > size) {
                       // printf("[类解析器] 错误: Code子属性长度 %d 超出文件大小\n", code_attr_length);
                        return J2ME_ERROR_INVALID_PARAMETER;
                    }
                    *offset += code_attr_length;
                }
                
                // printf("[类解析器] Code属性解析完成, 当前偏移=%zu\n", *offset);
            } else {
                // 跳过其他属性
                // printf("[类解析器] 跳过方法属性: %s (长度: %d)\n", 
                //        attr_name ? attr_name : "unknown", attr_length);
                *offset += attr_length;
            }
        }
    }
    
    return J2ME_SUCCESS;
}

j2me_class_t* j2me_class_parse(const uint8_t* data, size_t size) {
    if (!data || size < 10) {
        return NULL;
    }
    
   // printf("[类解析器] 开始解析Class文件，大小: %zu bytes\n", size);
    
    j2me_class_t* class_ptr = (j2me_class_t*)calloc(1, sizeof(j2me_class_t));
    if (!class_ptr) {
        return NULL;
    }
    
    size_t offset = 0;
    
    // 读取基本信息
    class_ptr->magic = READ_U4(data, offset);
    offset += 4;
    
    if (class_ptr->magic != 0xCAFEBABE) {
       // printf("[类解析器] 错误: 无效的魔数 0x%08x\n", class_ptr->magic);
        free(class_ptr);
        return NULL;
    }
    
    class_ptr->minor_version = READ_U2(data, offset);
    class_ptr->major_version = READ_U2(data, offset + 2);
    offset += 4;
    
   // printf("[类解析器] Class文件版本: %d.%d\n", class_ptr->major_version, class_ptr->minor_version);
    
    // 解析常量池
    j2me_error_t result = parse_constant_pool(data, &offset, &class_ptr->constant_pool);
    if (result != J2ME_SUCCESS) {
        j2me_class_destroy(class_ptr);
        return NULL;
    }
    
    // 读取类信息
    class_ptr->access_flags = READ_U2(data, offset);
    class_ptr->this_class = READ_U2(data, offset + 2);
    class_ptr->super_class = READ_U2(data, offset + 4);
    offset += 6;
    
    // 解析类名
    class_ptr->name = j2me_constant_pool_get_class_name(&class_ptr->constant_pool, class_ptr->this_class);
    if (class_ptr->super_class != 0) {
        class_ptr->super_name = j2me_constant_pool_get_class_name(&class_ptr->constant_pool, class_ptr->super_class);
    }
    
    printf("[类解析器] 类名: %s, 父类: %s\n", 
           class_ptr->name ? class_ptr->name : "unknown",
           class_ptr->super_name ? class_ptr->super_name : "none");
    
    // 解析接口
    class_ptr->interfaces_count = READ_U2(data, offset);
    offset += 2;
    
    if (class_ptr->interfaces_count > 0) {
        class_ptr->interfaces = (uint16_t*)malloc(sizeof(uint16_t) * class_ptr->interfaces_count);
        if (!class_ptr->interfaces) {
            j2me_class_destroy(class_ptr);
            return NULL;
        }
        
        printf("[类解析器] 实现的接口数量: %d\n", class_ptr->interfaces_count);
        for (uint16_t i = 0; i < class_ptr->interfaces_count; i++) {
            class_ptr->interfaces[i] = READ_U2(data, offset);
            offset += 2;
            
            // 获取接口名称
            const char* interface_name = j2me_constant_pool_get_class_name(
                &class_ptr->constant_pool, class_ptr->interfaces[i]);
            printf("[类解析器]   接口 %d: %s\n", i, interface_name ? interface_name : "unknown");
        }
    } else {
        class_ptr->interfaces = NULL;
    }
    
    // 解析字段
    result = parse_fields(data, &offset, class_ptr, size);
    if (result != J2ME_SUCCESS) {
        j2me_class_destroy(class_ptr);
        return NULL;
    }
    
    // 解析方法
    result = parse_methods(data, &offset, class_ptr, size);
    if (result != J2ME_SUCCESS) {
        j2me_class_destroy(class_ptr);
        return NULL;
    }
    
    // 跳过类属性 (简化处理)
    uint16_t attributes_count = READ_U2(data, offset);
    offset += 2;
    for (uint16_t i = 0; i < attributes_count; i++) {
        uint16_t attr_name_index = READ_U2(data, offset);
        uint32_t attr_length = READ_U4(data, offset + 2);
        offset += 6 + attr_length;
    }
    
   // printf("[类解析器] Class文件解析完成: %s\n", class_ptr->name ? class_ptr->name : "unknown");
    return class_ptr;
}

void j2me_class_destroy(j2me_class_t* class_ptr) {
    if (!class_ptr) {
        return;
    }
    
    // 释放常量池
    if (class_ptr->constant_pool.entries) {
        for (uint16_t i = 0; i < class_ptr->constant_pool.count - 1; i++) {
            j2me_constant_pool_entry_t* entry = &class_ptr->constant_pool.entries[i];
            if (entry->tag == J2ME_CONSTANT_UTF8 && entry->info.utf8.bytes) {
                free(entry->info.utf8.bytes);
            }
        }
        free(class_ptr->constant_pool.entries);
    }
    
    // 释放字段
    if (class_ptr->fields) {
        free(class_ptr->fields);
    }
    
    // 释放方法
    if (class_ptr->methods) {
        for (uint16_t i = 0; i < class_ptr->methods_count; i++) {
            if (class_ptr->methods[i].bytecode) {
                free(class_ptr->methods[i].bytecode);
            }
        }
        free(class_ptr->methods);
    }
    
    // 释放接口数组
    if (class_ptr->interfaces) {
        free(class_ptr->interfaces);
    }
    
    free(class_ptr);
}