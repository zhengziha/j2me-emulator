#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "../include/j2me_vm.h"
#include "../include/j2me_gc.h"
#include "../include/j2me_object.h"

/**
 * @file gc_test.c
 * @brief J2ME垃圾回收系统测试程序
 * 
 * 测试垃圾回收器的各种功能和性能
 */

// 测试对象类型ID
#define TEST_OBJECT_TYPE_SIMPLE     1
#define TEST_OBJECT_TYPE_ARRAY      2
#define TEST_OBJECT_TYPE_STRING     3

// 测试函数声明
void test_gc_basic_allocation(j2me_gc_t* gc);
void test_gc_collection(j2me_gc_t* gc);
void test_gc_root_management(j2me_gc_t* gc);
void test_gc_fragmentation(j2me_gc_t* gc);
void test_gc_performance(j2me_gc_t* gc);
void test_gc_stress(j2me_gc_t* gc);
void print_test_header(const char* test_name);
void print_test_result(const char* test_name, bool passed);

int main() {
    printf("=== J2ME垃圾回收系统测试 ===\n\n");
    
    // 创建虚拟机配置
    j2me_vm_config_t config = j2me_vm_get_default_config();
    config.heap_size = 1024 * 1024; // 1MB堆
    
    // 创建虚拟机
    j2me_vm_t* vm = j2me_vm_create(&config);
    if (!vm) {
        printf("错误: 虚拟机创建失败\n");
        return 1;
    }
    
    // 初始化虚拟机
    j2me_error_t result = j2me_vm_initialize(vm);
    if (result != J2ME_SUCCESS) {
        printf("错误: 虚拟机初始化失败: %d\n", result);
        j2me_vm_destroy(vm);
        return 1;
    }
    
    // 获取垃圾回收器
    j2me_gc_t* gc = vm->gc;
    if (!gc) {
        printf("错误: 垃圾回收器未初始化\n");
        j2me_vm_destroy(vm);
        return 1;
    }
    
    printf("虚拟机和垃圾回收器初始化成功\n\n");
    
    // 运行测试
    test_gc_basic_allocation(gc);
    test_gc_collection(gc);
    test_gc_root_management(gc);
    test_gc_fragmentation(gc);
    test_gc_performance(gc);
    test_gc_stress(gc);
    
    // 打印最终统计信息
    printf("\n=== 最终GC统计信息 ===\n");
    j2me_gc_print_stats(gc);
    
    // 清理
    j2me_vm_destroy(vm);
    
    printf("=== 所有测试完成 ===\n");
    return 0;
}

void test_gc_basic_allocation(j2me_gc_t* gc) {
    print_test_header("基础内存分配测试");
    
    bool test_passed = true;
    
    // 测试1: 基本分配
    void* ptr1 = j2me_gc_allocate(gc, 100, TEST_OBJECT_TYPE_SIMPLE);
    if (!ptr1) {
        printf("  错误: 基本分配失败\n");
        test_passed = false;
    } else {
        printf("  ✓ 基本分配成功: %p (100 bytes)\n", ptr1);
    }
    
    // 测试2: 大对象分配
    void* ptr2 = j2me_gc_allocate(gc, 10000, TEST_OBJECT_TYPE_ARRAY);
    if (!ptr2) {
        printf("  错误: 大对象分配失败\n");
        test_passed = false;
    } else {
        printf("  ✓ 大对象分配成功: %p (10000 bytes)\n", ptr2);
    }
    
    // 测试3: 多个小对象分配
    void* ptrs[10];
    int allocated_count = 0;
    for (int i = 0; i < 10; i++) {
        ptrs[i] = j2me_gc_allocate(gc, 50 + i * 10, TEST_OBJECT_TYPE_STRING);
        if (ptrs[i]) {
            allocated_count++;
        }
    }
    printf("  ✓ 小对象分配: %d/10 成功\n", allocated_count);
    
    // 测试4: 零大小分配（应该失败）
    void* ptr_zero = j2me_gc_allocate(gc, 0, TEST_OBJECT_TYPE_SIMPLE);
    if (ptr_zero) {
        printf("  错误: 零大小分配应该失败\n");
        test_passed = false;
    } else {
        printf("  ✓ 零大小分配正确失败\n");
    }
    
    // 打印堆使用情况
    size_t used, free, total;
    j2me_gc_get_heap_info(gc, &used, &free, &total);
    printf("  堆使用情况: %zu/%zu bytes (%.1f%%)\n", 
           used, total, (double)used * 100.0 / total);
    
    print_test_result("基础内存分配测试", test_passed);
}

void test_gc_collection(j2me_gc_t* gc) {
    print_test_header("垃圾回收测试");
    
    bool test_passed = true;
    
    // 获取初始状态
    j2me_gc_stats_t initial_stats = j2me_gc_get_stats(gc);
    size_t initial_used, initial_free, total;
    j2me_gc_get_heap_info(gc, &initial_used, &initial_free, &total);
    
    printf("  初始堆使用: %zu bytes\n", initial_used);
    
    // 分配一些对象（没有根引用，应该被回收）
    void* temp_ptrs[20];
    for (int i = 0; i < 20; i++) {
        temp_ptrs[i] = j2me_gc_allocate(gc, 1000 + i * 100, TEST_OBJECT_TYPE_SIMPLE);
    }
    
    size_t after_alloc_used, after_alloc_free;
    j2me_gc_get_heap_info(gc, &after_alloc_used, &after_alloc_free, &total);
    printf("  分配后堆使用: %zu bytes\n", after_alloc_used);
    
    // 清除临时引用（模拟对象不再被引用）
    memset(temp_ptrs, 0, sizeof(temp_ptrs));
    
    // 执行垃圾回收
    j2me_error_t result = j2me_gc_collect(gc);
    if (result != J2ME_SUCCESS) {
        printf("  错误: 垃圾回收执行失败: %d\n", result);
        test_passed = false;
    }
    
    // 检查回收效果
    size_t after_gc_used, after_gc_free;
    j2me_gc_get_heap_info(gc, &after_gc_used, &after_gc_free, &total);
    printf("  GC后堆使用: %zu bytes\n", after_gc_used);
    
    j2me_gc_stats_t final_stats = j2me_gc_get_stats(gc);
    uint64_t collections_performed = final_stats.collections - initial_stats.collections;
    uint64_t bytes_collected = final_stats.bytes_collected - initial_stats.bytes_collected;
    
    printf("  GC执行次数: %llu\n", (unsigned long long)collections_performed);
    printf("  回收字节数: %llu\n", (unsigned long long)bytes_collected);
    
    if (collections_performed > 0) {
        printf("  ✓ 垃圾回收成功执行\n");
    } else {
        printf("  警告: 垃圾回收未执行\n");
    }
    
    print_test_result("垃圾回收测试", test_passed);
}

void test_gc_root_management(j2me_gc_t* gc) {
    print_test_header("根对象管理测试");
    
    bool test_passed = true;
    
    // 分配一个对象
    j2me_object_t* obj = (j2me_object_t*)j2me_gc_allocate(gc, sizeof(j2me_object_t), TEST_OBJECT_TYPE_SIMPLE);
    if (!obj) {
        printf("  错误: 对象分配失败\n");
        print_test_result("根对象管理测试", false);
        return;
    }
    
    printf("  分配对象: %p\n", obj);
    
    // 添加为根对象
    j2me_error_t result = j2me_gc_add_root(gc, &obj, "测试根对象");
    if (result != J2ME_SUCCESS) {
        printf("  错误: 添加根对象失败: %d\n", result);
        test_passed = false;
    } else {
        printf("  ✓ 根对象添加成功\n");
    }
    
    // 获取初始堆使用情况
    size_t initial_used, initial_free, total;
    j2me_gc_get_heap_info(gc, &initial_used, &initial_free, &total);
    
    // 分配更多对象（没有根引用）
    for (int i = 0; i < 10; i++) {
        j2me_gc_allocate(gc, 500, TEST_OBJECT_TYPE_SIMPLE);
    }
    
    // 执行垃圾回收
    result = j2me_gc_collect(gc);
    if (result != J2ME_SUCCESS) {
        printf("  错误: 垃圾回收失败: %d\n", result);
        test_passed = false;
    }
    
    // 检查根对象是否仍然存在
    if (obj != NULL) {
        printf("  ✓ 根对象在GC后仍然存在\n");
    } else {
        printf("  错误: 根对象被错误回收\n");
        test_passed = false;
    }
    
    // 移除根对象
    result = j2me_gc_remove_root(gc, &obj);
    if (result != J2ME_SUCCESS) {
        printf("  错误: 移除根对象失败: %d\n", result);
        test_passed = false;
    } else {
        printf("  ✓ 根对象移除成功\n");
    }
    
    // 再次执行GC，现在对象应该被回收
    obj = NULL; // 清除引用
    result = j2me_gc_collect(gc);
    if (result == J2ME_SUCCESS) {
        printf("  ✓ 移除根对象后GC执行成功\n");
    }
    
    print_test_result("根对象管理测试", test_passed);
}

void test_gc_fragmentation(j2me_gc_t* gc) {
    print_test_header("内存碎片测试");
    
    bool test_passed = true;
    
    // 分配多个不同大小的对象
    void* ptrs[50];
    int sizes[] = {32, 64, 128, 256, 512, 1024};
    int size_count = sizeof(sizes) / sizeof(sizes[0]);
    
    printf("  分配多种大小的对象...\n");
    for (int i = 0; i < 50; i++) {
        int size = sizes[i % size_count];
        ptrs[i] = j2me_gc_allocate(gc, size, TEST_OBJECT_TYPE_SIMPLE);
        if (!ptrs[i]) {
            printf("  警告: 第%d个对象分配失败 (大小: %d)\n", i, size);
        }
    }
    
    // 释放一些对象（模拟随机释放）
    printf("  模拟随机对象释放...\n");
    for (int i = 0; i < 50; i += 3) {
        ptrs[i] = NULL; // 清除引用，使对象可被回收
    }
    
    // 执行垃圾回收
    j2me_error_t result = j2me_gc_collect(gc);
    if (result != J2ME_SUCCESS) {
        printf("  错误: 垃圾回收失败: %d\n", result);
        test_passed = false;
    }
    
    // 尝试分配大对象，测试碎片整理效果
    printf("  测试大对象分配...\n");
    void* large_obj = j2me_gc_allocate(gc, 8192, TEST_OBJECT_TYPE_ARRAY);
    if (large_obj) {
        printf("  ✓ 大对象分配成功: %p (8192 bytes)\n", large_obj);
    } else {
        printf("  警告: 大对象分配失败，可能存在碎片问题\n");
    }
    
    // 打印堆使用情况
    size_t used, free, total;
    j2me_gc_get_heap_info(gc, &used, &free, &total);
    printf("  最终堆使用: %zu/%zu bytes (%.1f%%)\n", 
           used, total, (double)used * 100.0 / total);
    
    print_test_result("内存碎片测试", test_passed);
}

void test_gc_performance(j2me_gc_t* gc) {
    print_test_header("GC性能测试");
    
    bool test_passed = true;
    
    // 获取初始统计信息
    j2me_gc_stats_t initial_stats = j2me_gc_get_stats(gc);
    
    printf("  执行大量分配和回收操作...\n");
    
    // 执行多轮分配和回收
    const int rounds = 10;
    const int objects_per_round = 100;
    
    for (int round = 0; round < rounds; round++) {
        // 分配对象
        void* ptrs[objects_per_round];
        for (int i = 0; i < objects_per_round; i++) {
            ptrs[i] = j2me_gc_allocate(gc, 100 + (i % 500), TEST_OBJECT_TYPE_SIMPLE);
        }
        
        // 保留一些对象，释放其他对象
        for (int i = 0; i < objects_per_round; i += 2) {
            ptrs[i] = NULL; // 清除引用
        }
        
        // 触发垃圾回收
        if (j2me_gc_should_collect(gc)) {
            j2me_gc_collect(gc);
        }
        
        printf("  完成第 %d/%d 轮\n", round + 1, rounds);
    }
    
    // 获取最终统计信息
    j2me_gc_stats_t final_stats = j2me_gc_get_stats(gc);
    
    uint64_t total_collections = final_stats.collections - initial_stats.collections;
    uint64_t total_time = final_stats.total_time_ms - initial_stats.total_time_ms;
    uint64_t max_pause = final_stats.max_pause_time_ms;
    
    printf("\n  性能统计:\n");
    printf("  - 总GC次数: %llu\n", (unsigned long long)total_collections);
    printf("  - 总GC时间: %llu ms\n", (unsigned long long)total_time);
    printf("  - 最大暂停时间: %llu ms\n", (unsigned long long)max_pause);
    
    if (total_collections > 0) {
        uint64_t avg_time = total_time / total_collections;
        printf("  - 平均GC时间: %llu ms\n", (unsigned long long)avg_time);
        
        // 性能检查
        if (max_pause <= 50) { // 最大暂停时间不超过50ms
            printf("  ✓ GC暂停时间在可接受范围内\n");
        } else {
            printf("  警告: GC暂停时间过长\n");
        }
    }
    
    print_test_result("GC性能测试", test_passed);
}

void test_gc_stress(j2me_gc_t* gc) {
    print_test_header("GC压力测试");
    
    bool test_passed = true;
    
    printf("  执行高强度内存分配...\n");
    
    // 高强度分配测试
    const int stress_iterations = 1000;
    int successful_allocations = 0;
    int failed_allocations = 0;
    
    for (int i = 0; i < stress_iterations; i++) {
        // 随机大小分配
        size_t size = 50 + (i % 1000);
        void* ptr = j2me_gc_allocate(gc, size, TEST_OBJECT_TYPE_SIMPLE);
        
        if (ptr) {
            successful_allocations++;
            
            // 随机决定是否立即"释放"（清除引用）
            if (i % 3 == 0) {
                ptr = NULL; // 模拟对象不再被引用
            }
        } else {
            failed_allocations++;
        }
        
        // 定期触发GC
        if (i % 100 == 0) {
            j2me_gc_collect(gc);
            if (i % 200 == 0) {
                printf("  完成 %d/%d 次分配\n", i, stress_iterations);
            }
        }
    }
    
    // 最终垃圾回收
    j2me_gc_collect(gc);
    
    printf("\n  压力测试结果:\n");
    printf("  - 成功分配: %d\n", successful_allocations);
    printf("  - 失败分配: %d\n", failed_allocations);
    printf("  - 成功率: %.1f%%\n", 
           (double)successful_allocations * 100.0 / stress_iterations);
    
    // 检查系统稳定性
    size_t used, free, total;
    j2me_gc_get_heap_info(gc, &used, &free, &total);
    printf("  - 最终堆使用: %zu/%zu bytes (%.1f%%)\n", 
           used, total, (double)used * 100.0 / total);
    
    if (successful_allocations > stress_iterations * 0.8) {
        printf("  ✓ 压力测试通过，系统稳定\n");
    } else {
        printf("  警告: 分配成功率较低\n");
        test_passed = false;
    }
    
    print_test_result("GC压力测试", test_passed);
}

void print_test_header(const char* test_name) {
    printf("\n--- %s ---\n", test_name);
}

void print_test_result(const char* test_name, bool passed) {
    printf("--- %s: %s ---\n", test_name, passed ? "通过" : "失败");
}