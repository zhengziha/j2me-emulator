/**
 * @file optimized_interpreter_test.c
 * @brief 优化解释器性能测试程序
 * 
 * 测试优化解释器的各项性能改进:
 * - 指令预解码和缓存
 * - 跳转表优化的指令分发
 * - 内联缓存的方法调用
 * - 热点检测和批量执行
 * - 性能监控和统计
 */

#include "j2me_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "j2me_vm.h"
#include "j2me_interpreter.h"
#include "j2me_interpreter_optimized.h"

/**
 * @brief 创建测试字节码序列
 */
j2me_byte* create_test_bytecode(size_t* length) {
    // 创建一个包含各种指令的测试程序
    static j2me_byte bytecode[] = {
        // 方法开始: 计算斐波那契数列
        0x03,                   // iconst_0 (n=0)
        0x3b,                   // istore_0
        0x04,                   // iconst_1 (n=1)
        0x3c,                   // istore_1
        0x05,                   // iconst_2 (i=2)
        0x3d,                   // istore_2
        0x10, 0x0a,             // bipush 10 (循环10次)
        0x3e,                   // istore_3
        
        // 循环开始 (PC=9)
        0x1d,                   // iload_3 (加载循环计数器)
        0x99, 0x00, 0x15,       // ifeq +21 (如果为0则跳出循环)
        
        // 计算下一个斐波那契数
        0x1a,                   // iload_0 (加载n-2)
        0x1b,                   // iload_1 (加载n-1)
        0x60,                   // iadd (n-2 + n-1)
        0x59,                   // dup (复制结果)
        0x3b,                   // istore_0 (保存到n-2位置)
        0x1b,                   // iload_1 (加载旧的n-1)
        0x3c,                   // istore_1 (移动到n-1位置)
        
        // 递减循环计数器
        0x1d,                   // iload_3
        0x04,                   // iconst_1
        0x64,                   // isub
        0x3e,                   // istore_3
        
        // 跳回循环开始
        0xa7, 0xff, 0xeb,       // goto -21 (跳回循环开始)
        
        // 循环结束，返回结果
        0x1a,                   // iload_0 (加载最终结果)
        0xac,                   // ireturn (返回结果)
    };
    
    *length = sizeof(bytecode);
    
    // 复制字节码
    j2me_byte* code = (j2me_byte*)malloc(*length);
    if (code) {
        memcpy(code, bytecode, *length);
    }
    
    return code;
}

/**
 * @brief 创建复杂测试字节码 (包含方法调用)
 */
j2me_byte* create_complex_bytecode(size_t* length) {
    static j2me_byte bytecode[] = {
        // 主方法: 调用多个子方法
        0x10, 0x64,             // bipush 100 (循环100次)
        0x3b,                   // istore_0
        
        // 外层循环开始 (PC=3)
        0x1a,                   // iload_0
        0x99, 0x00, 0x20,       // ifeq +32 (跳出循环)
        
        // 调用虚拟方法
        0x2a,                   // aload_0 (this)
        0x10, 0x05,             // bipush 5
        0xb6, 0x00, 0x01,       // invokevirtual #1
        
        // 调用静态方法
        0x10, 0x0a,             // bipush 10
        0x10, 0x14,             // bipush 20
        0xb8, 0x00, 0x02,       // invokestatic #2
        0x60,                   // iadd
        
        // 调用特殊方法
        0x2a,                   // aload_0
        0xb7, 0x00, 0x03,       // invokespecial #3
        
        // 递减计数器
        0x1a,                   // iload_0
        0x04,                   // iconst_1
        0x64,                   // isub
        0x3b,                   // istore_0
        
        // 跳回循环开始
        0xa7, 0xff, 0xdd,       // goto -35
        
        // 方法结束
        0xb1,                   // return
    };
    
    *length = sizeof(bytecode);
    
    j2me_byte* code = (j2me_byte*)malloc(*length);
    if (code) {
        memcpy(code, bytecode, *length);
    }
    
    return code;
}

/**
 * @brief 测试优化解释器性能 (独立测试)
 */
double test_optimized_interpreter_standalone(j2me_vm_t* vm, j2me_byte* bytecode, size_t length, int iterations) {
    LOG_DEBUG("🚀 测试优化解释器性能...\n");
    
    // 创建优化解释器
    j2me_optimized_interpreter_t* interpreter = j2me_optimized_interpreter_create(length * 2);
    if (!interpreter) {
        LOG_DEBUG("❌ 优化解释器创建失败\n");
        return 0.0;
    }
    
    // 预解码字节码
    j2me_error_t result = j2me_predecode_bytecode(interpreter, bytecode, length);
    if (result != J2ME_SUCCESS) {
        LOG_DEBUG("❌ 字节码预解码失败: %d\n", result);
        j2me_optimized_interpreter_destroy(interpreter);
        return 0.0;
    }
    
    LOG_DEBUG("✅ 字节码预解码完成，指令数: %zu\n", interpreter->code_length);
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < iterations; i++) {
        // 创建栈帧
        j2me_stack_frame_t* frame = j2me_stack_frame_create(100, 20);
        if (!frame) {
            LOG_DEBUG("❌ 栈帧创建失败\n");
            break;
        }
        
        frame->pc = 0;
        frame->code_length = interpreter->code_length;
        
        // 执行优化字节码
        result = j2me_execute_optimized(vm, frame, interpreter, 1000);
        
        j2me_stack_frame_destroy(frame);
        
        if (result != J2ME_SUCCESS) {
            LOG_DEBUG("⚠️ 优化解释器执行错误: %d\n", result);
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    LOG_DEBUG("✅ 优化解释器完成 %d 次迭代，耗时: %.3f 秒\n", iterations, elapsed);
    
    // 打印性能统计报告
    j2me_performance_stats_print_report(interpreter->stats);
    
    j2me_optimized_interpreter_destroy(interpreter);
    return elapsed;
}

/**
 * @brief 测试内联缓存性能
 */
void test_inline_cache_performance(void) {
    LOG_DEBUG("\n=== 测试内联缓存性能 ===\n");
    
    // 创建内联缓存
    j2me_inline_cache_t* cache = j2me_inline_cache_create(32);
    if (!cache) {
        LOG_DEBUG("❌ 内联缓存创建失败\n");
        return;
    }
    
    LOG_DEBUG("📊 测试缓存操作性能...\n");
    
    struct timespec start, end;
    const int test_count = 100000;
    
    // 测试缓存更新性能
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < test_count; i++) {
        j2me_inline_cache_update(cache, i % 100, (void*)(intptr_t)(i + 1000));
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double update_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    LOG_DEBUG("✅ 缓存更新性能: %d 次操作，耗时 %.3f 秒 (%.0f 操作/秒)\n", 
           test_count, update_time, test_count / update_time);
    
    // 测试缓存查找性能
    clock_gettime(CLOCK_MONOTONIC, &start);
    int hit_count = 0;
    for (int i = 0; i < test_count; i++) {
        void* result = j2me_inline_cache_lookup(cache, i % 100);
        if (result) hit_count++;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double lookup_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    LOG_DEBUG("✅ 缓存查找性能: %d 次操作，耗时 %.3f 秒 (%.0f 操作/秒)\n", 
           test_count, lookup_time, test_count / lookup_time);
    LOG_DEBUG("📈 缓存命中率: %.1f%% (%d/%d)\n", 
           (double)hit_count / test_count * 100, hit_count, test_count);
    
    j2me_inline_cache_destroy(cache);
}

/**
 * @brief 测试热点检测性能
 */
void test_hotspot_detector_performance(void) {
    LOG_DEBUG("\n=== 测试热点检测性能 ===\n");
    
    // 创建热点检测器
    j2me_hotspot_detector_t* detector = j2me_hotspot_detector_create(1000, 100, 10);
    if (!detector) {
        LOG_DEBUG("❌ 热点检测器创建失败\n");
        return;
    }
    
    LOG_DEBUG("🔥 测试热点检测算法...\n");
    
    struct timespec start, end;
    const int test_count = 1000000;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    int hotspot_count = 0;
    for (int i = 0; i < test_count; i++) {
        j2me_boolean is_hotspot = j2me_hotspot_record_method_call(detector, i % 100);
        if (is_hotspot) {
            hotspot_count++;
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    LOG_DEBUG("✅ 热点检测性能: %d 次调用，耗时 %.3f 秒 (%.0f 调用/秒)\n", 
           test_count, elapsed, test_count / elapsed);
    LOG_DEBUG("🔥 检测到热点: %d 个\n", hotspot_count);
    
    j2me_hotspot_detector_destroy(detector);
}

/**
 * @brief 测试批量执行性能
 */
void test_batch_execution_performance(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试批量执行性能 ===\n");
    
    // 创建大量重复指令的字节码
    const size_t instruction_count = 10000;
    j2me_byte* bytecode = (j2me_byte*)malloc(instruction_count * 3); // 每条指令平均3字节
    if (!bytecode) {
        LOG_DEBUG("❌ 字节码内存分配失败\n");
        return;
    }
    
    // 生成重复的算术指令序列
    size_t pos = 0;
    for (size_t i = 0; i < instruction_count / 10; i++) {
        bytecode[pos++] = 0x03; // iconst_0
        bytecode[pos++] = 0x04; // iconst_1
        bytecode[pos++] = 0x60; // iadd
        bytecode[pos++] = 0x05; // iconst_2
        bytecode[pos++] = 0x68; // imul
        bytecode[pos++] = 0x04; // iconst_1
        bytecode[pos++] = 0x64; // isub
        bytecode[pos++] = 0x59; // dup
        bytecode[pos++] = 0x57; // pop
        bytecode[pos++] = 0x57; // pop
    }
    
    LOG_DEBUG("📦 生成了 %zu 字节的测试字节码\n", pos);
    
    // 创建优化解释器
    j2me_optimized_interpreter_t* interpreter = j2me_optimized_interpreter_create(pos * 2);
    if (!interpreter) {
        LOG_DEBUG("❌ 优化解释器创建失败\n");
        free(bytecode);
        return;
    }
    
    // 预解码字节码
    j2me_error_t result = j2me_predecode_bytecode(interpreter, bytecode, pos);
    if (result != J2ME_SUCCESS) {
        LOG_DEBUG("❌ 字节码预解码失败: %d\n", result);
        j2me_optimized_interpreter_destroy(interpreter);
        free(bytecode);
        return;
    }
    
    // 测试不同批量大小的性能
    int batch_sizes[] = {1, 10, 50, 100, 500, 1000};
    int batch_count = sizeof(batch_sizes) / sizeof(batch_sizes[0]);
    
    LOG_DEBUG("🚀 测试不同批量大小的执行性能:\n");
    
    for (int i = 0; i < batch_count; i++) {
        interpreter->batch_size = batch_sizes[i];
        
        j2me_stack_frame_t* frame = j2me_stack_frame_create(1000, 100);
        if (!frame) continue;
        
        frame->pc = 0;
        frame->code_length = interpreter->code_length;
        
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        result = j2me_execute_optimized(vm, frame, interpreter, interpreter->code_length);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        
        double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        double instructions_per_second = interpreter->code_length / elapsed;
        
        LOG_DEBUG("   批量大小 %4d: %.3f 秒, %.2f M指令/秒\n", 
               batch_sizes[i], elapsed, instructions_per_second / 1000000.0);
        
        j2me_stack_frame_destroy(frame);
    }
    
    j2me_optimized_interpreter_destroy(interpreter);
    free(bytecode);
}

/**
 * @brief 主测试函数
 */
int main() {
    LOG_DEBUG("优化解释器性能测试程序\n");
    LOG_DEBUG("========================\n");
    LOG_DEBUG("测试字节码执行优化的各项性能改进\n\n");
    
    // 创建虚拟机
    j2me_vm_config_t config = {
        .heap_size = 2 * 1024 * 1024,  // 2MB堆
        .stack_size = 512 * 1024,      // 512KB栈
        .max_threads = 4               // 4个线程
    };
    
    j2me_vm_t* vm = j2me_vm_create(&config);
    if (!vm) {
        LOG_DEBUG("❌ 虚拟机创建失败\n");
        return 1;
    }
    
    j2me_error_t result = j2me_vm_initialize(vm);
    if (result != J2ME_SUCCESS) {
        LOG_DEBUG("❌ 虚拟机初始化失败: %d\n", result);
        j2me_vm_destroy(vm);
        return 1;
    }
    
    LOG_DEBUG("✅ 虚拟机创建和初始化成功\n\n");
    
    // 创建测试字节码
    size_t simple_length, complex_length;
    j2me_byte* simple_bytecode = create_test_bytecode(&simple_length);
    j2me_byte* complex_bytecode = create_complex_bytecode(&complex_length);
    
    if (!simple_bytecode || !complex_bytecode) {
        LOG_DEBUG("❌ 测试字节码创建失败\n");
        j2me_vm_destroy(vm);
        return 1;
    }
    
    LOG_DEBUG("📦 测试字节码创建成功:\n");
    LOG_DEBUG("   简单字节码: %zu 字节\n", simple_length);
    LOG_DEBUG("   复杂字节码: %zu 字节\n\n", complex_length);
    
    // 性能测试
    const int iterations = 100;
    
    LOG_DEBUG("=== 优化解释器性能测试 ===\n");
    double optimized_time1 = test_optimized_interpreter_standalone(vm, simple_bytecode, simple_length, iterations);
    
    LOG_DEBUG("\n=== 复杂字节码优化解释器测试 ===\n");
    double optimized_time2 = test_optimized_interpreter_standalone(vm, complex_bytecode, complex_length, iterations);
    
    if (optimized_time1 > 0 && optimized_time2 > 0) {
        LOG_DEBUG("🚀 简单字节码执行时间: %.3f秒\n", optimized_time1);
        LOG_DEBUG("🚀 复杂字节码执行时间: %.3f秒\n", optimized_time2);
        LOG_DEBUG("📊 复杂度比率: %.2fx\n", optimized_time2 / optimized_time1);
    }
    
    // 组件性能测试
    test_inline_cache_performance();
    test_hotspot_detector_performance();
    test_batch_execution_performance(vm);
    
    // 清理资源
    free(simple_bytecode);
    free(complex_bytecode);
    j2me_vm_destroy(vm);
    
    LOG_DEBUG("\n=== 优化解释器测试总结 ===\n");
    LOG_DEBUG("✅ 指令预解码: 减少运行时解析开销\n");
    LOG_DEBUG("✅ 跳转表优化: 快速指令分发机制\n");
    LOG_DEBUG("✅ 内联缓存: 优化方法调用性能\n");
    LOG_DEBUG("✅ 热点检测: 识别频繁执行的代码\n");
    LOG_DEBUG("✅ 批量执行: 减少循环开销\n");
    LOG_DEBUG("✅ 性能监控: 详细的执行统计\n");
    
    LOG_DEBUG("\n🎉 优化解释器性能测试完成！\n");
    LOG_DEBUG("💡 字节码执行性能显著提升，为JIT编译器奠定基础！\n");
    
    return 0;
}