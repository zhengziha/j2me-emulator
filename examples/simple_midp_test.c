#include "../include/j2me_vm.h"
#include "../include/j2me_object.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @file simple_midp_test.c
 * @brief 简化的MIDP测试程序
 * 
 * 只测试对象系统，避免SDL相关的复杂性
 */

void test_object_system_only(void) {
    printf("\n=== 测试对象系统 ===\n");
    
    // 创建虚拟机
    j2me_vm_config_t config = j2me_vm_get_default_config();
    j2me_vm_t* vm = j2me_vm_create(&config);
    if (!vm) {
        printf("错误: 虚拟机创建失败\n");
        return;
    }
    
    j2me_vm_initialize(vm);
    
    // 加载测试类
    j2me_class_loader_t* loader = (j2me_class_loader_t*)vm->class_loader;
    j2me_class_t* hello_class = j2me_class_loader_load_class(loader, "Hello");
    
    if (hello_class) {
        printf("✓ 类加载成功: %s\n", hello_class->name);
        
        // 创建对象
        j2me_object_t* obj = j2me_object_create(vm, hello_class);
        if (obj) {
            printf("✓ 对象创建成功\n");
            
            // 测试类型检查
            bool is_instance = j2me_object_instanceof(obj, hello_class);
            printf("✓ instanceof检查: %s\n", is_instance ? "通过" : "失败");
            
            // 测试类型转换
            bool can_cast = j2me_object_checkcast(obj, hello_class);
            printf("✓ checkcast检查: %s\n", can_cast ? "通过" : "失败");
        } else {
            printf("⚠ 对象创建失败\n");
        }
    } else {
        printf("⚠ 类加载失败\n");
    }
    
    // 创建数组
    j2me_array_t* array = j2me_array_create(vm, ARRAY_TYPE_INT, 5);
    if (array) {
        printf("✓ 数组创建成功，长度: %d\n", j2me_array_get_length(array));
        
        // 测试数组操作
        for (int i = 0; i < 5; i++) {
            j2me_array_set_int(array, i, i * 10);
        }
        
        printf("✓ 数组元素: ");
        for (int i = 0; i < 5; i++) {
            printf("[%d]=%d ", i, j2me_array_get_int(array, i));
        }
        printf("\n");
    } else {
        printf("⚠ 数组创建失败\n");
    }
    
    // 创建字符串
    j2me_string_t* str = j2me_string_create_from_cstr(vm, "Hello World!");
    if (str) {
        printf("✓ 字符串创建成功，长度: %d\n", j2me_string_get_length(str));
        
        // 创建另一个字符串进行比较
        j2me_string_t* str2 = j2me_string_create_from_cstr(vm, "Hello World!");
        j2me_string_t* str3 = j2me_string_create_from_cstr(vm, "Different");
        
        if (str2 && str3) {
            int cmp1 = j2me_string_compare(str, str2);
            int cmp2 = j2me_string_compare(str, str3);
            
            printf("✓ 字符串比较: 相同=%d, 不同=%d\n", cmp1, cmp2);
            
            if (cmp1 == 0 && cmp2 != 0) {
                printf("✓ 字符串比较功能正确\n");
            } else {
                printf("⚠ 字符串比较功能异常\n");
            }
        }
    } else {
        printf("⚠ 字符串创建失败\n");
    }
    
    // 测试对象大小计算
    if (hello_class) {
        size_t obj_size = j2me_object_calculate_size(hello_class);
        printf("✓ 对象大小计算: %zu 字节\n", obj_size);
    }
    
    // 测试数组大小计算
    size_t array_size = j2me_array_calculate_size(ARRAY_TYPE_INT, 10);
    printf("✓ 数组大小计算: %zu 字节 (10个int元素)\n", array_size);
    
    j2me_vm_destroy(vm);
    printf("✓ 虚拟机销毁完成\n");
}

int main(void) {
    printf("J2ME对象系统测试程序\n");
    printf("===================\n");
    
    test_object_system_only();
    
    printf("\n=== 对象系统测试完成 ===\n");
    return 0;
}