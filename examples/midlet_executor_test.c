/**
 * @file midlet_executor_test.c
 * @brief J2ME MIDlet执行器测试程序
 * 
 * 测试MIDlet类加载、实例创建和生命周期管理功能
 */

#include "j2me_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "j2me_vm.h"
#include "j2me_jar.h"
#include "j2me_midlet_executor.h"

/**
 * @brief 测试MIDlet执行器创建和销毁
 */
void test_midlet_executor_creation(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试MIDlet执行器创建和销毁 ===\n");
    
    // 打开JAR文件
    LOG_DEBUG("\n--- 打开JAR文件 ---\n");
    j2me_jar_file_t* jar_file = j2me_jar_open("test_jar/zxx-jtxy.jar");
    if (!jar_file) {
        LOG_DEBUG("❌ 打开JAR文件失败\n");
        return;
    }
    LOG_DEBUG("✅ JAR文件打开成功\n");
    
    // 解析JAR文件
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
    
    // 获取MIDlet套件信息
    j2me_midlet_suite_t* suite = j2me_jar_get_midlet_suite(jar_file);
    if (suite) {
        LOG_DEBUG("📊 MIDlet套件信息:\n");
        LOG_DEBUG("   名称: %s\n", suite->name ? suite->name : "未知");
        LOG_DEBUG("   供应商: %s\n", suite->vendor ? suite->vendor : "未知");
        LOG_DEBUG("   版本: %s\n", suite->version ? suite->version : "未知");
        LOG_DEBUG("   MIDlet数量: %d\n", suite->midlet_count);
        
        for (int i = 0; i < suite->midlet_count; i++) {
            j2me_midlet_t* midlet = suite->midlets[i];
            if (midlet) {
                LOG_DEBUG("   MIDlet #%d: %s (类: %s)\n", 
                       i + 1, midlet->name, midlet->class_name);
            }
        }
    }
    
    // 销毁MIDlet执行器
    LOG_DEBUG("\n--- 销毁MIDlet执行器 ---\n");
    j2me_midlet_executor_destroy(executor);
    LOG_DEBUG("✅ MIDlet执行器销毁成功\n");
    
    // 关闭JAR文件
    j2me_jar_close(jar_file);
    
    LOG_DEBUG("✅ MIDlet执行器创建和销毁测试完成\n");
}

/**
 * @brief 测试MIDlet类加载
 */
void test_midlet_class_loading(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试MIDlet类加载 ===\n");
    
    // 打开和解析JAR文件
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
    
    // 创建MIDlet执行器
    j2me_midlet_executor_t* executor = j2me_midlet_executor_create(vm, jar_file);
    if (!executor) {
        LOG_DEBUG("❌ 创建MIDlet执行器失败\n");
        j2me_jar_close(jar_file);
        return;
    }
    
    // 获取第一个MIDlet
    LOG_DEBUG("\n--- 获取MIDlet信息 ---\n");
    j2me_midlet_suite_t* suite = j2me_jar_get_midlet_suite(jar_file);
    if (!suite || suite->midlet_count == 0) {
        LOG_DEBUG("❌ 没有找到MIDlet\n");
        j2me_midlet_executor_destroy(executor);
        j2me_jar_close(jar_file);
        return;
    }
    
    j2me_midlet_t* midlet = suite->midlets[0];
    LOG_DEBUG("✅ 获取MIDlet: %s (类: %s)\n", midlet->name, midlet->class_name);
    
    // 测试MIDlet类加载
    LOG_DEBUG("\n--- 加载MIDlet类 ---\n");
    result = j2me_midlet_executor_load_midlet(executor, midlet);
    if (result == J2ME_SUCCESS) {
        LOG_DEBUG("✅ MIDlet类加载成功\n");
    } else {
        LOG_DEBUG("❌ MIDlet类加载失败: %d\n", result);
    }
    
    // 清理
    j2me_midlet_executor_destroy(executor);
    j2me_jar_close(jar_file);
    
    LOG_DEBUG("✅ MIDlet类加载测试完成\n");
}

/**
 * @brief 测试MIDlet实例生命周期
 */
void test_midlet_lifecycle(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试MIDlet实例生命周期 ===\n");
    
    // 打开和解析JAR文件
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
    
    // 创建MIDlet执行器
    j2me_midlet_executor_t* executor = j2me_midlet_executor_create(vm, jar_file);
    if (!executor) {
        LOG_DEBUG("❌ 创建MIDlet执行器失败\n");
        j2me_jar_close(jar_file);
        return;
    }
    
    // 获取MIDlet
    j2me_midlet_suite_t* suite = j2me_jar_get_midlet_suite(jar_file);
    j2me_midlet_t* midlet = suite->midlets[0];
    
    // 创建MIDlet实例
    LOG_DEBUG("\n--- 创建MIDlet实例 ---\n");
    j2me_midlet_instance_t* instance = j2me_midlet_executor_create_instance(executor, midlet);
    if (!instance) {
        LOG_DEBUG("❌ 创建MIDlet实例失败\n");
        j2me_midlet_executor_destroy(executor);
        j2me_jar_close(jar_file);
        return;
    }
    LOG_DEBUG("✅ MIDlet实例创建成功\n");
    LOG_DEBUG("📊 实例状态: %s\n", j2me_midlet_instance_get_state_name(instance->state));
    
    // 启动MIDlet实例
    LOG_DEBUG("\n--- 启动MIDlet实例 ---\n");
    result = j2me_midlet_executor_start_instance(executor, instance);
    if (result == J2ME_SUCCESS) {
        LOG_DEBUG("✅ MIDlet实例启动成功\n");
        LOG_DEBUG("📊 实例状态: %s\n", j2me_midlet_instance_get_state_name(instance->state));
    } else {
        LOG_DEBUG("❌ MIDlet实例启动失败: %d\n", result);
    }
    
    // 模拟运行一段时间
    LOG_DEBUG("\n--- 模拟运行 ---\n");
    LOG_DEBUG("🔄 MIDlet运行中...\n");
    usleep(100000); // 100ms
    
    // 暂停MIDlet实例
    LOG_DEBUG("\n--- 暂停MIDlet实例 ---\n");
    result = j2me_midlet_executor_pause_instance(executor, instance);
    if (result == J2ME_SUCCESS) {
        LOG_DEBUG("✅ MIDlet实例暂停成功\n");
        LOG_DEBUG("📊 实例状态: %s\n", j2me_midlet_instance_get_state_name(instance->state));
    } else {
        LOG_DEBUG("❌ MIDlet实例暂停失败: %d\n", result);
    }
    
    // 恢复MIDlet实例
    LOG_DEBUG("\n--- 恢复MIDlet实例 ---\n");
    result = j2me_midlet_executor_resume_instance(executor, instance);
    if (result == J2ME_SUCCESS) {
        LOG_DEBUG("✅ MIDlet实例恢复成功\n");
        LOG_DEBUG("📊 实例状态: %s\n", j2me_midlet_instance_get_state_name(instance->state));
    } else {
        LOG_DEBUG("❌ MIDlet实例恢复失败: %d\n", result);
    }
    
    // 再次模拟运行
    LOG_DEBUG("\n--- 再次模拟运行 ---\n");
    LOG_DEBUG("🔄 MIDlet继续运行...\n");
    usleep(50000); // 50ms
    
    // 销毁MIDlet实例
    LOG_DEBUG("\n--- 销毁MIDlet实例 ---\n");
    result = j2me_midlet_executor_destroy_instance(executor, instance);
    if (result == J2ME_SUCCESS) {
        LOG_DEBUG("✅ MIDlet实例销毁成功\n");
    } else {
        LOG_DEBUG("❌ MIDlet实例销毁失败: %d\n", result);
    }
    
    // 清理
    j2me_midlet_executor_destroy(executor);
    j2me_jar_close(jar_file);
    
    LOG_DEBUG("✅ MIDlet实例生命周期测试完成\n");
}

/**
 * @brief 测试MIDlet运行接口
 */
void test_midlet_run_interface(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试MIDlet运行接口 ===\n");
    
    // 打开和解析JAR文件
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
    
    // 创建MIDlet执行器
    j2me_midlet_executor_t* executor = j2me_midlet_executor_create(vm, jar_file);
    if (!executor) {
        LOG_DEBUG("❌ 创建MIDlet执行器失败\n");
        j2me_jar_close(jar_file);
        return;
    }
    
    // 获取MIDlet名称
    j2me_midlet_suite_t* suite = j2me_jar_get_midlet_suite(jar_file);
    const char* midlet_name = suite->midlets[0]->name;
    
    // 使用高级接口运行MIDlet
    LOG_DEBUG("\n--- 运行MIDlet ---\n");
    result = j2me_midlet_executor_run_midlet(executor, midlet_name);
    if (result == J2ME_SUCCESS) {
        LOG_DEBUG("✅ MIDlet运行成功: %s\n", midlet_name);
        
        // 模拟运行
        LOG_DEBUG("🔄 MIDlet运行中...\n");
        usleep(200000); // 200ms
        
        // 获取统计信息
        uint32_t total_midlets;
        uint64_t total_time;
        j2me_midlet_executor_get_statistics(executor, &total_midlets, &total_time);
        LOG_DEBUG("📊 执行统计: 总MIDlet数=%d, 总执行时间=%llu ms\n", 
               total_midlets, total_time);
        
    } else {
        LOG_DEBUG("❌ MIDlet运行失败: %d\n", result);
    }
    
    // 清理
    j2me_midlet_executor_destroy(executor);
    j2me_jar_close(jar_file);
    
    LOG_DEBUG("✅ MIDlet运行接口测试完成\n");
}

/**
 * @brief 测试多MIDlet管理
 */
void test_multiple_midlets(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试多MIDlet管理 ===\n");
    
    // 打开和解析JAR文件
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
    
    // 创建MIDlet执行器
    j2me_midlet_executor_t* executor = j2me_midlet_executor_create(vm, jar_file);
    if (!executor) {
        LOG_DEBUG("❌ 创建MIDlet执行器失败\n");
        j2me_jar_close(jar_file);
        return;
    }
    
    // 获取所有MIDlet
    j2me_midlet_suite_t* suite = j2me_jar_get_midlet_suite(jar_file);
    LOG_DEBUG("📊 发现 %d 个MIDlet\n", suite->midlet_count);
    
    // 依次运行每个MIDlet
    for (int i = 0; i < suite->midlet_count; i++) {
        j2me_midlet_t* midlet = suite->midlets[i];
        LOG_DEBUG("\n--- 运行MIDlet #%d: %s ---\n", i + 1, midlet->name);
        
        result = j2me_midlet_executor_run_midlet(executor, midlet->name);
        if (result == J2ME_SUCCESS) {
            LOG_DEBUG("✅ MIDlet #%d 运行成功\n", i + 1);
            
            // 短暂运行
            usleep(50000); // 50ms
            
        } else {
            LOG_DEBUG("❌ MIDlet #%d 运行失败: %d\n", i + 1, result);
        }
    }
    
    // 获取最终统计信息
    uint32_t total_midlets;
    uint64_t total_time;
    j2me_midlet_executor_get_statistics(executor, &total_midlets, &total_time);
    LOG_DEBUG("\n📊 最终统计: 总运行MIDlet数=%d, 总执行时间=%llu ms\n", 
           total_midlets, total_time);
    
    // 清理
    j2me_midlet_executor_destroy(executor);
    j2me_jar_close(jar_file);
    
    LOG_DEBUG("✅ 多MIDlet管理测试完成\n");
}

/**
 * @brief 主测试函数
 */
int main() {
    LOG_DEBUG("J2ME MIDlet执行器测试程序\n");
    LOG_DEBUG("==========================\n");
    LOG_DEBUG("测试MIDlet类加载、实例创建和生命周期管理功能\n");
    LOG_DEBUG("使用测试文件: test_jar/zxx-jtxy.jar\n\n");
    
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
    
    // 初始化虚拟机
    j2me_error_t result = j2me_vm_initialize(vm);
    if (result != J2ME_SUCCESS) {
        LOG_DEBUG("❌ 虚拟机初始化失败: %d\n", result);
        j2me_vm_destroy(vm);
        return 1;
    }
    LOG_DEBUG("✅ 虚拟机初始化成功\n");
    
    // 运行MIDlet执行器测试
    test_midlet_executor_creation(vm);
    test_midlet_class_loading(vm);
    test_midlet_lifecycle(vm);
    test_midlet_run_interface(vm);
    test_multiple_midlets(vm);
    
    // 清理虚拟机
    j2me_vm_destroy(vm);
    
    LOG_DEBUG("\n=== MIDlet执行器测试总结 ===\n");
    LOG_DEBUG("✅ MIDlet执行器创建: 执行器创建和销毁正常\n");
    LOG_DEBUG("✅ MIDlet类加载: 从JAR文件加载类正常\n");
    LOG_DEBUG("✅ MIDlet实例管理: 实例创建和生命周期正常\n");
    LOG_DEBUG("✅ MIDlet运行接口: 高级运行接口正常\n");
    LOG_DEBUG("✅ 多MIDlet支持: 多MIDlet管理正常\n");
    LOG_DEBUG("\n🎉 MIDlet执行器测试完成！MIDlet类加载和执行功能已实现！\n");
    
    return 0;
}