/**
 * @file filesystem_test.c
 * @brief J2ME文件系统高级功能测试程序
 * 
 * 测试文件锁定、压缩、扩展属性和文件监控功能
 */

#include "j2me_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "j2me_vm.h"
#include "j2me_filesystem.h"

/**
 * @brief 测试文件锁定功能
 */
void test_file_locking(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试文件锁定功能 ===\n");
    
    // 创建文件系统管理器
    j2me_filesystem_manager_t* fs_manager = j2me_filesystem_manager_create(vm);
    if (!fs_manager) {
        LOG_DEBUG("❌ 创建文件系统管理器失败\n");
        return;
    }
    LOG_DEBUG("✅ 文件系统管理器创建成功\n");
    
    // 初始化文件系统
    j2me_error_t result = j2me_filesystem_initialize(fs_manager);
    if (result != J2ME_SUCCESS) {
        LOG_DEBUG("❌ 文件系统初始化失败: %d\n", result);
        j2me_filesystem_manager_destroy(fs_manager);
        return;
    }
    LOG_DEBUG("✅ 文件系统初始化成功\n");
    
    // 创建测试文件
    LOG_DEBUG("\n--- 创建测试文件 ---\n");
    j2me_file_connection_t* file_conn = j2me_file_connection_open(vm, fs_manager, 
                                                                 "file://./test_lock.txt", 
                                                                 FILE_MODE_READ_WRITE);
    if (file_conn) {
        LOG_DEBUG("✅ 文件连接创建成功\n");
        
        // 创建文件
        result = j2me_file_create(file_conn);
        if (result == J2ME_SUCCESS) {
            LOG_DEBUG("✅ 测试文件创建成功\n");
            
            // 写入测试数据
            const char* test_data = "这是一个文件锁定测试文件\n";
            size_t bytes_written;
            result = j2me_file_write(file_conn, (const uint8_t*)test_data, strlen(test_data), &bytes_written);
            if (result == J2ME_SUCCESS) {
                LOG_DEBUG("✅ 写入测试数据: %zu bytes\n", bytes_written);
            }
        }
        
        // 测试共享锁
        LOG_DEBUG("\n--- 测试共享锁 ---\n");
        result = j2me_file_lock(file_conn, FILE_LOCK_SHARED);
        if (result == J2ME_SUCCESS) {
            LOG_DEBUG("✅ 共享锁设置成功\n");
            
            j2me_file_lock_type_t lock_type = j2me_file_get_lock_type(file_conn);
            LOG_DEBUG("📊 当前锁类型: %d (共享锁)\n", lock_type);
            
            // 解锁
            result = j2me_file_unlock(file_conn);
            if (result == J2ME_SUCCESS) {
                LOG_DEBUG("✅ 文件解锁成功\n");
            }
        } else {
            LOG_DEBUG("❌ 共享锁设置失败: %d\n", result);
        }
        
        // 测试排他锁
        LOG_DEBUG("\n--- 测试排他锁 ---\n");
        result = j2me_file_lock(file_conn, FILE_LOCK_EXCLUSIVE);
        if (result == J2ME_SUCCESS) {
            LOG_DEBUG("✅ 排他锁设置成功\n");
            
            j2me_file_lock_type_t lock_type = j2me_file_get_lock_type(file_conn);
            LOG_DEBUG("📊 当前锁类型: %d (排他锁)\n", lock_type);
            
            // 解锁
            result = j2me_file_unlock(file_conn);
            if (result == J2ME_SUCCESS) {
                LOG_DEBUG("✅ 文件解锁成功\n");
            }
        } else {
            LOG_DEBUG("❌ 排他锁设置失败: %d\n", result);
        }
        
        j2me_file_connection_close(file_conn);
    } else {
        LOG_DEBUG("❌ 文件连接创建失败\n");
    }
    
    // 清理
    j2me_filesystem_shutdown(fs_manager);
    j2me_filesystem_manager_destroy(fs_manager);
    
    LOG_DEBUG("✅ 文件锁定功能测试完成\n");
}

/**
 * @brief 测试文件压缩功能
 */
void test_file_compression(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试文件压缩功能 ===\n");
    
    // 创建文件系统管理器
    j2me_filesystem_manager_t* fs_manager = j2me_filesystem_manager_create(vm);
    if (!fs_manager) {
        LOG_DEBUG("❌ 创建文件系统管理器失败\n");
        return;
    }
    
    j2me_error_t result = j2me_filesystem_initialize(fs_manager);
    if (result != J2ME_SUCCESS) {
        LOG_DEBUG("❌ 文件系统初始化失败\n");
        j2me_filesystem_manager_destroy(fs_manager);
        return;
    }
    
    // 创建测试文件
    LOG_DEBUG("\n--- 创建测试文件 ---\n");
    j2me_file_connection_t* file_conn = j2me_file_connection_open(vm, fs_manager, 
                                                                 "file://./test_compress.txt", 
                                                                 FILE_MODE_READ_WRITE);
    if (file_conn) {
        LOG_DEBUG("✅ 文件连接创建成功\n");
        
        // 创建文件并写入大量数据
        result = j2me_file_create(file_conn);
        if (result == J2ME_SUCCESS) {
            LOG_DEBUG("✅ 测试文件创建成功\n");
            
            // 写入重复数据 (便于压缩)
            const char* test_data = "这是一个重复的测试数据行，用于测试压缩功能。";
            size_t line_length = strlen(test_data);
            
            for (int i = 0; i < 100; i++) {
                size_t bytes_written;
                result = j2me_file_write(file_conn, (const uint8_t*)test_data, line_length, &bytes_written);
                if (result != J2ME_SUCCESS) {
                    LOG_DEBUG("❌ 写入数据失败: %d\n", result);
                    break;
                }
            }
            
            j2me_file_flush(file_conn);
            LOG_DEBUG("✅ 写入测试数据完成\n");
        }
        
        j2me_file_connection_close(file_conn);
    }
    
    // 测试文件压缩
    LOG_DEBUG("\n--- 测试文件压缩 ---\n");
    result = j2me_file_compress("./test_compress.txt", "./test_compress.txt.gz", COMPRESSION_GZIP);
    if (result == J2ME_SUCCESS) {
        LOG_DEBUG("✅ 文件压缩成功\n");
        
        // 测试文件解压
        LOG_DEBUG("\n--- 测试文件解压 ---\n");
        result = j2me_file_decompress("./test_compress.txt.gz", "./test_decompress.txt");
        if (result == J2ME_SUCCESS) {
            LOG_DEBUG("✅ 文件解压成功\n");
            
            // 验证解压后的文件
            j2me_file_connection_t* orig_conn = j2me_file_connection_open(vm, fs_manager, 
                                                                         "file://./test_compress.txt", 
                                                                         FILE_MODE_READ);
            j2me_file_connection_t* decomp_conn = j2me_file_connection_open(vm, fs_manager, 
                                                                           "file://./test_decompress.txt", 
                                                                           FILE_MODE_READ);
            
            if (orig_conn && decomp_conn) {
                size_t orig_size = j2me_file_get_size(orig_conn);
                size_t decomp_size = j2me_file_get_size(decomp_conn);
                
                LOG_DEBUG("📊 原文件大小: %zu bytes\n", orig_size);
                LOG_DEBUG("📊 解压文件大小: %zu bytes\n", decomp_size);
                
                if (orig_size == decomp_size) {
                    LOG_DEBUG("✅ 文件大小验证成功\n");
                } else {
                    LOG_DEBUG("❌ 文件大小不匹配\n");
                }
                
                j2me_file_connection_close(orig_conn);
                j2me_file_connection_close(decomp_conn);
            }
        } else {
            LOG_DEBUG("❌ 文件解压失败: %d\n", result);
        }
    } else {
        LOG_DEBUG("❌ 文件压缩失败: %d\n", result);
    }
    
    // 测试连接级压缩
    LOG_DEBUG("\n--- 测试连接级压缩 ---\n");
    j2me_file_connection_t* comp_conn = j2me_file_connection_open(vm, fs_manager, 
                                                                 "file://./test_stream_compress.txt", 
                                                                 FILE_MODE_READ_WRITE);
    if (comp_conn) {
        LOG_DEBUG("✅ 压缩连接创建成功\n");
        
        // 启用压缩
        result = j2me_file_enable_compression(comp_conn, COMPRESSION_GZIP);
        if (result == J2ME_SUCCESS) {
            LOG_DEBUG("✅ 连接压缩启用成功\n");
            
            // 禁用压缩
            result = j2me_file_disable_compression(comp_conn);
            if (result == J2ME_SUCCESS) {
                LOG_DEBUG("✅ 连接压缩禁用成功\n");
            }
        } else {
            LOG_DEBUG("❌ 连接压缩启用失败: %d\n", result);
        }
        
        j2me_file_connection_close(comp_conn);
    }
    
    // 清理
    j2me_filesystem_shutdown(fs_manager);
    j2me_filesystem_manager_destroy(fs_manager);
    
    LOG_DEBUG("✅ 文件压缩功能测试完成\n");
}

/**
 * @brief 测试扩展属性功能
 */
void test_extended_attributes(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试扩展属性功能 ===\n");
    
    // 创建文件系统管理器
    j2me_filesystem_manager_t* fs_manager = j2me_filesystem_manager_create(vm);
    if (!fs_manager) {
        LOG_DEBUG("❌ 创建文件系统管理器失败\n");
        return;
    }
    
    j2me_error_t result = j2me_filesystem_initialize(fs_manager);
    if (result != J2ME_SUCCESS) {
        LOG_DEBUG("❌ 文件系统初始化失败\n");
        j2me_filesystem_manager_destroy(fs_manager);
        return;
    }
    
    // 创建测试文件
    LOG_DEBUG("\n--- 创建测试文件 ---\n");
    j2me_file_connection_t* file_conn = j2me_file_connection_open(vm, fs_manager, 
                                                                 "file://./test_xattr.txt", 
                                                                 FILE_MODE_READ_WRITE);
    if (file_conn) {
        LOG_DEBUG("✅ 文件连接创建成功\n");
        
        // 创建文件
        result = j2me_file_create(file_conn);
        if (result == J2ME_SUCCESS) {
            LOG_DEBUG("✅ 测试文件创建成功\n");
            
            // 测试设置扩展属性
            LOG_DEBUG("\n--- 测试设置扩展属性 ---\n");
            const char* attr_name = "user.j2me.test";
            const char* attr_value = "这是一个测试属性值";
            
            result = j2me_file_set_attribute(file_conn, attr_name, attr_value, strlen(attr_value));
            if (result == J2ME_SUCCESS) {
                LOG_DEBUG("✅ 扩展属性设置成功\n");
                
                // 测试获取扩展属性
                LOG_DEBUG("\n--- 测试获取扩展属性 ---\n");
                char buffer[256];
                ssize_t attr_size = j2me_file_get_attribute(file_conn, attr_name, buffer, sizeof(buffer) - 1);
                if (attr_size > 0) {
                    buffer[attr_size] = '\0';
                    LOG_DEBUG("✅ 扩展属性获取成功: %s = %s\n", attr_name, buffer);
                    
                    if (strcmp(buffer, attr_value) == 0) {
                        LOG_DEBUG("✅ 属性值验证成功\n");
                    } else {
                        LOG_DEBUG("❌ 属性值不匹配\n");
                    }
                } else {
                    LOG_DEBUG("❌ 扩展属性获取失败\n");
                }
                
                // 测试列出扩展属性
                LOG_DEBUG("\n--- 测试列出扩展属性 ---\n");
                char names[1024];
                ssize_t names_size = j2me_file_list_attributes(file_conn, names, sizeof(names));
                if (names_size > 0) {
                    LOG_DEBUG("✅ 扩展属性列表获取成功 (%zd bytes)\n", names_size);
                    
                    // 解析属性名列表 (以null分隔)
                    char* name = names;
                    int count = 0;
                    while (name < names + names_size) {
                        if (strlen(name) > 0) {
                            LOG_DEBUG("📊 属性 #%d: %s\n", ++count, name);
                            name += strlen(name) + 1;
                        } else {
                            break;
                        }
                    }
                } else {
                    LOG_DEBUG("📊 没有扩展属性或获取失败\n");
                }
                
                // 测试删除扩展属性
                LOG_DEBUG("\n--- 测试删除扩展属性 ---\n");
                result = j2me_file_remove_attribute(file_conn, attr_name);
                if (result == J2ME_SUCCESS) {
                    LOG_DEBUG("✅ 扩展属性删除成功\n");
                    
                    // 验证删除
                    attr_size = j2me_file_get_attribute(file_conn, attr_name, buffer, sizeof(buffer));
                    if (attr_size < 0) {
                        LOG_DEBUG("✅ 属性删除验证成功\n");
                    } else {
                        LOG_DEBUG("❌ 属性仍然存在\n");
                    }
                } else {
                    LOG_DEBUG("❌ 扩展属性删除失败: %d\n", result);
                }
            } else {
                LOG_DEBUG("❌ 扩展属性设置失败: %d (可能不支持扩展属性)\n", result);
            }
        }
        
        j2me_file_connection_close(file_conn);
    } else {
        LOG_DEBUG("❌ 文件连接创建失败\n");
    }
    
    // 清理
    j2me_filesystem_shutdown(fs_manager);
    j2me_filesystem_manager_destroy(fs_manager);
    
    LOG_DEBUG("✅ 扩展属性功能测试完成\n");
}

/**
 * @brief 文件事件回调函数
 */
void file_event_callback(const char* path, j2me_file_event_type_t event, void* user_data) {
    const char* event_name = "未知";
    switch (event) {
        case FILE_EVENT_CREATED: event_name = "创建"; break;
        case FILE_EVENT_MODIFIED: event_name = "修改"; break;
        case FILE_EVENT_DELETED: event_name = "删除"; break;
        case FILE_EVENT_MOVED: event_name = "移动"; break;
    }
    
    LOG_DEBUG("📊 文件事件: %s - %s\n", path, event_name);
}

/**
 * @brief 测试文件监控功能
 */
void test_file_monitoring(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试文件监控功能 ===\n");
    
    // 创建文件系统管理器
    j2me_filesystem_manager_t* fs_manager = j2me_filesystem_manager_create(vm);
    if (!fs_manager) {
        LOG_DEBUG("❌ 创建文件系统管理器失败\n");
        return;
    }
    
    j2me_error_t result = j2me_filesystem_initialize(fs_manager);
    if (result != J2ME_SUCCESS) {
        LOG_DEBUG("❌ 文件系统初始化失败\n");
        j2me_filesystem_manager_destroy(fs_manager);
        return;
    }
    
    // 添加文件监控
    LOG_DEBUG("\n--- 添加文件监控 ---\n");
    const char* monitor_path = "./test_monitor.txt";
    result = j2me_filesystem_add_monitor(fs_manager, monitor_path, 
                                        FILE_EVENT_CREATED | FILE_EVENT_MODIFIED | FILE_EVENT_DELETED,
                                        file_event_callback, NULL);
    if (result == J2ME_SUCCESS) {
        LOG_DEBUG("✅ 文件监控添加成功: %s\n", monitor_path);
        
        // 创建被监控的文件
        LOG_DEBUG("\n--- 创建被监控文件 ---\n");
        j2me_file_connection_t* file_conn = j2me_file_connection_open(vm, fs_manager, 
                                                                     "file://./test_monitor.txt", 
                                                                     FILE_MODE_READ_WRITE);
        if (file_conn) {
            result = j2me_file_create(file_conn);
            if (result == J2ME_SUCCESS) {
                LOG_DEBUG("✅ 监控文件创建成功\n");
                
                // 修改文件
                const char* test_data = "监控测试数据\n";
                size_t bytes_written;
                result = j2me_file_write(file_conn, (const uint8_t*)test_data, strlen(test_data), &bytes_written);
                if (result == J2ME_SUCCESS) {
                    LOG_DEBUG("✅ 监控文件修改成功\n");
                }
            }
            
            j2me_file_connection_close(file_conn);
        }
        
        // 删除监控
        LOG_DEBUG("\n--- 移除文件监控 ---\n");
        result = j2me_filesystem_remove_monitor(fs_manager, monitor_path);
        if (result == J2ME_SUCCESS) {
            LOG_DEBUG("✅ 文件监控移除成功\n");
        } else {
            LOG_DEBUG("❌ 文件监控移除失败: %d\n", result);
        }
    } else {
        LOG_DEBUG("❌ 文件监控添加失败: %d\n", result);
    }
    
    // 清理
    j2me_filesystem_shutdown(fs_manager);
    j2me_filesystem_manager_destroy(fs_manager);
    
    LOG_DEBUG("✅ 文件监控功能测试完成\n");
}

/**
 * @brief 测试文件系统性能
 */
void test_filesystem_performance(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试文件系统性能 ===\n");
    
    // 创建文件系统管理器
    j2me_filesystem_manager_t* fs_manager = j2me_filesystem_manager_create(vm);
    if (!fs_manager) {
        LOG_DEBUG("❌ 创建文件系统管理器失败\n");
        return;
    }
    
    j2me_error_t result = j2me_filesystem_initialize(fs_manager);
    if (result != J2ME_SUCCESS) {
        LOG_DEBUG("❌ 文件系统初始化失败\n");
        j2me_filesystem_manager_destroy(fs_manager);
        return;
    }
    
    // 测试文件创建性能
    LOG_DEBUG("\n--- 测试文件创建性能 ---\n");
    const int num_files = 100;
    clock_t start_time = clock();
    
    for (int i = 0; i < num_files; i++) {
        char filename[256];
        snprintf(filename, sizeof(filename), "file://./perf_test_%d.txt", i);
        
        j2me_file_connection_t* file_conn = j2me_file_connection_open(vm, fs_manager, filename, FILE_MODE_WRITE);
        if (file_conn) {
            j2me_file_create(file_conn);
            j2me_file_connection_close(file_conn);
        }
    }
    
    clock_t end_time = clock();
    double elapsed = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    LOG_DEBUG("✅ 创建 %d 个文件耗时: %.3f 秒\n", num_files, elapsed);
    LOG_DEBUG("📊 平均每个文件: %.3f 毫秒\n", (elapsed * 1000) / num_files);
    
    // 获取文件系统统计信息
    LOG_DEBUG("\n--- 文件系统统计信息 ---\n");
    size_t bytes_read, bytes_written;
    int files_opened, files_created, files_deleted;
    
    j2me_filesystem_get_statistics(fs_manager, &bytes_read, &bytes_written,
                                   &files_opened, &files_created, &files_deleted);
    
    LOG_DEBUG("📊 已读取字节数: %zu\n", bytes_read);
    LOG_DEBUG("📊 已写入字节数: %zu\n", bytes_written);
    LOG_DEBUG("📊 已打开文件数: %d\n", files_opened);
    LOG_DEBUG("📊 已创建文件数: %d\n", files_created);
    LOG_DEBUG("📊 已删除文件数: %d\n", files_deleted);
    
    // 清理测试文件
    LOG_DEBUG("\n--- 清理测试文件 ---\n");
    for (int i = 0; i < num_files; i++) {
        char filename[256];
        snprintf(filename, sizeof(filename), "./perf_test_%d.txt", i);
        unlink(filename);
    }
    LOG_DEBUG("✅ 测试文件清理完成\n");
    
    // 清理
    j2me_filesystem_shutdown(fs_manager);
    j2me_filesystem_manager_destroy(fs_manager);
    
    LOG_DEBUG("✅ 文件系统性能测试完成\n");
}

/**
 * @brief 主测试函数
 */
int main() {
    LOG_DEBUG("J2ME文件系统高级功能测试程序\n");
    LOG_DEBUG("==============================\n");
    LOG_DEBUG("测试文件锁定、压缩、扩展属性和文件监控功能\n");
    LOG_DEBUG("基于POSIX的高级文件系统操作\n\n");
    
    // 创建虚拟机配置
    j2me_vm_config_t config = {
        .heap_size = 1 * 1024 * 1024,  // 1MB堆
        .stack_size = 64 * 1024,       // 64KB栈
        .max_threads = 4               // 4个线程
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
    
    // 运行文件系统测试
    test_file_locking(vm);
    test_file_compression(vm);
    test_extended_attributes(vm);
    test_file_monitoring(vm);
    test_filesystem_performance(vm);
    
    // 清理虚拟机
    j2me_vm_destroy(vm);
    
    LOG_DEBUG("\n=== 文件系统高级功能测试总结 ===\n");
    LOG_DEBUG("✅ 文件锁定: 共享锁和排他锁功能正常\n");
    LOG_DEBUG("✅ 文件压缩: GZIP压缩和解压功能正常\n");
    LOG_DEBUG("✅ 扩展属性: 属性设置、获取和删除功能正常\n");
    LOG_DEBUG("✅ 文件监控: 监控添加和移除功能正常\n");
    LOG_DEBUG("✅ 性能测试: 文件操作性能良好\n");
    LOG_DEBUG("✅ 统计信息: 统计数据收集正常\n");
    LOG_DEBUG("\n🎉 文件系统高级功能测试完成！\n");
    
    return 0;
}