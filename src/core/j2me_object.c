#include "j2me_object.h"
#include "j2me_vm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @file j2me_object.c
 * @brief J2ME对象系统实现
 * 
 * 实现Java对象的创建、销毁和字段访问功能
 */

// 元素类型大小表
static const uint8_t element_sizes[] = {
    0, 0, 0, 0,
    1,  // ARRAY_TYPE_BOOLEAN
    2,  // ARRAY_TYPE_CHAR
    4,  // ARRAY_TYPE_FLOAT
    8,  // ARRAY_TYPE_DOUBLE
    1,  // ARRAY_TYPE_BYTE
    2,  // ARRAY_TYPE_SHORT
    4,  // ARRAY_TYPE_INT
    8,  // ARRAY_TYPE_LONG
    sizeof(void*)  // ARRAY_TYPE_REFERENCE
};

/**
 * @brief 从虚拟机堆分配内存
 * @param vm 虚拟机实例
 * @param size 分配大小
 * @return 内存指针，失败返回NULL
 */
static void* vm_heap_alloc(j2me_vm_t* vm, size_t size) {
    if (!vm || !vm->heap_start) {
        return NULL;
    }
    
    // 简单的线性分配器 (实际实现应该更复杂)
    size_t aligned_size = (size + 7) & ~7; // 8字节对齐
    
    if ((char*)vm->heap_current + aligned_size > (char*)vm->heap_end) {
        printf("[对象系统] 堆内存不足，需要 %zu 字节\n", aligned_size);
        return NULL;
    }
    
    void* ptr = vm->heap_current;
    vm->heap_current = (char*)vm->heap_current + aligned_size;
    
    memset(ptr, 0, aligned_size);
    return ptr;
}

size_t j2me_object_calculate_size(j2me_class_t* class_ptr) {
    if (!class_ptr) {
        return 0;
    }
    
    size_t size = sizeof(j2me_object_header_t);
    
    // 计算实例字段大小
    for (uint16_t i = 0; i < class_ptr->fields_count; i++) {
        j2me_field_t* field = &class_ptr->fields[i];
        
        // 跳过静态字段
        if (field->access_flags & ACC_STATIC) {
            continue;
        }
        
        // 根据字段描述符计算大小 (简化实现)
        if (field->descriptor) {
            switch (field->descriptor[0]) {
                case 'Z': // boolean
                case 'B': // byte
                    size += 1;
                    break;
                case 'C': // char
                case 'S': // short
                    size += 2;
                    break;
                case 'I': // int
                case 'F': // float
                    size += 4;
                    break;
                case 'J': // long
                case 'D': // double
                    size += 8;
                    break;
                case 'L': // 引用类型
                case '[': // 数组类型
                    size += sizeof(void*);
                    break;
                default:
                    size += 4; // 默认大小
                    break;
            }
        } else {
            size += 4; // 默认大小
        }
    }
    
    // 包含父类字段
    if (class_ptr->super_class_ptr) {
        size += j2me_object_calculate_size(class_ptr->super_class_ptr) - sizeof(j2me_object_header_t);
    }
    
    return size;
}

j2me_object_t* j2me_object_create(j2me_vm_t* vm, j2me_class_t* class_ptr) {
    if (!vm || !class_ptr) {
        return NULL;
    }
    
    // 确保类已初始化
    if (class_ptr->state != CLASS_INITIALIZED) {
        j2me_error_t result = j2me_class_initialize(class_ptr);
        if (result != J2ME_SUCCESS) {
            printf("[对象系统] 类初始化失败: %s\n", class_ptr->name ? class_ptr->name : "unknown");
            return NULL;
        }
    }
    
    size_t object_size = j2me_object_calculate_size(class_ptr);
    j2me_object_t* obj = (j2me_object_t*)vm_heap_alloc(vm, object_size);
    
    if (!obj) {
        printf("[对象系统] 对象创建失败，内存不足\n");
        return NULL;
    }
    
    // 初始化对象头
    obj->header.class_ptr = class_ptr;
    obj->header.hash_code = (uint32_t)(uintptr_t)obj; // 简单的哈希码
    obj->header.flags = 0;
    obj->header.lock_count = 0;
    
    printf("[对象系统] 创建对象: %s (大小: %zu 字节)\n", 
           class_ptr->name ? class_ptr->name : "unknown", object_size);
    
    return obj;
}

void j2me_object_destroy(j2me_vm_t* vm, j2me_object_t* obj) {
    if (!vm || !obj) {
        return;
    }
    
    // 在实际的GC实现中，这里会被GC管理
    // 现在只是标记为已销毁
    obj->header.flags |= OBJECT_FLAG_FINALIZED;
    
    printf("[对象系统] 销毁对象: %s\n", 
           obj->header.class_ptr && obj->header.class_ptr->name ? 
           obj->header.class_ptr->name : "unknown");
}

j2me_class_t* j2me_object_get_class(j2me_object_t* obj) {
    return obj ? obj->header.class_ptr : NULL;
}

/**
 * @brief 计算字段在对象中的偏移量
 * @param class_ptr 对象的类
 * @param field 字段
 * @return 字段偏移量
 */
static size_t calculate_field_offset(j2me_class_t* class_ptr, j2me_field_t* field) {
    size_t offset = sizeof(j2me_object_header_t);
    
    // 包含父类字段
    if (class_ptr->super_class_ptr) {
        offset += j2me_object_calculate_size(class_ptr->super_class_ptr) - sizeof(j2me_object_header_t);
    }
    
    // 计算当前类中字段的偏移量
    for (uint16_t i = 0; i < class_ptr->fields_count; i++) {
        j2me_field_t* current_field = &class_ptr->fields[i];
        
        // 跳过静态字段
        if (current_field->access_flags & ACC_STATIC) {
            continue;
        }
        
        if (current_field == field) {
            return offset;
        }
        
        // 根据字段类型增加偏移量
        if (current_field->descriptor) {
            switch (current_field->descriptor[0]) {
                case 'Z': case 'B': offset += 1; break;
                case 'C': case 'S': offset += 2; break;
                case 'I': case 'F': offset += 4; break;
                case 'J': case 'D': offset += 8; break;
                default: offset += sizeof(void*); break;
            }
        } else {
            offset += 4;
        }
    }
    
    return 0; // 字段未找到
}

j2me_int j2me_object_get_field_int(j2me_object_t* obj, j2me_field_t* field) {
    if (!obj || !field) {
        return 0;
    }
    
    size_t offset = calculate_field_offset(obj->header.class_ptr, field);
    if (offset == 0) {
        return 0;
    }
    
    return *(j2me_int*)((char*)obj + offset);
}

void j2me_object_set_field_int(j2me_object_t* obj, j2me_field_t* field, j2me_int value) {
    if (!obj || !field) {
        return;
    }
    
    size_t offset = calculate_field_offset(obj->header.class_ptr, field);
    if (offset == 0) {
        return;
    }
    
    *(j2me_int*)((char*)obj + offset) = value;
}

j2me_reference j2me_object_get_field_ref(j2me_object_t* obj, j2me_field_t* field) {
    if (!obj || !field) {
        return NULL;
    }
    
    size_t offset = calculate_field_offset(obj->header.class_ptr, field);
    if (offset == 0) {
        return NULL;
    }
    
    return *(j2me_reference*)((char*)obj + offset);
}

void j2me_object_set_field_ref(j2me_object_t* obj, j2me_field_t* field, j2me_reference value) {
    if (!obj || !field) {
        return;
    }
    
    size_t offset = calculate_field_offset(obj->header.class_ptr, field);
    if (offset == 0) {
        return;
    }
    
    *(j2me_reference*)((char*)obj + offset) = value;
}

size_t j2me_array_calculate_size(j2me_array_type_t element_type, uint32_t length) {
    if (element_type >= sizeof(element_sizes) / sizeof(element_sizes[0])) {
        return 0;
    }
    
    size_t header_size = sizeof(j2me_array_t);
    size_t element_size = element_sizes[element_type];
    size_t data_size = element_size * length;
    
    return header_size + data_size;
}

j2me_array_t* j2me_array_create(j2me_vm_t* vm, j2me_array_type_t element_type, uint32_t length) {
    if (!vm || element_type >= sizeof(element_sizes) / sizeof(element_sizes[0])) {
        return NULL;
    }
    
    size_t array_size = j2me_array_calculate_size(element_type, length);
    j2me_array_t* array = (j2me_array_t*)vm_heap_alloc(vm, array_size);
    
    if (!array) {
        printf("[对象系统] 数组创建失败，内存不足\n");
        return NULL;
    }
    
    // 初始化数组头
    array->header.class_ptr = NULL; // TODO: 设置数组类
    array->header.hash_code = (uint32_t)(uintptr_t)array;
    array->header.flags = OBJECT_FLAG_ARRAY;
    array->header.lock_count = 0;
    
    array->length = length;
    array->element_size = element_sizes[element_type];
    array->element_type = element_type;
    array->padding = 0;
    
    printf("[对象系统] 创建数组: 类型=%d, 长度=%d, 大小=%zu 字节\n", 
           element_type, length, array_size);
    
    return array;
}

j2me_array_t* j2me_array_create_ref(j2me_vm_t* vm, j2me_class_t* element_class, uint32_t length) {
    j2me_array_t* array = j2me_array_create(vm, ARRAY_TYPE_REFERENCE, length);
    if (array) {
        // TODO: 设置正确的数组类型
        printf("[对象系统] 创建引用数组: 元素类型=%s, 长度=%d\n", 
               element_class && element_class->name ? element_class->name : "unknown", length);
    }
    return array;
}

uint32_t j2me_array_get_length(j2me_array_t* array) {
    return array ? array->length : 0;
}

/**
 * @brief 获取数组数据指针
 * @param array 数组对象
 * @return 数据指针
 */
static void* get_array_data(j2me_array_t* array) {
    return (char*)array + sizeof(j2me_array_t);
}

j2me_int j2me_array_get_int(j2me_array_t* array, uint32_t index) {
    if (!array || index >= array->length || array->element_type != ARRAY_TYPE_INT) {
        return 0;
    }
    
    j2me_int* data = (j2me_int*)get_array_data(array);
    return data[index];
}

void j2me_array_set_int(j2me_array_t* array, uint32_t index, j2me_int value) {
    if (!array || index >= array->length || array->element_type != ARRAY_TYPE_INT) {
        return;
    }
    
    j2me_int* data = (j2me_int*)get_array_data(array);
    data[index] = value;
}

j2me_reference j2me_array_get_ref(j2me_array_t* array, uint32_t index) {
    if (!array || index >= array->length || array->element_type != ARRAY_TYPE_REFERENCE) {
        return NULL;
    }
    
    j2me_reference* data = (j2me_reference*)get_array_data(array);
    return data[index];
}

void j2me_array_set_ref(j2me_array_t* array, uint32_t index, j2me_reference value) {
    if (!array || index >= array->length || array->element_type != ARRAY_TYPE_REFERENCE) {
        return;
    }
    
    j2me_reference* data = (j2me_reference*)get_array_data(array);
    data[index] = value;
}

// 旧的String实现已移至j2me_string.c（基于新的堆系统）
// 这些函数已被注释掉以避免符号冲突

/*
j2me_string_t* j2me_string_create(j2me_vm_t* vm, const j2me_char* chars, uint32_t length) {
    // 已移至j2me_string.c
    return NULL;
}

j2me_string_t* j2me_string_create_from_cstr(j2me_vm_t* vm, const char* cstr) {
    // 已移至j2me_string.c
    return NULL;
}

uint32_t j2me_string_get_length(j2me_string_t* str) {
    // 已移至j2me_string.c
    return 0;
}

const j2me_char* j2me_string_get_chars(j2me_string_t* str) {
    // 已移至j2me_string.c
    return NULL;
}

int j2me_string_compare(j2me_string_t* str1, j2me_string_t* str2) {
    // 已移至j2me_string.c
    return 0;
}
*/

bool j2me_object_instanceof(j2me_object_t* obj, j2me_class_t* class_ptr) {
    if (!obj || !class_ptr) {
        return false;
    }
    
    j2me_class_t* obj_class = obj->header.class_ptr;
    
    // 检查类层次结构
    while (obj_class) {
        if (obj_class == class_ptr) {
            return true;
        }
        
        // 检查类名 (如果指针不同但名称相同)
        if (obj_class->name && class_ptr->name && 
            strcmp(obj_class->name, class_ptr->name) == 0) {
            return true;
        }
        
        obj_class = obj_class->super_class_ptr;
    }
    
    return false;
}

bool j2me_object_checkcast(j2me_object_t* obj, j2me_class_t* target_class) {
    if (!obj) {
        return true; // null可以转换为任何类型
    }
    
    return j2me_object_instanceof(obj, target_class);
}