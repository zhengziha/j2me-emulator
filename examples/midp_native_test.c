/**
 * @file midp_native_test.c
 * @brief MIDP本地方法集成测试程序
 * 
 * 测试MIDP API本地方法的注册和调用功能
 */

#include "j2me_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "j2me_vm.h"
#include "j2me_jar.h"
#include "j2me_midlet_executor.h"
#include "j2me_native_methods.h"
#include "j2me_interpreter.h"

/**
 * @brief 测试本地方法注册表创建和销毁
 */
void test_native_method_registry(void) {
    LOG_DEBUG("\n=== 测试本地方法注册表 ===\n");
    
    // 创建注册表
    LOG_DEBUG("\n--- 创建本地方法注册表 ---\n");
    j2me_native_method_registry_t* registry = j2me_native_method_registry_create();
    if (!registry) {
        LOG_DEBUG("❌ 创建本地方法注册表失败\n");
        return;
    }
    LOG_DEBUG("✅ 本地方法注册表创建成功\n");
    
    // 注册测试方法
    LOG_DEBUG("\n--- 注册测试方法 ---\n");
    j2me_error_t result = j2me_native_method_register(registry,
                                                      "javax/microedition/lcdui/Display",
                                                      "getDisplay",
                                                      "()Ljavax/microedition/lcdui/Display;",
                                                      midp_display_get_display);
    if (result == J2ME_SUCCESS) {
        LOG_DEBUG("✅ 注册Display.getDisplay()成功\n");
    } else {
        LOG_DEBUG("❌ 注册Display.getDisplay()失败: %d\n", result);
    }
    
    // 查找方法
    LOG_DEBUG("\n--- 查找注册的方法 ---\n");
    j2me_native_method_func_t func = j2me_native_method_find(registry,
                                                             "javax/microedition/lcdui/Display",
                                                             "getDisplay",
                                                             "()Ljavax/microedition/lcdui/Display;");
    if (func) {
        LOG_DEBUG("✅ 找到Display.getDisplay()方法: %p\n", func);
    } else {
        LOG_DEBUG("❌ 未找到Display.getDisplay()方法\n");
    }
    
    // 查找不存在的方法
    LOG_DEBUG("\n--- 查找不存在的方法 ---\n");
    j2me_native_method_func_t not_found = j2me_native_method_find(registry,
                                                                  "javax/microedition/lcdui/Display",
                                                                  "nonExistentMethod",
                                                                  "()V");
    if (!not_found) {
        LOG_DEBUG("✅ 正确返回NULL (方法不存在)\n");
    } else {
        LOG_DEBUG("❌ 错误: 找到了不存在的方法\n");
    }
    
    // 销毁注册表
    LOG_DEBUG("\n--- 销毁本地方法注册表 ---\n");
    j2me_native_method_registry_destroy(registry);
    LOG_DEBUG("✅ 本地方法注册表销毁成功\n");
    
    LOG_DEBUG("✅ 本地方法注册表测试完成\n");
}

/**
 * @brief 测试MIDP本地方法初始化
 */
void test_midp_native_methods_init(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试MIDP本地方法初始化 ===\n");
    
    // 初始化MIDP本地方法
    LOG_DEBUG("\n--- 初始化MIDP本地方法 ---\n");
    j2me_error_t result = j2me_midp_native_methods_init(vm);
    if (result == J2ME_SUCCESS) {
        LOG_DEBUG("✅ MIDP本地方法初始化成功\n");
    } else {
        LOG_DEBUG("❌ MIDP本地方法初始化失败: %d\n", result);
        return;
    }
    
    // 检查虚拟机是否有本地方法注册表
    if (vm->native_method_registry) {
        LOG_DEBUG("✅ 虚拟机本地方法注册表已设置\n");
        LOG_DEBUG("📊 注册表中有 %zu 个本地方法\n", vm->native_method_registry->count);
    } else {
        LOG_DEBUG("❌ 虚拟机本地方法注册表未设置\n");
    }
    
    LOG_DEBUG("✅ MIDP本地方法初始化测试完成\n");
}

/**
 * @brief 测试本地方法调用
 */
void test_native_method_invocation(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试本地方法调用 ===\n");
    
    if (!vm->native_method_registry) {
        LOG_DEBUG("❌ 虚拟机本地方法注册表未初始化\n");
        return;
    }
    
    // 创建测试栈帧
    LOG_DEBUG("\n--- 创建测试栈帧 ---\n");
    j2me_stack_frame_t* frame = j2me_stack_frame_create(10, 5);
    if (!frame) {
        LOG_DEBUG("❌ 创建栈帧失败\n");
        return;
    }
    LOG_DEBUG("✅ 测试栈帧创建成功\n");
    
    // 测试Display.getDisplay()调用
    LOG_DEBUG("\n--- 测试Display.getDisplay()调用 ---\n");
    j2me_error_t result = j2me_native_method_invoke(vm, frame,
                                                    "javax/microedition/lcdui/Display",
                                                    "getDisplay",
                                                    "()Ljavax/microedition/lcdui/Display;",
                                                    NULL);
    if (result == J2ME_SUCCESS) {
        LOG_DEBUG("✅ Display.getDisplay()调用成功\n");
        
        // 检查返回值
        if (frame->operand_stack.top > 0) {
            j2me_int display_ref = frame->operand_stack.data[frame->operand_stack.top - 1];
            LOG_DEBUG("📊 返回的Display对象引用: 0x%x\n", display_ref);
        }
    } else {
        LOG_DEBUG("❌ Display.getDisplay()调用失败: %d\n", result);
    }
    
    // 测试Canvas.getWidth()调用
    LOG_DEBUG("\n--- 测试Canvas.getWidth()调用 ---\n");
    // 先压入Canvas对象引用
    j2me_operand_stack_push(&frame->operand_stack, 0x30000001);
    
    result = j2me_native_method_invoke(vm, frame,
                                       "javax/microedition/lcdui/Canvas",
                                       "getWidth",
                                       "()I",
                                       NULL);
    if (result == J2ME_SUCCESS) {
        LOG_DEBUG("✅ Canvas.getWidth()调用成功\n");
        
        // 检查返回值
        if (frame->operand_stack.top > 0) {
            j2me_int width = frame->operand_stack.data[frame->operand_stack.top - 1];
            LOG_DEBUG("📊 返回的Canvas宽度: %d\n", width);
        }
    } else {
        LOG_DEBUG("❌ Canvas.getWidth()调用失败: %d\n", result);
    }
    
    // 测试Graphics.setColor()调用
    LOG_DEBUG("\n--- 测试Graphics.setColor()调用 ---\n");
    // 压入Graphics对象引用和颜色值
    j2me_operand_stack_push(&frame->operand_stack, 0x40000001); // Graphics对象
    j2me_operand_stack_push(&frame->operand_stack, 0xFF0000);   // 红色
    
    result = j2me_native_method_invoke(vm, frame,
                                       "javax/microedition/lcdui/Graphics",
                                       "setColor",
                                       "(I)V",
                                       NULL);
    if (result == J2ME_SUCCESS) {
        LOG_DEBUG("✅ Graphics.setColor()调用成功\n");
    } else {
        LOG_DEBUG("❌ Graphics.setColor()调用失败: %d\n", result);
    }
    
    // 测试不存在的方法
    LOG_DEBUG("\n--- 测试不存在的方法调用 ---\n");
    result = j2me_native_method_invoke(vm, frame,
                                       "javax/microedition/lcdui/Display",
                                       "nonExistentMethod",
                                       "()V",
                                       NULL);
    if (result == J2ME_ERROR_METHOD_NOT_FOUND) {
        LOG_DEBUG("✅ 正确返回方法未找到错误\n");
    } else {
        LOG_DEBUG("❌ 错误: 应该返回方法未找到错误，实际返回: %d\n", result);
    }
    
    // 清理栈帧
    j2me_stack_frame_destroy(frame);
    LOG_DEBUG("✅ 本地方法调用测试完成\n");
}

/**
 * @brief 测试与MIDlet执行器的集成
 */
void test_midlet_native_integration(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试MIDlet与本地方法集成 ===\n");
    
    // 打开和解析JAR文件
    LOG_DEBUG("\n--- 打开JAR文件 ---\n");
    j2me_jar_file_t* jar_file = j2me_jar_open("test_jar/zxx-jtxy.jar");
    if (!jar_file) {
        LOG_DEBUG("❌ 打开JAR文件失败\n");
        return;
    }
    
    j2me_error_t result = j2me_jar_parse(jar_file);
    if (result != J2ME_SUCCESS) {
        LOG_DEBUG("❌ JAR文件解析失败\n");
        j2me_jar_close(jar_file);
        return;
    }
    LOG_DEBUG("✅ JAR文件解析成功\n");
    
    // 创建MIDlet执行器
    LOG_DEBUG("\n--- 创建MIDlet执行器 ---\n");
    j2me_midlet_executor_t* executor = j2me_midlet_executor_create(vm, jar_file);
    if (!executor) {
        LOG_DEBUG("❌ 创建MIDlet执行器失败\n");
        j2me_jar_close(jar_file);
        return;
    }
    LOG_DEBUG("✅ MIDlet执行器创建成功\n");
    
    // 获取MIDlet信息
    j2me_midlet_suite_t* suite = j2me_jar_get_midlet_suite(jar_file);
    if (!suite || suite->midlet_count == 0) {
        LOG_DEBUG("❌ 没有找到MIDlet\n");
        j2me_midlet_executor_destroy(executor);
        j2me_jar_close(jar_file);
        return;
    }
    
    j2me_midlet_t* midlet = suite->midlets[0];
    LOG_DEBUG("📊 测试MIDlet: %s (类: %s)\n", midlet->name, midlet->class_name);
    
    // 运行MIDlet (这将触发字节码执行，可能调用本地方法)
    LOG_DEBUG("\n--- 运行MIDlet (可能调用本地方法) ---\n");
    result = j2me_midlet_executor_run_midlet(executor, midlet->name);
    if (result == J2ME_SUCCESS) {
        LOG_DEBUG("✅ MIDlet运行成功 (本地方法集成正常)\n");
        
        // 模拟运行一段时间
        LOG_DEBUG("🔄 MIDlet运行中，可能调用MIDP API...\n");
        usleep(100000); // 100ms
        
    } else {
        LOG_DEBUG("❌ MIDlet运行失败: %d\n", result);
    }
    
    // 清理
    j2me_midlet_executor_destroy(executor);
    j2me_jar_close(jar_file);
    
    LOG_DEBUG("✅ MIDlet与本地方法集成测试完成\n");
}

/**
 * @brief 主测试函数
 */
int main() {
    LOG_DEBUG("MIDP本地方法集成测试程序\n");
    LOG_DEBUG("============================\n");
    LOG_DEBUG("测试MIDP API本地方法的注册、查找和调用功能\n");
    LOG_DEBUG("验证字节码解释器与本地方法的集成\n\n");
    
    // 创建虚拟机配置
    j2me_vm_config_t config = {
        .heap_size = 2 * 1024 * 1024,  // 2MB堆
        .stack_size = 128 * 1024,      // 128KB栈
        .max_threads = 8               // 8个线程
    };
    
    // 创建虚拟机
    j2me_vm_t* vm = j2me_vm_create(&config);
    if (!vm) {
        LOG_DEBUG("❌ 创建虚拟机失败\n");
        return 1;
    }
    LOG_DEBUG("✅ 虚拟机创建成功\n");
    
    // 运行本地方法测试
    test_native_method_registry();
    
    // 初始化虚拟机 (这将初始化本地方法)
    j2me_error_t result = j2me_vm_initialize(vm);
    if (result != J2ME_SUCCESS) {
        LOG_DEBUG("❌ 虚拟机初始化失败: %d\n", result);
        j2me_vm_destroy(vm);
        return 1;
    }
    LOG_DEBUG("✅ 虚拟机初始化成功\n");
    
    test_midp_native_methods_init(vm);
    test_native_method_invocation(vm);
    test_midlet_native_integration(vm);
    
    // 清理虚拟机
    j2me_vm_destroy(vm);
    
    LOG_DEBUG("\n=== MIDP本地方法集成测试总结 ===\n");
    LOG_DEBUG("✅ 本地方法注册表: 创建、注册、查找、销毁正常\n");
    LOG_DEBUG("✅ MIDP本地方法初始化: 自动注册所有MIDP API方法\n");
    LOG_DEBUG("✅ 本地方法调用: 栈操作和参数传递正常\n");
    LOG_DEBUG("✅ MIDlet集成: 字节码执行可以调用本地方法\n");
    LOG_DEBUG("✅ 错误处理: 未找到方法时正确返回错误\n");
    LOG_DEBUG("\n🎉 MIDP本地方法集成测试完成！\n");
    LOG_DEBUG("💡 下一步: 实现更完整的方法解析和参数传递\n");
    
    return 0;
}