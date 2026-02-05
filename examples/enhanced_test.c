#include "../include/j2me_vm.h"
#include "../include/j2me_interpreter.h"
#include "../include/j2me_class.h"
#include "../include/j2me_bytecode.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @file enhanced_test.c
 * @brief 增强的J2ME虚拟机测试程序
 * 
 * 测试第二阶段新增的功能：扩展字节码指令集、类加载器、对象系统
 */

/**
 * @brief 测试扩展的字节码指令
 */
void test_extended_bytecode(void) {
    printf("\n=== 测试扩展字节码指令 ===\n");
    
    j2me_vm_config_t config = j2me_vm_get_default_config();
    j2me_vm_t* vm = j2me_vm_create(&config);
    if (!vm) {
        printf("错误: 虚拟机创建失败\n");
        return;
    }
    
    j2me_vm_initialize(vm);
    
    // 创建测试线程和栈帧
    j2me_thread_t thread = {0};
    thread.current_frame = j2me_stack_frame_create(20, 10);
    thread.is_running = true;
    
    if (!thread.current_frame) {
        printf("错误: 栈帧创建失败\n");
        j2me_vm_destroy(vm);
        return;
    }
    
    // 测试程序：计算 (5 + 3) * 2 - 1 = 15
    static uint8_t test_bytecode[] = {
        OPCODE_ICONST_5,    // 压入5
        OPCODE_ICONST_3,    // 压入3
        OPCODE_IADD,        // 加法: 5 + 3 = 8
        OPCODE_ICONST_2,    // 压入2
        OPCODE_IMUL,        // 乘法: 8 * 2 = 16
        OPCODE_ICONST_1,    // 压入1
        OPCODE_ISUB,        // 减法: 16 - 1 = 15
        OPCODE_DUP,         // 复制栈顶值 (15)
        OPCODE_ISTORE_0,    // 存储到局部变量0
        OPCODE_ISTORE_1,    // 存储到局部变量1
        OPCODE_RETURN       // 返回
    };
    
    thread.current_frame->bytecode = test_bytecode;
    thread.current_frame->pc = 0;
    
    printf("执行复杂计算: (5 + 3) * 2 - 1\n");
    
    // 执行字节码
    j2me_error_t result = j2me_interpreter_execute_batch(vm, &thread, 20);
    
    if (result == J2ME_SUCCESS) {
        printf("✓ 字节码执行成功\n");
        printf("✓ 计算结果: %d (存储在局部变量0)\n", thread.current_frame->local_vars.variables[0]);
        printf("✓ 复制结果: %d (存储在局部变量1)\n", thread.current_frame->local_vars.variables[1]);
        
        if (thread.current_frame->local_vars.variables[0] == 15 && 
            thread.current_frame->local_vars.variables[1] == 15) {
            printf("✓ 计算结果正确\n");
        } else {
            printf("⚠ 计算结果错误\n");
        }
    } else {
        printf("错误: 字节码执行失败 (错误码: %d)\n", result);
    }
    
    // 清理
    j2me_stack_frame_destroy(thread.current_frame);
    j2me_vm_destroy(vm);
}

/**
 * @brief 测试控制流指令
 */
void test_control_flow(void) {
    printf("\n=== 测试控制流指令 ===\n");
    
    j2me_vm_config_t config = j2me_vm_get_default_config();
    j2me_vm_t* vm = j2me_vm_create(&config);
    if (!vm) {
        printf("错误: 虚拟机创建失败\n");
        return;
    }
    
    j2me_vm_initialize(vm);
    
    j2me_thread_t thread = {0};
    thread.current_frame = j2me_stack_frame_create(20, 10);
    thread.is_running = true;
    
    if (!thread.current_frame) {
        printf("错误: 栈帧创建失败\n");
        j2me_vm_destroy(vm);
        return;
    }
    
    // 测试条件跳转：if (x > 0) x = x * 2; else x = -x;
    static uint8_t control_bytecode[] = {
        OPCODE_ICONST_5,    // 0: 压入5
        OPCODE_ISTORE_0,    // 1: 存储到x (局部变量0)
        OPCODE_ILOAD_0,     // 2: 加载x
        OPCODE_IFLE, 0, 7,  // 3: 如果x <= 0则跳转到位置12 (3+7+2)
        // x > 0的分支
        OPCODE_ILOAD_0,     // 6: 加载x
        OPCODE_ICONST_2,    // 7: 压入2
        OPCODE_IMUL,        // 8: x * 2
        OPCODE_GOTO, 0, 4,  // 9: 跳转到位置15 (9+4+2)
        // x <= 0的分支
        OPCODE_ILOAD_0,     // 12: 加载x
        OPCODE_INEG,        // 13: -x
        // 公共部分
        OPCODE_ISTORE_0,    // 14: 存储结果到x (这里是15)
        OPCODE_RETURN       // 15: 返回
    };
    
    thread.current_frame->bytecode = control_bytecode;
    thread.current_frame->pc = 0;
    
    printf("执行条件分支测试 (x=5)\n");
    
    j2me_error_t result = j2me_interpreter_execute_batch(vm, &thread, 30);
    
    if (result == J2ME_SUCCESS) {
        printf("✓ 控制流执行成功\n");
        printf("✓ 结果: %d (期望: 10)\n", thread.current_frame->local_vars.variables[0]);
        
        if (thread.current_frame->local_vars.variables[0] == 10) {
            printf("✓ 条件分支测试通过\n");
        } else {
            printf("⚠ 条件分支测试失败\n");
        }
    } else {
        printf("错误: 控制流执行失败 (错误码: %d)\n", result);
    }
    
    // 清理
    j2me_stack_frame_destroy(thread.current_frame);
    j2me_vm_destroy(vm);
}

/**
 * @brief 测试类加载器
 */
void test_class_loader(void) {
    printf("\n=== 测试类加载器 ===\n");
    
    j2me_vm_config_t config = j2me_vm_get_default_config();
    j2me_vm_t* vm = j2me_vm_create(&config);
    if (!vm) {
        printf("错误: 虚拟机创建失败\n");
        return;
    }
    
    j2me_error_t result = j2me_vm_initialize(vm);
    if (result != J2ME_SUCCESS) {
        printf("错误: 虚拟机初始化失败\n");
        j2me_vm_destroy(vm);
        return;
    }
    
    // 测试加载Hello类
    j2me_class_loader_t* loader = (j2me_class_loader_t*)vm->class_loader;
    j2me_class_t* hello_class = j2me_class_loader_load_class(loader, "Hello");
    
    if (hello_class) {
        printf("✓ 类加载成功: %s\n", hello_class->name ? hello_class->name : "unknown");
        printf("✓ 类版本: %d.%d\n", hello_class->major_version, hello_class->minor_version);
        printf("✓ 访问标志: 0x%04x\n", hello_class->access_flags);
        printf("✓ 常量池大小: %d\n", hello_class->constant_pool.count);
        printf("✓ 字段数量: %d\n", hello_class->fields_count);
        printf("✓ 方法数量: %d\n", hello_class->methods_count);
        
        // 测试类链接
        result = j2me_class_link(hello_class);
        if (result == J2ME_SUCCESS) {
            printf("✓ 类链接成功\n");
            
            // 测试类初始化
            result = j2me_class_initialize(hello_class);
            if (result == J2ME_SUCCESS) {
                printf("✓ 类初始化成功\n");
            } else {
                printf("⚠ 类初始化失败 (错误码: %d)\n", result);
            }
        } else {
            printf("⚠ 类链接失败 (错误码: %d)\n", result);
        }
        
        // 测试重复加载
        j2me_class_t* hello_class2 = j2me_class_loader_load_class(loader, "Hello");
        if (hello_class2 == hello_class) {
            printf("✓ 重复加载返回相同实例\n");
        } else {
            printf("⚠ 重复加载返回不同实例\n");
        }
        
    } else {
        printf("⚠ 类加载失败\n");
    }
    
    // 测试加载不存在的类
    j2me_class_t* nonexistent = j2me_class_loader_load_class(loader, "NonExistent");
    if (!nonexistent) {
        printf("✓ 不存在的类正确返回NULL\n");
    } else {
        printf("⚠ 不存在的类错误返回非NULL\n");
    }
    
    j2me_vm_destroy(vm);
}

/**
 * @brief 测试字节码指令信息
 */
void test_bytecode_info(void) {
    printf("\n=== 测试字节码指令信息 ===\n");
    
    // 测试指令信息查询
    const j2me_instruction_info_t* info = j2me_get_instruction_info(OPCODE_IADD);
    if (info) {
        printf("✓ IADD指令信息: 名称=%s, 操作数=%d, 栈效果=%d\n", 
               info->name, info->operand_count, info->stack_effect);
    } else {
        printf("⚠ 无法获取IADD指令信息\n");
    }
    
    // 测试指令名称查询
    const char* name = j2me_get_instruction_name(OPCODE_IFEQ);
    printf("✓ IFEQ指令名称: %s\n", name);
    
    // 测试指令长度计算
    uint8_t test_code[] = {OPCODE_BIPUSH, 42, OPCODE_SIPUSH, 0x01, 0x00, OPCODE_RETURN};
    int len1 = j2me_get_instruction_length(test_code, 0); // BIPUSH
    int len2 = j2me_get_instruction_length(test_code, 2); // SIPUSH
    int len3 = j2me_get_instruction_length(test_code, 5); // RETURN
    
    printf("✓ 指令长度: BIPUSH=%d, SIPUSH=%d, RETURN=%d\n", len1, len2, len3);
    
    if (len1 == 2 && len2 == 3 && len3 == 1) {
        printf("✓ 指令长度计算正确\n");
    } else {
        printf("⚠ 指令长度计算错误\n");
    }
}

/**
 * @brief 测试位运算指令
 */
void test_bitwise_operations(void) {
    printf("\n=== 测试位运算指令 ===\n");
    
    j2me_vm_config_t config = j2me_vm_get_default_config();
    j2me_vm_t* vm = j2me_vm_create(&config);
    if (!vm) {
        printf("错误: 虚拟机创建失败\n");
        return;
    }
    
    j2me_vm_initialize(vm);
    
    j2me_thread_t thread = {0};
    thread.current_frame = j2me_stack_frame_create(20, 10);
    thread.is_running = true;
    
    if (!thread.current_frame) {
        printf("错误: 栈帧创建失败\n");
        j2me_vm_destroy(vm);
        return;
    }
    
    // 测试位运算：12 & 10 = 8, 12 | 10 = 14, 12 ^ 10 = 6
    static uint8_t bitwise_bytecode[] = {
        // 测试按位与
        OPCODE_BIPUSH, 12,  // 压入12
        OPCODE_BIPUSH, 10,  // 压入10
        OPCODE_IAND,        // 12 & 10 = 8
        OPCODE_ISTORE_0,    // 存储结果
        
        // 测试按位或
        OPCODE_BIPUSH, 12,  // 压入12
        OPCODE_BIPUSH, 10,  // 压入10
        OPCODE_IOR,         // 12 | 10 = 14
        OPCODE_ISTORE_1,    // 存储结果
        
        // 测试按位异或
        OPCODE_BIPUSH, 12,  // 压入12
        OPCODE_BIPUSH, 10,  // 压入10
        OPCODE_IXOR,        // 12 ^ 10 = 6
        OPCODE_ISTORE_2,    // 存储结果
        
        // 测试左移
        OPCODE_ICONST_5,    // 压入5
        OPCODE_ICONST_2,    // 压入2
        OPCODE_ISHL,        // 5 << 2 = 20
        OPCODE_ISTORE_3,    // 存储结果
        
        // 测试右移
        OPCODE_BIPUSH, 20,  // 压入20
        OPCODE_ICONST_2,    // 压入2
        OPCODE_ISHR,        // 20 >> 2 = 5
        OPCODE_ISTORE, 4,   // 存储结果到局部变量4
        
        OPCODE_RETURN
    };
    
    thread.current_frame->bytecode = bitwise_bytecode;
    thread.current_frame->pc = 0;
    
    printf("执行位运算测试\n");
    
    j2me_error_t result = j2me_interpreter_execute_batch(vm, &thread, 50);
    
    if (result == J2ME_SUCCESS) {
        printf("✓ 位运算执行成功\n");
        printf("✓ 12 & 10 = %d (期望: 8)\n", thread.current_frame->local_vars.variables[0]);
        printf("✓ 12 | 10 = %d (期望: 14)\n", thread.current_frame->local_vars.variables[1]);
        printf("✓ 12 ^ 10 = %d (期望: 6)\n", thread.current_frame->local_vars.variables[2]);
        printf("✓ 5 << 2 = %d (期望: 20)\n", thread.current_frame->local_vars.variables[3]);
        printf("✓ 20 >> 2 = %d (期望: 5)\n", thread.current_frame->local_vars.variables[4]);
        
        bool all_correct = (thread.current_frame->local_vars.variables[0] == 8) &&
                          (thread.current_frame->local_vars.variables[1] == 14) &&
                          (thread.current_frame->local_vars.variables[2] == 6) &&
                          (thread.current_frame->local_vars.variables[3] == 20) &&
                          (thread.current_frame->local_vars.variables[4] == 5);
        
        if (all_correct) {
            printf("✓ 所有位运算结果正确\n");
        } else {
            printf("⚠ 部分位运算结果错误\n");
        }
    } else {
        printf("错误: 位运算执行失败 (错误码: %d)\n", result);
    }
    
    // 清理
    j2me_stack_frame_destroy(thread.current_frame);
    j2me_vm_destroy(vm);
}

int main(void) {
    printf("J2ME模拟器增强测试程序 (第二阶段)\n");
    printf("=====================================\n");
    
    // 运行所有测试
    test_bytecode_info();
    test_extended_bytecode();
    test_control_flow();
    test_bitwise_operations();
    test_class_loader();
    
    printf("\n=== 增强测试完成 ===\n");
    return 0;
}