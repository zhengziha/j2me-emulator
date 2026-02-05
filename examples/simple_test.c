#include "../include/j2me_vm.h"
#include "../include/j2me_interpreter.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @file simple_test.c
 * @brief 简单的J2ME虚拟机测试程序
 * 
 * 演示如何创建虚拟机并执行简单的字节码指令
 */

/**
 * @brief 创建测试字节码
 * @return 字节码数组指针
 */
static uint8_t* create_test_bytecode(void) {
    // 简单的字节码程序：计算 2 + 3 = 5
    static uint8_t bytecode[] = {
        0x05,  // iconst_2 (将常量2压入栈)
        0x06,  // iconst_3 (将常量3压入栈)
        0x60,  // iadd     (整数加法)
        0x3b,  // istore_0 (存储到局部变量0)
        0x1a,  // iload_0  (加载局部变量0)
        0xb1   // return   (返回)
    };
    return bytecode;
}

/**
 * @brief 测试虚拟机基本功能
 */
void test_vm_basic(void) {
    printf("=== 测试虚拟机基本功能 ===\n");
    
    // 创建虚拟机
    j2me_vm_config_t config = j2me_vm_get_default_config();
    config.heap_size = 512 * 1024; // 512KB堆
    
    j2me_vm_t* vm = j2me_vm_create(&config);
    if (!vm) {
        printf("错误: 虚拟机创建失败\n");
        return;
    }
    
    // 初始化虚拟机
    j2me_error_t result = j2me_vm_initialize(vm);
    if (result != J2ME_SUCCESS) {
        printf("错误: 虚拟机初始化失败 (错误码: %d)\n", result);
        j2me_vm_destroy(vm);
        return;
    }
    
    printf("✓ 虚拟机创建和初始化成功\n");
    
    // 清理
    j2me_vm_destroy(vm);
    printf("✓ 虚拟机销毁成功\n");
}

/**
 * @brief 测试解释器功能
 */
void test_interpreter(void) {
    printf("\n=== 测试解释器功能 ===\n");
    
    // 创建栈帧
    j2me_stack_frame_t* frame = j2me_stack_frame_create(10, 5);
    if (!frame) {
        printf("错误: 栈帧创建失败\n");
        return;
    }
    
    // 设置测试字节码
    frame->bytecode = create_test_bytecode();
    frame->pc = 0;
    
    printf("✓ 栈帧创建成功\n");
    
    // 测试栈操作
    j2me_error_t result;
    
    // 压入测试值
    result = j2me_operand_stack_push(&frame->operand_stack, 42);
    if (result != J2ME_SUCCESS) {
        printf("错误: 栈压入操作失败\n");
        j2me_stack_frame_destroy(frame);
        return;
    }
    
    // 弹出测试值
    j2me_int value;
    result = j2me_operand_stack_pop(&frame->operand_stack, &value);
    if (result != J2ME_SUCCESS || value != 42) {
        printf("错误: 栈弹出操作失败 (期望: 42, 实际: %d)\n", value);
        j2me_stack_frame_destroy(frame);
        return;
    }
    
    printf("✓ 栈操作测试成功 (值: %d)\n", value);
    
    // 测试局部变量
    frame->local_vars.variables[0] = 100;
    if (frame->local_vars.variables[0] != 100) {
        printf("错误: 局部变量操作失败\n");
        j2me_stack_frame_destroy(frame);
        return;
    }
    
    printf("✓ 局部变量操作成功 (值: %d)\n", frame->local_vars.variables[0]);
    
    // 清理
    j2me_stack_frame_destroy(frame);
    printf("✓ 栈帧销毁成功\n");
}

/**
 * @brief 测试字节码执行
 */
void test_bytecode_execution(void) {
    printf("\n=== 测试字节码执行 ===\n");
    
    // 创建虚拟机
    j2me_vm_config_t config = j2me_vm_get_default_config();
    j2me_vm_t* vm = j2me_vm_create(&config);
    if (!vm) {
        printf("错误: 虚拟机创建失败\n");
        return;
    }
    
    j2me_vm_initialize(vm);
    
    // 创建线程和栈帧
    j2me_thread_t thread = {0};
    thread.current_frame = j2me_stack_frame_create(10, 5);
    thread.is_running = true;
    
    if (!thread.current_frame) {
        printf("错误: 栈帧创建失败\n");
        j2me_vm_destroy(vm);
        return;
    }
    
    // 设置字节码
    thread.current_frame->bytecode = create_test_bytecode();
    thread.current_frame->pc = 0;
    
    printf("开始执行字节码程序...\n");
    
    // 执行字节码 (最多10条指令)
    j2me_error_t result = j2me_interpreter_execute_batch(vm, &thread, 10);
    
    if (result == J2ME_SUCCESS) {
        printf("✓ 字节码执行成功\n");
        
        // 检查结果 (应该在局部变量0中存储5)
        if (thread.current_frame->local_vars.variables[0] == 5) {
            printf("✓ 计算结果正确: 2 + 3 = %d\n", 
                   thread.current_frame->local_vars.variables[0]);
        } else {
            printf("⚠ 计算结果异常: 期望5, 实际%d\n", 
                   thread.current_frame->local_vars.variables[0]);
        }
    } else {
        printf("错误: 字节码执行失败 (错误码: %d)\n", result);
    }
    
    // 清理
    j2me_stack_frame_destroy(thread.current_frame);
    j2me_vm_destroy(vm);
}

/**
 * @brief 性能基准测试
 */
void test_performance(void) {
    printf("\n=== 性能基准测试 ===\n");
    
    j2me_vm_config_t config = j2me_vm_get_default_config();
    j2me_vm_t* vm = j2me_vm_create(&config);
    if (!vm) {
        printf("错误: 虚拟机创建失败\n");
        return;
    }
    
    j2me_vm_initialize(vm);
    
    // 创建测试线程
    j2me_thread_t thread = {0};
    thread.current_frame = j2me_stack_frame_create(100, 10);
    thread.is_running = true;
    
    if (!thread.current_frame) {
        printf("错误: 栈帧创建失败\n");
        j2me_vm_destroy(vm);
        return;
    }
    
    // 创建循环字节码 (简单的加法循环)
    static uint8_t loop_bytecode[] = {
        0x03,  // iconst_0
        0x3b,  // istore_0  (计数器 = 0)
        0x04,  // iconst_1
        0x3c,  // istore_1  (增量 = 1)
        // 循环开始
        0x1a,  // iload_0   (加载计数器)
        0x1b,  // iload_1   (加载增量)
        0x60,  // iadd      (加法)
        0x3b,  // istore_0  (存储计数器)
        0xb1   // return
    };
    
    thread.current_frame->bytecode = loop_bytecode;
    thread.current_frame->pc = 0;
    
    printf("执行性能测试 (1000条指令)...\n");
    
    // 执行大量指令
    uint32_t instructions = 1000;
    j2me_error_t result = j2me_interpreter_execute_batch(vm, &thread, instructions);
    
    if (result == J2ME_SUCCESS) {
        printf("✓ 性能测试完成，执行了 %u 条指令\n", instructions);
    } else {
        printf("错误: 性能测试失败 (错误码: %d)\n", result);
    }
    
    // 清理
    j2me_stack_frame_destroy(thread.current_frame);
    j2me_vm_destroy(vm);
}

int main(void) {
    printf("J2ME模拟器测试程序\n");
    printf("==================\n");
    
    // 运行所有测试
    test_vm_basic();
    test_interpreter();
    test_bytecode_execution();
    test_performance();
    
    printf("\n=== 测试完成 ===\n");
    return 0;
}