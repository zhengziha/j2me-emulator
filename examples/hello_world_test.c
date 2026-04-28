#include "j2me_vm.h"
#include "j2me_heap.h"
#include "j2me_string.h"
#include "j2me_native_methods.h"
#include "j2me_interpreter.h"
#include "j2me_log.h"
#include <stdio.h>
#include <assert.h>

/**
 * @file hello_world_test.c
 * @brief Hello World测试程序
 * 
 * 测试VM集成、堆系统和System.out.println
 */

int main(void) {
    LOG_DEBUG("=== Hello World测试 ===\n\n");
    
    // 测试1: 创建虚拟机
    LOG_DEBUG("测试1: 创建虚拟机\n");
    j2me_vm_config_t config = j2me_vm_get_default_config();
    config.heap_size = 2 * 1024 * 1024; // 2MB堆
    
    j2me_vm_t* vm = j2me_vm_create(&config);
    assert(vm != NULL);
    LOG_DEBUG("✓ 虚拟机创建成功\n\n");
    
    // 测试2: 检查堆是否已创建
    LOG_DEBUG("测试2: 检查堆\n");
    assert(vm->heap != NULL);
    j2me_heap_print_stats(vm->heap);
    LOG_DEBUG("✓ 堆已创建\n\n");
    
    // 测试3: 在堆上创建String对象
    LOG_DEBUG("测试3: 创建String对象\n");
    j2me_ref_t str1 = j2me_string_create(vm->heap, "Hello J2ME!");
    assert(str1 != J2ME_NULL_REF);
    j2me_string_print(vm->heap, str1);
    LOG_DEBUG("✓ String对象创建成功\n\n");
    
    // 测试4: 创建更多String对象
    LOG_DEBUG("测试4: 创建多个String对象\n");
    j2me_ref_t str2 = j2me_string_create(vm->heap, "Welcome to the new VM!");
    j2me_ref_t str3 = j2me_string_create(vm->heap, "This is a test.");
    assert(str2 != J2ME_NULL_REF);
    assert(str3 != J2ME_NULL_REF);
    LOG_DEBUG("✓ 多个String对象创建成功\n\n");
    
    // 测试5: 初始化本地方法
    LOG_DEBUG("测试5: 初始化本地方法\n");
    j2me_error_t result = j2me_midp_native_methods_init(vm);
    assert(result == J2ME_SUCCESS);
    LOG_DEBUG("✓ 本地方法初始化成功\n\n");
    
    // 测试6: 模拟System.out.println调用
    LOG_DEBUG("测试6: 测试System.out.println\n");
    
    // 创建栈帧
    j2me_stack_frame_t* frame = j2me_stack_frame_create(10, 5);
    assert(frame != NULL);
    
    // 将String引用压入栈
    result = j2me_operand_stack_push(&frame->operand_stack, str1);
    assert(result == J2ME_SUCCESS);
    
    // 调用System.out.println
    LOG_DEBUG("  调用System.out.println(\"Hello J2ME!\"):\n");
    LOG_DEBUG("  ----------------------------------------\n");
    result = java_system_out_println(vm, frame, NULL);
    LOG_DEBUG("  ----------------------------------------\n");
    assert(result == J2ME_SUCCESS);
    LOG_DEBUG("✓ System.out.println调用成功\n\n");
    
    // 测试7: 测试更多字符串
    LOG_DEBUG("测试7: 打印更多字符串\n");
    
    // 重置栈帧
    frame->operand_stack.top = 0;
    
    // 打印第二个字符串
    result = j2me_operand_stack_push(&frame->operand_stack, str2);
    assert(result == J2ME_SUCCESS);
    LOG_DEBUG("  调用System.out.println(\"Welcome to the new VM!\"):\n");
    LOG_DEBUG("  ----------------------------------------\n");
    result = java_system_out_println(vm, frame, NULL);
    LOG_DEBUG("  ----------------------------------------\n");
    assert(result == J2ME_SUCCESS);
    
    // 重置栈帧
    frame->operand_stack.top = 0;
    
    // 打印第三个字符串
    result = j2me_operand_stack_push(&frame->operand_stack, str3);
    assert(result == J2ME_SUCCESS);
    LOG_DEBUG("  调用System.out.println(\"This is a test.\"):\n");
    LOG_DEBUG("  ----------------------------------------\n");
    result = java_system_out_println(vm, frame, NULL);
    LOG_DEBUG("  ----------------------------------------\n");
    assert(result == J2ME_SUCCESS);
    LOG_DEBUG("✓ 多个字符串打印成功\n\n");
    
    // 测试8: 堆统计
    LOG_DEBUG("测试8: 最终堆统计\n");
    j2me_heap_print_stats(vm->heap);
    
    size_t used, total, objects;
    j2me_heap_get_stats(vm->heap, &used, &total, &objects);
    LOG_DEBUG("  已使用: %zu bytes\n", used);
    LOG_DEBUG("  总大小: %zu bytes\n", total);
    LOG_DEBUG("  对象数: %zu\n", objects);
    LOG_DEBUG("✓ 堆统计成功\n\n");
    
    // 清理
    LOG_DEBUG("清理资源...\n");
    j2me_stack_frame_destroy(frame);
    j2me_vm_destroy(vm);
    
    LOG_DEBUG("\n=== 所有测试通过! ===\n");
    LOG_DEBUG("\n🎉 Phase 1 核心功能验证成功！\n");
    LOG_DEBUG("  ✓ 虚拟机创建\n");
    LOG_DEBUG("  ✓ 堆内存管理\n");
    LOG_DEBUG("  ✓ String对象系统\n");
    LOG_DEBUG("  ✓ 本地方法注册\n");
    LOG_DEBUG("  ✓ System.out.println\n");
    LOG_DEBUG("\n下一步: 创建SimpleTest.java并运行真实的Java程序！\n");
    
    return 0;
}
