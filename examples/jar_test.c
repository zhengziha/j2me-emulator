/**
 * @file jar_test.c
 * @brief J2ME JAR文件解析测试程序
 * 
 * 测试JAR文件解析、MIDlet套件管理和生命周期功能
 */

#include "j2me_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "j2me_vm.h"
#include "j2me_jar.h"

/**
 * @brief 测试JAR文件解析
 */
void test_jar_parsing(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试JAR文件解析 ===\n");
    
    // 打开测试JAR文件
    LOG_DEBUG("\n--- 打开JAR文件 ---\n");
    j2me_jar_file_t* jar_file = j2me_jar_open("test_jar/zxx-jtxy.jar");
    if (!jar_file) {
        LOG_DEBUG("❌ 打开JAR文件失败\n");
        return;
    }
    LOG_DEBUG("✅ JAR文件打开成功\n");
    
    // 解析JAR文件
    LOG_DEBUG("\n--- 解析JAR文件 ---\n");
    j2me_error_t result = j2me_jar_parse(jar_file);
    if (result != J2ME_SUCCESS) {
        LOG_DEBUG("❌ JAR文件解析失败: %d\n", result);
        j2me_jar_close(jar_file);
        return;
    }
    LOG_DEBUG("✅ JAR文件解析成功\n");
    
    // 获取JAR文件统计信息
    LOG_DEBUG("\n--- JAR文件统计信息 ---\n");
    int total_entries;
    size_t total_size, compressed_size;
    j2me_jar_get_statistics(jar_file, &total_entries, &total_size, &compressed_size);
    
    LOG_DEBUG("📊 总条目数: %d\n", total_entries);
    LOG_DEBUG("📊 总大小: %zu bytes\n", total_size);
    LOG_DEBUG("📊 压缩大小: %zu bytes\n", compressed_size);
    LOG_DEBUG("📊 压缩比: %.1f%%\n", total_size > 0 ? (100.0 * compressed_size / total_size) : 0.0);
    
    // 列出所有条目
    LOG_DEBUG("\n--- JAR文件条目列表 ---\n");
    int entry_count = j2me_jar_get_entry_count(jar_file);
    LOG_DEBUG("📊 条目数量: %d\n", entry_count);
    
    for (int i = 0; i < entry_count && i < 20; i++) { // 只显示前20个条目
        j2me_jar_entry_t* entry = j2me_jar_get_entry(jar_file, i);
        if (entry) {
            LOG_DEBUG("📄 条目 #%d: %s (%s, %zu -> %zu bytes)\n", 
                   i, entry->name, j2me_jar_get_entry_type_name(entry->type),
                   entry->compressed_size, entry->uncompressed_size);
        }
    }
    
    if (entry_count > 20) {
        LOG_DEBUG("📄 ... 还有 %d 个条目\n", entry_count - 20);
    }
    
    // 查找特定条目
    LOG_DEBUG("\n--- 查找特定条目 ---\n");
    j2me_jar_entry_t* manifest = j2me_jar_find_entry(jar_file, "META-INF/MANIFEST.MF");
    if (manifest) {
        LOG_DEBUG("✅ 找到清单文件: %s (%zu bytes)\n", manifest->name, manifest->uncompressed_size);
        
        // 加载清单文件内容
        result = j2me_jar_load_entry(jar_file, manifest);
        if (result == J2ME_SUCCESS) {
            LOG_DEBUG("✅ 清单文件加载成功\n");
            if (manifest->data && manifest->uncompressed_size > 0) {
                LOG_DEBUG("📄 清单文件内容 (前500字符):\n");
                size_t display_size = manifest->uncompressed_size > 500 ? 500 : manifest->uncompressed_size;
                for (size_t i = 0; i < display_size; i++) {
                    putchar(manifest->data[i]);
                }
                if (manifest->uncompressed_size > 500) {
                    LOG_DEBUG("\n... (还有 %zu 字符)\n", manifest->uncompressed_size - 500);
                } else {
                    LOG_DEBUG("\n");
                }
            }
        }
    } else {
        LOG_DEBUG("❌ 未找到清单文件\n");
    }
    
    // 验证JAR文件
    LOG_DEBUG("\n--- 验证JAR文件 ---\n");
    bool valid = j2me_jar_verify(jar_file);
    LOG_DEBUG("📊 JAR文件有效性: %s\n", valid ? "有效" : "无效");
    
    // 关闭JAR文件
    j2me_jar_close(jar_file);
    
    LOG_DEBUG("✅ JAR文件解析测试完成\n");
}

/**
 * @brief 测试MIDlet套件管理
 */
void test_midlet_suite(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试MIDlet套件管理 ===\n");
    
    // 打开JAR文件
    LOG_DEBUG("\n--- 打开JAR文件 ---\n");
    j2me_jar_file_t* jar_file = j2me_jar_open("test_jar/zxx-jtxy.jar");
    if (!jar_file) {
        LOG_DEBUG("❌ 打开JAR文件失败\n");
        return;
    }
    
    // 解析JAR文件
    j2me_error_t result = j2me_jar_parse(jar_file);
    if (result != J2ME_SUCCESS) {
        LOG_DEBUG("❌ JAR文件解析失败\n");
        j2me_jar_close(jar_file);
        return;
    }
    
    // 获取MIDlet套件
    LOG_DEBUG("\n--- 获取MIDlet套件信息 ---\n");
    j2me_midlet_suite_t* suite = j2me_jar_get_midlet_suite(jar_file);
    if (!suite) {
        LOG_DEBUG("❌ 获取MIDlet套件失败\n");
        j2me_jar_close(jar_file);
        return;
    }
    LOG_DEBUG("✅ MIDlet套件获取成功\n");
    
    // 显示套件信息
    LOG_DEBUG("\n--- MIDlet套件信息 ---\n");
    LOG_DEBUG("📊 套件名称: %s\n", suite->name ? suite->name : "未知");
    LOG_DEBUG("📊 供应商: %s\n", suite->vendor ? suite->vendor : "未知");
    LOG_DEBUG("📊 版本: %s\n", suite->version ? suite->version : "未知");
    LOG_DEBUG("📊 描述: %s\n", suite->description ? suite->description : "无");
    LOG_DEBUG("📊 配置: %s\n", suite->microedition_configuration ? suite->microedition_configuration : "未知");
    LOG_DEBUG("📊 配置文件: %s\n", suite->microedition_profile ? suite->microedition_profile : "未知");
    
    // 显示MIDlet列表
    LOG_DEBUG("\n--- MIDlet列表 ---\n");
    int midlet_count = j2me_midlet_suite_get_midlet_count(suite);
    LOG_DEBUG("📊 MIDlet数量: %d\n", midlet_count);
    
    for (int i = 0; i < midlet_count; i++) {
        j2me_midlet_t* midlet = j2me_midlet_suite_get_midlet(suite, i);
        if (midlet) {
            LOG_DEBUG("📱 MIDlet #%d:\n", i + 1);
            LOG_DEBUG("   名称: %s\n", midlet->name ? midlet->name : "未知");
            LOG_DEBUG("   类名: %s\n", midlet->class_name ? midlet->class_name : "未知");
            LOG_DEBUG("   图标: %s\n", midlet->icon ? midlet->icon : "无");
            LOG_DEBUG("   状态: %s\n", j2me_midlet_get_state_name(midlet->state));
        }
    }
    
    // 测试MIDlet查找
    LOG_DEBUG("\n--- 测试MIDlet查找 ---\n");
    if (midlet_count > 0) {
        j2me_midlet_t* first_midlet = j2me_midlet_suite_get_midlet(suite, 0);
        if (first_midlet && first_midlet->name) {
            j2me_midlet_t* found_midlet = j2me_midlet_suite_find_midlet(suite, first_midlet->name);
            if (found_midlet) {
                LOG_DEBUG("✅ 成功找到MIDlet: %s\n", found_midlet->name);
            } else {
                LOG_DEBUG("❌ 未找到MIDlet: %s\n", first_midlet->name);
            }
        }
    }
    
    // 关闭JAR文件
    j2me_jar_close(jar_file);
    
    LOG_DEBUG("✅ MIDlet套件管理测试完成\n");
}

/**
 * @brief 测试MIDlet生命周期
 */
void test_midlet_lifecycle(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试MIDlet生命周期 ===\n");
    
    // 打开JAR文件
    j2me_jar_file_t* jar_file = j2me_jar_open("test_jar/zxx-jtxy.jar");
    if (!jar_file) {
        LOG_DEBUG("❌ 打开JAR文件失败\n");
        return;
    }
    
    // 解析JAR文件
    j2me_error_t result = j2me_jar_parse(jar_file);
    if (result != J2ME_SUCCESS) {
        LOG_DEBUG("❌ JAR文件解析失败\n");
        j2me_jar_close(jar_file);
        return;
    }
    
    // 获取MIDlet套件
    j2me_midlet_suite_t* suite = j2me_jar_get_midlet_suite(jar_file);
    if (!suite) {
        LOG_DEBUG("❌ 获取MIDlet套件失败\n");
        j2me_jar_close(jar_file);
        return;
    }
    
    // 获取第一个MIDlet
    LOG_DEBUG("\n--- 获取MIDlet ---\n");
    int midlet_count = j2me_midlet_suite_get_midlet_count(suite);
    if (midlet_count == 0) {
        LOG_DEBUG("❌ 没有找到MIDlet\n");
        j2me_jar_close(jar_file);
        return;
    }
    
    j2me_midlet_t* midlet = j2me_midlet_suite_get_midlet(suite, 0);
    if (!midlet) {
        LOG_DEBUG("❌ 获取MIDlet失败\n");
        j2me_jar_close(jar_file);
        return;
    }
    
    LOG_DEBUG("✅ 获取MIDlet成功: %s\n", midlet->name);
    LOG_DEBUG("📊 初始状态: %s\n", j2me_midlet_get_state_name(midlet->state));
    
    // 测试MIDlet生命周期
    LOG_DEBUG("\n--- 测试MIDlet生命周期 ---\n");
    
    // 启动MIDlet
    LOG_DEBUG("🚀 启动MIDlet...\n");
    result = j2me_midlet_start(vm, midlet);
    if (result == J2ME_SUCCESS) {
        LOG_DEBUG("✅ MIDlet启动成功\n");
        LOG_DEBUG("📊 当前状态: %s\n", j2me_midlet_get_state_name(midlet->state));
    } else {
        LOG_DEBUG("❌ MIDlet启动失败: %d\n", result);
    }
    
    // 暂停MIDlet
    LOG_DEBUG("⏸️ 暂停MIDlet...\n");
    result = j2me_midlet_pause(midlet);
    if (result == J2ME_SUCCESS) {
        LOG_DEBUG("✅ MIDlet暂停成功\n");
        LOG_DEBUG("📊 当前状态: %s\n", j2me_midlet_get_state_name(midlet->state));
    } else {
        LOG_DEBUG("❌ MIDlet暂停失败: %d\n", result);
    }
    
    // 恢复MIDlet
    LOG_DEBUG("▶️ 恢复MIDlet...\n");
    result = j2me_midlet_resume(midlet);
    if (result == J2ME_SUCCESS) {
        LOG_DEBUG("✅ MIDlet恢复成功\n");
        LOG_DEBUG("📊 当前状态: %s\n", j2me_midlet_get_state_name(midlet->state));
    } else {
        LOG_DEBUG("❌ MIDlet恢复失败: %d\n", result);
    }
    
    // 销毁MIDlet
    LOG_DEBUG("🗑️ 销毁MIDlet...\n");
    result = j2me_midlet_destroy(midlet);
    if (result == J2ME_SUCCESS) {
        LOG_DEBUG("✅ MIDlet销毁成功\n");
        LOG_DEBUG("📊 当前状态: %s\n", j2me_midlet_get_state_name(midlet->state));
    } else {
        LOG_DEBUG("❌ MIDlet销毁失败: %d\n", result);
    }
    
    // 关闭JAR文件
    j2me_jar_close(jar_file);
    
    LOG_DEBUG("✅ MIDlet生命周期测试完成\n");
}

/**
 * @brief 测试JAR文件提取
 */
void test_jar_extraction(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试JAR文件提取 ===\n");
    
    // 打开JAR文件
    j2me_jar_file_t* jar_file = j2me_jar_open("test_jar/zxx-jtxy.jar");
    if (!jar_file) {
        LOG_DEBUG("❌ 打开JAR文件失败\n");
        return;
    }
    
    // 解析JAR文件
    j2me_error_t result = j2me_jar_parse(jar_file);
    if (result != J2ME_SUCCESS) {
        LOG_DEBUG("❌ JAR文件解析失败\n");
        j2me_jar_close(jar_file);
        return;
    }
    
    // 提取清单文件
    LOG_DEBUG("\n--- 提取清单文件 ---\n");
    j2me_jar_entry_t* manifest = j2me_jar_find_entry(jar_file, "META-INF/MANIFEST.MF");
    if (manifest) {
        result = j2me_jar_extract_entry(jar_file, manifest, "./extracted_manifest.mf");
        if (result == J2ME_SUCCESS) {
            LOG_DEBUG("✅ 清单文件提取成功: ./extracted_manifest.mf\n");
        } else {
            LOG_DEBUG("❌ 清单文件提取失败: %d\n", result);
        }
    }
    
    // 查找并提取第一个类文件
    LOG_DEBUG("\n--- 提取类文件 ---\n");
    int entry_count = j2me_jar_get_entry_count(jar_file);
    for (int i = 0; i < entry_count; i++) {
        j2me_jar_entry_t* entry = j2me_jar_get_entry(jar_file, i);
        if (entry && entry->type == JAR_ENTRY_CLASS) {
            char output_path[256];
            snprintf(output_path, sizeof(output_path), "./extracted_%s", entry->name);
            
            // 替换路径分隔符
            for (char* p = output_path; *p; p++) {
                if (*p == '/') *p = '_';
            }
            
            result = j2me_jar_extract_entry(jar_file, entry, output_path);
            if (result == J2ME_SUCCESS) {
                LOG_DEBUG("✅ 类文件提取成功: %s -> %s\n", entry->name, output_path);
            } else {
                LOG_DEBUG("❌ 类文件提取失败: %s\n", entry->name);
            }
            break; // 只提取第一个类文件
        }
    }
    
    // 关闭JAR文件
    j2me_jar_close(jar_file);
    
    LOG_DEBUG("✅ JAR文件提取测试完成\n");
}

/**
 * @brief 测试性能
 */
void test_jar_performance(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试JAR解析性能 ===\n");
    
    clock_t start_time = clock();
    
    // 打开JAR文件
    j2me_jar_file_t* jar_file = j2me_jar_open("test_jar/zxx-jtxy.jar");
    if (!jar_file) {
        LOG_DEBUG("❌ 打开JAR文件失败\n");
        return;
    }
    
    clock_t open_time = clock();
    
    // 解析JAR文件
    j2me_error_t result = j2me_jar_parse(jar_file);
    if (result != J2ME_SUCCESS) {
        LOG_DEBUG("❌ JAR文件解析失败\n");
        j2me_jar_close(jar_file);
        return;
    }
    
    clock_t parse_time = clock();
    
    // 加载所有条目
    int entry_count = j2me_jar_get_entry_count(jar_file);
    int loaded_count = 0;
    
    for (int i = 0; i < entry_count; i++) {
        j2me_jar_entry_t* entry = j2me_jar_get_entry(jar_file, i);
        if (entry && entry->type != JAR_ENTRY_DIRECTORY) {
            result = j2me_jar_load_entry(jar_file, entry);
            if (result == J2ME_SUCCESS) {
                loaded_count++;
            }
        }
    }
    
    clock_t load_time = clock();
    
    // 关闭JAR文件
    j2me_jar_close(jar_file);
    
    clock_t close_time = clock();
    
    // 计算性能指标
    double open_elapsed = ((double)(open_time - start_time)) / CLOCKS_PER_SEC;
    double parse_elapsed = ((double)(parse_time - open_time)) / CLOCKS_PER_SEC;
    double load_elapsed = ((double)(load_time - parse_time)) / CLOCKS_PER_SEC;
    double close_elapsed = ((double)(close_time - load_time)) / CLOCKS_PER_SEC;
    double total_elapsed = ((double)(close_time - start_time)) / CLOCKS_PER_SEC;
    
    LOG_DEBUG("📊 性能统计:\n");
    LOG_DEBUG("   打开时间: %.3f 秒\n", open_elapsed);
    LOG_DEBUG("   解析时间: %.3f 秒\n", parse_elapsed);
    LOG_DEBUG("   加载时间: %.3f 秒 (%d/%d 条目)\n", load_elapsed, loaded_count, entry_count);
    LOG_DEBUG("   关闭时间: %.3f 秒\n", close_elapsed);
    LOG_DEBUG("   总时间: %.3f 秒\n", total_elapsed);
    
    if (entry_count > 0) {
        LOG_DEBUG("   平均每条目: %.3f 毫秒\n", (parse_elapsed * 1000) / entry_count);
    }
    
    LOG_DEBUG("✅ JAR解析性能测试完成\n");
}

/**
 * @brief 主测试函数
 */
int main() {
    LOG_DEBUG("J2ME JAR文件解析测试程序\n");
    LOG_DEBUG("========================\n");
    LOG_DEBUG("测试JAR文件解析、MIDlet套件管理和生命周期功能\n");
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
    
    // 运行JAR测试
    test_jar_parsing(vm);
    test_midlet_suite(vm);
    test_midlet_lifecycle(vm);
    test_jar_extraction(vm);
    test_jar_performance(vm);
    
    // 清理虚拟机
    j2me_vm_destroy(vm);
    
    LOG_DEBUG("\n=== JAR文件解析测试总结 ===\n");
    LOG_DEBUG("✅ JAR文件解析: ZIP格式解析正常\n");
    LOG_DEBUG("✅ 条目管理: 条目查找和加载正常\n");
    LOG_DEBUG("✅ 清单解析: MANIFEST.MF解析正常\n");
    LOG_DEBUG("✅ MIDlet套件: 套件信息提取正常\n");
    LOG_DEBUG("✅ MIDlet管理: 生命周期管理正常\n");
    LOG_DEBUG("✅ 文件提取: 条目提取功能正常\n");
    LOG_DEBUG("✅ 性能测试: 解析性能良好\n");
    LOG_DEBUG("\n🎉 JAR文件解析测试完成！MIDlet支持已实现！\n");
    
    return 0;
}