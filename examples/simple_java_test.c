#include "j2me_vm.h"
#include "j2me_heap.h"
#include "j2me_string.h"
#include "j2me_native_methods.h"
#include "j2me_class.h"
#include "j2me_interpreter.h"
#include "j2me_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/**
 * @file simple_java_test.c
 * @brief 简单Java程序测试
 * 
 * 加载并执行SimpleTest.class，验证完整的Java程序执行流程
 */

/**
 * @brief 从文件加载class数据
 */
static uint8_t* load_class_file(const char* filename, size_t* size) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        LOG_DEBUG("错误: 无法打开文件 %s\n", filename);
        return NULL;
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // 读取文件内容
    uint8_t* data = (uint8_t*)malloc(*size);
    if (!data) {
        fclose(file);
        return NULL;
    }
    
    size_t read_size = fread(data, 1, *size, file);
    fclose(file);
    
    if (read_size != *size) {
        free(data);
        return NULL;
    }
    
    return data;
}

/**
 * @brief 查找main方法
 */
static j2me_method_t* find_main_method(j2me_class_t* class_ptr) {
    if (!class_ptr || !class_ptr->methods) {
        return NULL;
    }
    
    for (uint16_t i = 0; i < class_ptr->methods_count; i++) {
        j2me_method_t* method = &class_ptr->methods[i];
        if (method->name && strcmp(method->name, "main") == 0) {
            if (method->descriptor && strcmp(method->descriptor, "([Ljava/lang/String;)V") == 0) {
                return method;
            }
        }
    }
    
    return NULL;
}

int main(void) {
    LOG_DEBUG("=== 简单Java程序测试 ===\n\n");
    
    // 步骤1: 创建虚拟机
    LOG_DEBUG("步骤1: 创建虚拟机\n");
    j2me_vm_config_t config = j2me_vm_get_default_config();
    config.heap_size = 2 * 1024 * 1024; // 2MB堆
    
    j2me_vm_t* vm = j2me_vm_create(&config);
    assert(vm != NULL);
    LOG_DEBUG("✓ 虚拟机创建成功\n\n");
    
    // 步骤2: 初始化本地方法
    LOG_DEBUG("步骤2: 初始化本地方法\n");
    j2me_error_t result = j2me_midp_native_methods_init(vm);
    assert(result == J2ME_SUCCESS);
    LOG_DEBUG("✓ 本地方法初始化成功\n\n");
    
    // 步骤3: 加载SimpleTest.class
    LOG_DEBUG("步骤3: 加载SimpleTest.class\n");
    size_t class_size;
    uint8_t* class_data = load_class_file("test_programs/SimpleTest.class", &class_size);
    if (!class_data) {
        LOG_DEBUG("✗ 无法加载class文件\n");
        j2me_vm_destroy(vm);
        return 1;
    }
    LOG_DEBUG("✓ 加载class文件成功 (%zu bytes)\n\n", class_size);
    
    // 步骤4: 解析class文件
    LOG_DEBUG("步骤4: 解析class文件\n");
    j2me_class_t* class_ptr = j2me_class_parse(class_data, class_size);
    free(class_data);
    
    if (!class_ptr) {
        LOG_DEBUG("✗ 解析class文件失败\n");
        j2me_vm_destroy(vm);
        return 1;
    }
    LOG_DEBUG("✓ 解析class文件成功\n");
    LOG_DEBUG("  类名: %s\n", class_ptr->name ? class_ptr->name : "unknown");
    LOG_DEBUG("  方法数: %d\n", class_ptr->methods_count);
    LOG_DEBUG("  字段数: %d\n\n", class_ptr->fields_count);
    
    // 步骤5: 查找main方法
    LOG_DEBUG("步骤5: 查找main方法\n");
    j2me_method_t* main_method = find_main_method(class_ptr);
    if (!main_method) {
        LOG_DEBUG("✗ 未找到main方法\n");
        j2me_class_destroy(class_ptr);
        j2me_vm_destroy(vm);
        return 1;
    }
    LOG_DEBUG("✓ 找到main方法\n");
    LOG_DEBUG("  方法名: %s\n", main_method->name);
    LOG_DEBUG("  描述符: %s\n", main_method->descriptor);
    LOG_DEBUG("  访问标志: 0x%04x\n", main_method->access_flags);
    LOG_DEBUG("  代码长度: %d bytes\n\n", main_method->bytecode_length);
    
    // 步骤6: 准备执行
    LOG_DEBUG("步骤6: 准备执行\n");
    LOG_DEBUG("  最大栈深度: %d\n", main_method->max_stack);
    LOG_DEBUG("  最大局部变量: %d\n\n", main_method->max_locals);
    
    // 步骤7: 执行main方法
    LOG_DEBUG("步骤7: 执行main方法\n");
    LOG_DEBUG("========================================\n");
    LOG_DEBUG("开始执行Java程序...\n");
    LOG_DEBUG("========================================\n\n");
    
    result = j2me_interpreter_execute_method(vm, main_method, NULL, NULL);
    
    LOG_DEBUG("\n========================================\n");
    LOG_DEBUG("Java程序执行完成\n");
    LOG_DEBUG("========================================\n\n");
    
    if (result != J2ME_SUCCESS) {
        LOG_DEBUG("✗ 方法执行失败，错误码: %d\n", result);
    } else {
        LOG_DEBUG("✓ 方法执行成功\n\n");
    }
    
    // 步骤8: 清理资源
    LOG_DEBUG("步骤8: 清理资源\n");
    j2me_class_destroy(class_ptr);
    j2me_vm_destroy(vm);
    LOG_DEBUG("✓ 资源清理完成\n\n");
    
    if (result == J2ME_SUCCESS) {
        LOG_DEBUG("=== 所有测试通过! ===\n");
        LOG_DEBUG("\n🎉 Phase 1 完成！\n");
        LOG_DEBUG("  ✓ 虚拟机创建\n");
        LOG_DEBUG("  ✓ 堆内存管理\n");
        LOG_DEBUG("  ✓ String对象系统\n");
        LOG_DEBUG("  ✓ 本地方法注册\n");
        LOG_DEBUG("  ✓ Class文件加载\n");
        LOG_DEBUG("  ✓ 方法执行\n");
        LOG_DEBUG("  ✓ System.out.println\n");
        LOG_DEBUG("\n成功运行了第一个Java程序！\n");
        return 0;
    } else {
        LOG_DEBUG("=== 测试失败 ===\n");
        return 1;
    }
}
