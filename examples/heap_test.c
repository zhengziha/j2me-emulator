#include "j2me_heap.h"
#include "j2me_string.h"
#include "j2me_log.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/**
 * @file heap_test.c
 * @brief 堆和String对象测试程序
 * 
 * 测试基础对象系统的功能
 */

int main(void) {
    LOG_DEBUG("=== J2ME堆和String对象测试 ===\n\n");
    
    // 测试1: 创建堆
    LOG_DEBUG("测试1: 创建堆\n");
    j2me_heap_t* heap = j2me_heap_create(1024 * 1024); // 1MB堆
    assert(heap != NULL);
    LOG_DEBUG("✓ 堆创建成功\n\n");
    
    // 测试2: 分配对象
    LOG_DEBUG("测试2: 分配对象\n");
    j2me_ref_t ref1 = j2me_heap_alloc(heap, 100, 64);
    assert(ref1 != J2ME_NULL_REF);
    LOG_DEBUG("✓ 对象分配成功: ref=0x%x\n\n", ref1);
    
    // 测试3: 获取对象
    LOG_DEBUG("测试3: 获取对象\n");
    j2me_object_header_t* obj1 = j2me_heap_get_object(heap, ref1);
    assert(obj1 != NULL);
    assert(obj1->class_id == 100);
    assert(obj1->size == 64);
    LOG_DEBUG("✓ 对象获取成功: class_id=%u, size=%u\n\n", obj1->class_id, obj1->size);
    
    // 测试4: 创建String对象
    LOG_DEBUG("测试4: 创建String对象\n");
    j2me_ref_t str1 = j2me_string_create(heap, "Hello J2ME!");
    assert(str1 != J2ME_NULL_REF);
    j2me_string_print(heap, str1);
    LOG_DEBUG("✓ String对象创建成功\n\n");
    
    // 测试5: 获取String内容
    LOG_DEBUG("测试5: 获取String内容\n");
    const char* chars = j2me_heap_string_get_chars(heap, str1);
    assert(chars != NULL);
    LOG_DEBUG("  内容: \"%s\"\n", chars);
    
    uint32_t length = j2me_string_get_length(heap, str1);
    LOG_DEBUG("  长度: %u\n", length);
    assert(length == 11);
    LOG_DEBUG("✓ String内容获取成功\n\n");
    
    // 测试6: charAt
    LOG_DEBUG("测试6: charAt\n");
    char ch = j2me_string_char_at(heap, str1, 0);
    LOG_DEBUG("  charAt(0) = '%c'\n", ch);
    assert(ch == 'H');
    
    ch = j2me_string_char_at(heap, str1, 6);
    LOG_DEBUG("  charAt(6) = '%c'\n", ch);
    assert(ch == 'J');
    LOG_DEBUG("✓ charAt成功\n\n");
    
    // 测试7: 连接String
    LOG_DEBUG("测试7: 连接String\n");
    j2me_ref_t str2 = j2me_string_create(heap, " Welcome!");
    j2me_ref_t str3 = j2me_string_concat(heap, str1, str2);
    assert(str3 != J2ME_NULL_REF);
    j2me_string_print(heap, str3);
    LOG_DEBUG("✓ String连接成功\n\n");
    
    // 测试8: 子串
    LOG_DEBUG("测试8: 子串\n");
    j2me_ref_t str4 = j2me_string_substring(heap, str1, 0, 5);
    assert(str4 != J2ME_NULL_REF);
    j2me_string_print(heap, str4);
    
    const char* substr = j2me_heap_string_get_chars(heap, str4);
    assert(strcmp(substr, "Hello") == 0);
    LOG_DEBUG("✓ 子串成功\n\n");
    
    // 测试9: 比较String
    LOG_DEBUG("测试9: 比较String\n");
    j2me_ref_t str5 = j2me_string_create(heap, "Hello J2ME!");
    int cmp = j2me_string_compare(heap, str1, str5);
    LOG_DEBUG("  compare(str1, str5) = %d\n", cmp);
    assert(cmp == 0);
    
    cmp = j2me_string_compare(heap, str1, str2);
    LOG_DEBUG("  compare(str1, str2) = %d\n", cmp);
    assert(cmp != 0);
    LOG_DEBUG("✓ String比较成功\n\n");
    
    // 测试10: 引用计数
    LOG_DEBUG("测试10: 引用计数\n");
    j2me_heap_retain(heap, str1);
    j2me_heap_retain(heap, str1);
    j2me_heap_release(heap, str1);
    LOG_DEBUG("✓ 引用计数成功\n\n");
    
    // 测试11: 堆统计
    LOG_DEBUG("测试11: 堆统计\n");
    j2me_heap_print_stats(heap);
    
    size_t used, total, objects;
    j2me_heap_get_stats(heap, &used, &total, &objects);
    LOG_DEBUG("  已使用: %zu bytes\n", used);
    LOG_DEBUG("  总大小: %zu bytes\n", total);
    LOG_DEBUG("  对象数: %zu\n", objects);
    LOG_DEBUG("✓ 堆统计成功\n\n");
    
    // 测试12: 多个String对象
    LOG_DEBUG("测试12: 创建多个String对象\n");
    for (int i = 0; i < 10; i++) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "String #%d", i);
        j2me_ref_t str = j2me_string_create(heap, buffer);
        assert(str != J2ME_NULL_REF);
        LOG_DEBUG("  创建: ref=0x%x, content=\"%s\"\n", str, buffer);
    }
    LOG_DEBUG("✓ 多个String对象创建成功\n\n");
    
    // 最终统计
    LOG_DEBUG("=== 最终堆统计 ===\n");
    j2me_heap_print_stats(heap);
    
    // 清理
    LOG_DEBUG("清理堆...\n");
    j2me_heap_destroy(heap);
    
    LOG_DEBUG("\n=== 所有测试通过! ===\n");
    return 0;
}
