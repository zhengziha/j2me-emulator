/**
 * 简单解释器测试程序
 * 用于测试基本的字节码执行功能
 */

#include "j2me_vm.h"
#include "j2me_class.h"
#include "j2me_interpreter.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    printf("=== 简单解释器测试 ===\n\n");
    
    // 创建虚拟机
    j2me_vm_config_t config = j2me_vm_get_default_config();
    config.heap_size = 1024 * 1024;  // 1MB堆
    
    j2me_vm_t* vm = j2me_vm_create(&config);
    if (!vm) {
        printf("❌ 虚拟机创建失败\n");
        return 1;
    }
    
    printf("✅ 虚拟机创建成功\n");
    
    // 初始化虚拟机
    j2me_error_t result = j2me_vm_initialize(vm);
    if (result != J2ME_SUCCESS) {
        printf("❌ 虚拟机初始化失败: %d\n", result);
        j2me_vm_destroy(vm);
        return 1;
    }
    
    printf("✅ 虚拟机初始化成功\n\n");
    
    // 读取class文件
    const char* class_file = "test_simple/classes/SimpleTest.class";
    FILE* f = fopen(class_file, "rb");
    if (!f) {
        printf("❌ 无法打开class文件: %s\n", class_file);
        j2me_vm_destroy(vm);
        return 1;
    }
    
    // 获取文件大小
    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    // 读取文件内容
    uint8_t* class_data = (uint8_t*)malloc(file_size);
    if (!class_data) {
        printf("❌ 内存分配失败\n");
        fclose(f);
        j2me_vm_destroy(vm);
        return 1;
    }
    
    size_t read_size = fread(class_data, 1, file_size, f);
    fclose(f);
    
    if (read_size != file_size) {
        printf("❌ 文件读取失败\n");
        free(class_data);
        j2me_vm_destroy(vm);
        return 1;
    }
    
    printf("✅ 读取class文件成功: %zu bytes\n\n", file_size);
    
    // 解析class文件
    j2me_class_t* test_class = j2me_class_parse(class_data, file_size);
    free(class_data);
    
    if (!test_class) {
        printf("❌ class文件解析失败\n");
        j2me_vm_destroy(vm);
        return 1;
    }
    
    printf("✅ class文件解析成功\n");
    printf("   类名: %s\n", test_class->name ? test_class->name : "unknown");
    printf("   父类: %s\n", test_class->super_name ? test_class->super_name : "none");
    printf("   方法数: %d\n", test_class->methods_count);
    printf("   字段数: %d\n\n", test_class->fields_count);
    
    // 设置类加载器
    test_class->loader = vm->class_loader;
    test_class->state = CLASS_LOADED;
    
    // 添加到已加载类列表
    j2me_class_loader_t* loader = (j2me_class_loader_t*)vm->class_loader;
    test_class->next = loader->loaded_classes;
    loader->loaded_classes = test_class;
    
    // 链接类
    result = j2me_class_link(test_class);
    if (result != J2ME_SUCCESS) {
        printf("❌ 类链接失败: %d\n", result);
        j2me_class_destroy(test_class);
        j2me_vm_destroy(vm);
        return 1;
    }
    
    printf("✅ 类链接成功\n");
    
    // 初始化类
    result = j2me_class_initialize(test_class);
    if (result != J2ME_SUCCESS) {
        printf("❌ 类初始化失败: %d\n", result);
        j2me_class_destroy(test_class);
        j2me_vm_destroy(vm);
        return 1;
    }
    
    printf("✅ 类初始化成功\n\n");
    
    // 测试1: testArithmetic
    printf("=== 测试1: testArithmetic() ===\n");
    j2me_method_t* method1 = j2me_class_find_method(test_class, "testArithmetic", "()I");
    if (method1) {
        printf("找到方法: testArithmetic\n");
        result = j2me_interpreter_execute_method(vm, method1, NULL, NULL);
        if (result == J2ME_SUCCESS && vm->last_method_has_return_value) {
            printf("✅ 执行成功，返回值: %d (期望: 10)\n\n", vm->last_method_return_value);
        } else {
            printf("❌ 执行失败: %d\n\n", result);
        }
    } else {
        printf("❌ 未找到方法\n\n");
    }
    
    // 测试2: testCondition
    printf("=== 测试2: testCondition(5) ===\n");
    j2me_method_t* method2 = j2me_class_find_method(test_class, "testCondition", "(I)I");
    if (method2) {
        printf("找到方法: testCondition\n");
        j2me_int args[] = {5};
        result = j2me_interpreter_execute_method(vm, method2, NULL, args);
        if (result == J2ME_SUCCESS && vm->last_method_has_return_value) {
            printf("✅ 执行成功，返回值: %d (期望: 1)\n\n", vm->last_method_return_value);
        } else {
            printf("❌ 执行失败: %d\n\n", result);
        }
    } else {
        printf("❌ 未找到方法\n\n");
    }
    
    // 测试3: testLoop
    printf("=== 测试3: testLoop() ===\n");
    j2me_method_t* method3 = j2me_class_find_method(test_class, "testLoop", "()I");
    if (method3) {
        printf("找到方法: testLoop\n");
        result = j2me_interpreter_execute_method(vm, method3, NULL, NULL);
        if (result == J2ME_SUCCESS && vm->last_method_has_return_value) {
            printf("✅ 执行成功，返回值: %d (期望: 55)\n\n", vm->last_method_return_value);
        } else {
            printf("❌ 执行失败: %d\n\n", result);
        }
    } else {
        printf("❌ 未找到方法\n\n");
    }
    
    // 测试4: main
    printf("=== 测试4: main() ===\n");
    j2me_method_t* method4 = j2me_class_find_method(test_class, "main", "()V");
    if (method4) {
        printf("找到方法: main\n");
        result = j2me_interpreter_execute_method(vm, method4, NULL, NULL);
        if (result == J2ME_SUCCESS) {
            printf("✅ 执行成功\n\n");
        } else {
            printf("❌ 执行失败: %d\n\n", result);
        }
    } else {
        printf("❌ 未找到方法\n\n");
    }
    
    // 清理
    j2me_class_destroy(test_class);
    j2me_vm_destroy(vm);
    
    printf("=== 测试完成 ===\n");
    return 0;
}
