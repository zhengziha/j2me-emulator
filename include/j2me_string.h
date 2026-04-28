#ifndef J2ME_STRING_H
#define J2ME_STRING_H

#include "j2me_heap.h"
#include "j2me_types.h"

/**
 * @file j2me_string.h
 * @brief J2ME String对象实现
 * 
 * 实现Java String对象的创建、存储和访问
 */

// String对象数据结构
typedef struct {
    uint32_t length;        // 字符串长度
    char chars[];           // 字符数据（UTF-8编码）
} j2me_string_data_t;

// String类ID
#define J2ME_CLASS_STRING 1

/**
 * @brief 创建String对象
 * @param heap 堆指针
 * @param str C字符串
 * @return String对象引用，失败返回J2ME_NULL_REF
 */
j2me_ref_t j2me_heap_string_create(j2me_heap_t* heap, const char* str);

/**
 * @brief 创建String对象（指定长度）
 * @param heap 堆指针
 * @param str C字符串
 * @param length 字符串长度
 * @return String对象引用，失败返回J2ME_NULL_REF
 */
j2me_ref_t j2me_heap_string_create_n(j2me_heap_t* heap, const char* str, size_t length);

/**
 * @brief 获取String对象的C字符串
 * @param heap 堆指针
 * @param ref String对象引用
 * @return C字符串指针，失败返回NULL
 */
const char* j2me_heap_string_get_chars(j2me_heap_t* heap, j2me_ref_t ref);

/**
 * @brief 获取String对象的长度
 * @param heap 堆指针
 * @param ref String对象引用
 * @return 字符串长度，失败返回0
 */
uint32_t j2me_heap_string_get_length(j2me_heap_t* heap, j2me_ref_t ref);

/**
 * @brief 获取String对象指定位置的字符
 * @param heap 堆指针
 * @param ref String对象引用
 * @param index 字符索引
 * @return 字符，失败返回0
 */
char j2me_heap_string_char_at(j2me_heap_t* heap, j2me_ref_t ref, uint32_t index);

/**
 * @brief 连接两个String对象
 * @param heap 堆指针
 * @param ref1 第一个String对象引用
 * @param ref2 第二个String对象引用
 * @return 新的String对象引用，失败返回J2ME_NULL_REF
 */
j2me_ref_t j2me_heap_string_concat(j2me_heap_t* heap, j2me_ref_t ref1, j2me_ref_t ref2);

/**
 * @brief 获取String对象的子串
 * @param heap 堆指针
 * @param ref String对象引用
 * @param start 起始索引
 * @param end 结束索引（不包括）
 * @return 新的String对象引用，失败返回J2ME_NULL_REF
 */
j2me_ref_t j2me_heap_string_substring(j2me_heap_t* heap, j2me_ref_t ref, uint32_t start, uint32_t end);

/**
 * @brief 比较两个String对象
 * @param heap 堆指针
 * @param ref1 第一个String对象引用
 * @param ref2 第二个String对象引用
 * @return 0表示相等，<0表示ref1<ref2，>0表示ref1>ref2
 */
int j2me_heap_string_compare(j2me_heap_t* heap, j2me_ref_t ref1, j2me_ref_t ref2);

/**
 * @brief 打印String对象（调试用）
 * @param heap 堆指针
 * @param ref String对象引用
 */
void j2me_heap_string_print(j2me_heap_t* heap, j2me_ref_t ref);

#endif // J2ME_STRING_H
