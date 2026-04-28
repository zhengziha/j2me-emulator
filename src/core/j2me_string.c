#include "j2me_string.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @file j2me_string.c
 * @brief J2ME String对象实现
 */

j2me_ref_t j2me_heap_string_create(j2me_heap_t* heap, const char* str) {
    if (!heap || !str) {
        return J2ME_NULL_REF;
    }
    
    size_t length = strlen(str);
    return j2me_heap_string_create_n(heap, str, length);
}

j2me_ref_t j2me_heap_string_create_n(j2me_heap_t* heap, const char* str, size_t length) {
    if (!heap || !str) {
        return J2ME_NULL_REF;
    }
    
    // 计算String对象数据大小
    size_t data_size = sizeof(j2me_string_data_t) + length + 1; // +1 for null terminator
    
    // 在堆上分配String对象
    j2me_ref_t ref = j2me_heap_alloc(heap, J2ME_CLASS_STRING, data_size);
    if (ref == J2ME_NULL_REF) {
        printf("[String] 错误: 无法分配String对象\n");
        return J2ME_NULL_REF;
    }
    
    // 获取对象数据指针
    j2me_string_data_t* string_data = (j2me_string_data_t*)j2me_heap_get_object_data(heap, ref);
    if (!string_data) {
        printf("[String] 错误: 无法获取String对象数据\n");
        j2me_heap_free(heap, ref);
        return J2ME_NULL_REF;
    }
    
    // 设置字符串数据
    string_data->length = length;
    memcpy(string_data->chars, str, length);
    string_data->chars[length] = '\0'; // null terminator
    
    printf("[String] 创建String对象: ref=0x%x, length=%u, content=\"%s\"\n", 
           ref, (uint32_t)length, string_data->chars);
    
    return ref;
}

const char* j2me_heap_string_get_chars(j2me_heap_t* heap, j2me_ref_t ref) {
    if (!heap || ref == J2ME_NULL_REF) {
        return NULL;
    }
    
    j2me_string_data_t* string_data = (j2me_string_data_t*)j2me_heap_get_object_data(heap, ref);
    if (!string_data) {
        return NULL;
    }
    
    return string_data->chars;
}

uint32_t j2me_heap_string_get_length(j2me_heap_t* heap, j2me_ref_t ref) {
    if (!heap || ref == J2ME_NULL_REF) {
        return 0;
    }
    
    j2me_string_data_t* string_data = (j2me_string_data_t*)j2me_heap_get_object_data(heap, ref);
    if (!string_data) {
        return 0;
    }
    
    return string_data->length;
}

char j2me_heap_string_char_at(j2me_heap_t* heap, j2me_ref_t ref, uint32_t index) {
    if (!heap || ref == J2ME_NULL_REF) {
        return 0;
    }
    
    j2me_string_data_t* string_data = (j2me_string_data_t*)j2me_heap_get_object_data(heap, ref);
    if (!string_data || index >= string_data->length) {
        return 0;
    }
    
    return string_data->chars[index];
}

j2me_ref_t j2me_heap_string_concat(j2me_heap_t* heap, j2me_ref_t ref1, j2me_ref_t ref2) {
    if (!heap || ref1 == J2ME_NULL_REF || ref2 == J2ME_NULL_REF) {
        return J2ME_NULL_REF;
    }
    
    const char* str1 = j2me_heap_string_get_chars(heap, ref1);
    const char* str2 = j2me_heap_string_get_chars(heap, ref2);
    
    if (!str1 || !str2) {
        return J2ME_NULL_REF;
    }
    
    uint32_t len1 = j2me_heap_string_get_length(heap, ref1);
    uint32_t len2 = j2me_heap_string_get_length(heap, ref2);
    uint32_t total_len = len1 + len2;
    
    // 创建临时缓冲区
    char* buffer = (char*)malloc(total_len + 1);
    if (!buffer) {
        return J2ME_NULL_REF;
    }
    
    memcpy(buffer, str1, len1);
    memcpy(buffer + len1, str2, len2);
    buffer[total_len] = '\0';
    
    // 创建新的String对象
    j2me_ref_t result = j2me_heap_string_create_n(heap, buffer, total_len);
    
    free(buffer);
    
    printf("[String] 连接String: 0x%x + 0x%x = 0x%x\n", ref1, ref2, result);
    
    return result;
}

j2me_ref_t j2me_heap_string_substring(j2me_heap_t* heap, j2me_ref_t ref, uint32_t start, uint32_t end) {
    if (!heap || ref == J2ME_NULL_REF) {
        return J2ME_NULL_REF;
    }
    
    j2me_string_data_t* string_data = (j2me_string_data_t*)j2me_heap_get_object_data(heap, ref);
    if (!string_data) {
        return J2ME_NULL_REF;
    }
    
    // 检查边界
    if (start > end || end > string_data->length) {
        printf("[String] 错误: substring索引越界 (start=%u, end=%u, length=%u)\n", 
               start, end, string_data->length);
        return J2ME_NULL_REF;
    }
    
    uint32_t sub_len = end - start;
    
    // 创建新的String对象
    j2me_ref_t result = j2me_heap_string_create_n(heap, string_data->chars + start, sub_len);
    
    printf("[String] 子串: 0x%x[%u:%u] = 0x%x\n", ref, start, end, result);
    
    return result;
}

int j2me_heap_string_compare(j2me_heap_t* heap, j2me_ref_t ref1, j2me_ref_t ref2) {
    if (!heap) {
        return 0;
    }
    
    // 处理NULL引用
    if (ref1 == J2ME_NULL_REF && ref2 == J2ME_NULL_REF) {
        return 0;
    }
    if (ref1 == J2ME_NULL_REF) {
        return -1;
    }
    if (ref2 == J2ME_NULL_REF) {
        return 1;
    }
    
    const char* str1 = j2me_heap_string_get_chars(heap, ref1);
    const char* str2 = j2me_heap_string_get_chars(heap, ref2);
    
    if (!str1 || !str2) {
        return 0;
    }
    
    return strcmp(str1, str2);
}

void j2me_heap_string_print(j2me_heap_t* heap, j2me_ref_t ref) {
    if (!heap || ref == J2ME_NULL_REF) {
        printf("[String] (null)\n");
        return;
    }
    
    const char* str = j2me_heap_string_get_chars(heap, ref);
    uint32_t length = j2me_heap_string_get_length(heap, ref);
    
    if (str) {
        printf("[String] ref=0x%x, length=%u, content=\"%s\"\n", ref, length, str);
    } else {
        printf("[String] ref=0x%x (invalid)\n", ref);
    }
}
