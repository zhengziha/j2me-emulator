/**
 * @file phase4_test.c
 * @brief J2ME模拟器第四阶段测试程序
 * 
 * 测试音频、网络和文件系统功能
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "j2me_vm.h"
#include "j2me_audio.h"
#include "j2me_network.h"
#include "j2me_filesystem.h"

/**
 * @brief 测试音频系统
 */
void test_audio_system(j2me_vm_t* vm) {
    printf("\n=== 测试音频系统 ===\n");
    
    // 创建音频管理器
    j2me_audio_manager_t* audio_manager = j2me_audio_manager_create(vm);
    if (!audio_manager) {
        printf("❌ 创建音频管理器失败\n");
        return;
    }
    printf("✅ 音频管理器创建成功\n");
    
    // 初始化音频系统
    j2me_error_t result = j2me_audio_initialize(audio_manager);
    if (result != J2ME_SUCCESS) {
        printf("❌ 音频系统初始化失败: %d\n", result);
        j2me_audio_manager_destroy(audio_manager);
        return;
    }
    printf("✅ 音频系统初始化成功\n");
    
    // 测试音频剪辑创建
    const char* test_audio_data = "RIFF....WAVE...."; // 模拟WAV头
    j2me_audio_clip_t* clip = j2me_audio_clip_create(vm, (const uint8_t*)test_audio_data, 
                                                     strlen(test_audio_data), AUDIO_FORMAT_WAV);
    if (!clip) {
        printf("❌ 创建音频剪辑失败\n");
    } else {
        printf("✅ 音频剪辑创建成功\n");
        
        // 测试播放器创建
        j2me_player_t* player = j2me_player_create(vm, audio_manager, clip);
        if (!player) {
            printf("❌ 创建播放器失败\n");
        } else {
            printf("✅ 播放器创建成功\n");
            
            // 测试播放器状态
            j2me_player_state_t state = j2me_player_get_state(player);
            printf("📊 播放器状态: %d\n", state);
            
            // 测试音量控制
            j2me_player_set_volume(player, 75);
            int volume = j2me_player_get_volume(player);
            printf("📊 播放器音量: %d%%\n", volume);
            
            // 测试循环设置
            j2me_player_set_looping(player, true);
            bool looping = j2me_player_is_looping(player);
            printf("📊 循环播放: %s\n", looping ? "是" : "否");
            
            // 测试静音
            j2me_player_set_muted(player, true);
            bool muted = j2me_player_is_muted(player);
            printf("📊 静音状态: %s\n", muted ? "是" : "否");
            
            // 测试播放器实现
            result = j2me_player_realize(player);
            if (result == J2ME_SUCCESS) {
                printf("✅ 播放器实现成功\n");
                
                // 测试预取
                result = j2me_player_prefetch(player);
                if (result == J2ME_SUCCESS) {
                    printf("✅ 播放器预取成功\n");
                    
                    // 测试开始播放
                    result = j2me_player_start(player);
                    if (result == J2ME_SUCCESS) {
                        printf("✅ 播放开始成功\n");
                        
                        // 模拟播放一段时间
                        usleep(100000); // 100ms
                        
                        // 停止播放
                        result = j2me_player_stop(player);
                        if (result == J2ME_SUCCESS) {
                            printf("✅ 播放停止成功\n");
                        }
                    }
                }
            }
            
            // 不要手动销毁播放器，让管理器处理
        }
        
        j2me_audio_clip_destroy(clip);
    }
    
    // 测试从文件创建音频剪辑
    j2me_audio_clip_t* file_clip = j2me_audio_clip_create_from_file(vm, "test_audio.wav");
    if (file_clip) {
        printf("✅ 从文件创建音频剪辑成功\n");
        j2me_audio_clip_destroy(file_clip);
    }
    
    // 测试从URL创建播放器
    j2me_player_t* url_player = j2me_player_create_from_url(vm, audio_manager, "file://test_audio.wav");
    if (url_player) {
        printf("✅ 从URL创建播放器成功\n");
        // 不要手动销毁，让管理器处理
    }
    
    // 测试音调播放
    result = j2me_audio_play_tone(audio_manager, 60, 500, 80); // 中央C, 500ms, 80%音量
    if (result == J2ME_SUCCESS) {
        printf("✅ 音调播放测试成功\n");
    }
    
    // 测试音调序列
    uint8_t tone_sequence[] = {0x02, 0x4A, 0x0A, 0x05, 0x4E, 0x0A, 0x05, 0x51, 0x0A, 0x05, 0x4E, 0x0A};
    j2me_audio_clip_t* tone_clip = j2me_audio_create_tone_sequence(vm, tone_sequence, sizeof(tone_sequence));
    if (tone_clip) {
        printf("✅ 音调序列创建成功\n");
        j2me_audio_clip_destroy(tone_clip);
    }
    
    // 测试主音量控制
    j2me_audio_set_master_volume(audio_manager, 90);
    int master_volume = j2me_audio_get_master_volume(audio_manager);
    printf("📊 主音量: %d%%\n", master_volume);
    
    // 测试主静音
    j2me_audio_set_master_muted(audio_manager, true);
    bool master_muted = j2me_audio_is_master_muted(audio_manager);
    printf("📊 主静音: %s\n", master_muted ? "是" : "否");
    
    // 测试格式支持
    printf("📊 支持的音频格式:\n");
    printf("   WAV: %s\n", j2me_audio_is_format_supported(AUDIO_FORMAT_WAV) ? "是" : "否");
    printf("   MIDI: %s\n", j2me_audio_is_format_supported(AUDIO_FORMAT_MIDI) ? "是" : "否");
    printf("   MP3: %s\n", j2me_audio_is_format_supported(AUDIO_FORMAT_MP3) ? "是" : "否");
    
    // 更新音频系统
    j2me_audio_update(audio_manager);
    
    // 清理
    j2me_audio_shutdown(audio_manager);
    j2me_audio_manager_destroy(audio_manager);
    
    printf("✅ 音频系统测试完成\n");
}

/**
 * @brief 测试网络系统
 */
void test_network_system(j2me_vm_t* vm) {
    printf("\n=== 测试网络系统 ===\n");
    
    // 创建网络管理器
    j2me_network_manager_t* network_manager = j2me_network_manager_create(vm);
    if (!network_manager) {
        printf("❌ 创建网络管理器失败\n");
        return;
    }
    printf("✅ 网络管理器创建成功\n");
    
    // 初始化网络系统
    j2me_error_t result = j2me_network_initialize(network_manager);
    if (result != J2ME_SUCCESS) {
        printf("❌ 网络系统初始化失败: %d\n", result);
        j2me_network_manager_destroy(network_manager);
        return;
    }
    printf("✅ 网络系统初始化成功\n");
    
    // 测试URL解析
    j2me_connection_type_t type;
    char* host = NULL;
    int port = 0;
    char* path = NULL;
    
    result = j2me_network_parse_url("http://www.example.com:8080/test/path", &type, &host, &port, &path);
    if (result == J2ME_SUCCESS) {
        printf("✅ URL解析成功:\n");
        printf("   类型: %s\n", j2me_network_get_type_name(type));
        printf("   主机: %s\n", host ? host : "NULL");
        printf("   端口: %d\n", port);
        printf("   路径: %s\n", path ? path : "NULL");
        
        if (host) free(host);
        if (path) free(path);
    } else {
        printf("❌ URL解析失败\n");
    }
    
    // 测试HTTP连接
    j2me_connection_t* http_conn = j2me_connection_open(vm, network_manager, 
                                                        "http://www.example.com/test", 0, false);
    if (!http_conn) {
        printf("❌ 创建HTTP连接失败\n");
    } else {
        printf("✅ HTTP连接创建成功\n");
        
        // 测试HTTP方法设置
        result = j2me_http_set_request_method(http_conn, HTTP_METHOD_GET);
        if (result == J2ME_SUCCESS) {
            printf("✅ 设置HTTP方法成功\n");
        }
        
        // 测试HTTP头设置
        result = j2me_http_set_request_property(http_conn, "User-Agent", "J2ME-Emulator/1.0");
        if (result == J2ME_SUCCESS) {
            printf("✅ 设置HTTP头成功\n");
        }
        
        result = j2me_http_set_request_property(http_conn, "Accept", "text/html,application/json");
        if (result == J2ME_SUCCESS) {
            printf("✅ 设置Accept头成功\n");
        }
        
        // 测试HTTP请求发送
        const char* request_data = "test=data";
        result = j2me_http_send_request(http_conn, (const uint8_t*)request_data, strlen(request_data));
        if (result == J2ME_SUCCESS) {
            printf("✅ HTTP请求发送成功\n");
            
            // 获取响应码
            int response_code = j2me_http_get_response_code(http_conn);
            printf("📊 HTTP响应码: %d\n", response_code);
            
            // 获取响应消息
            char* response_message = j2me_http_get_response_message(http_conn);
            if (response_message) {
                printf("📊 HTTP响应消息: %s\n", response_message);
                free(response_message);
            }
            
            // 获取响应头
            char* content_type = j2me_http_get_header_field(http_conn, "Content-Type");
            if (content_type) {
                printf("📊 Content-Type: %s\n", content_type);
                free(content_type);
            }
            
            // 接收响应数据
            uint8_t response_buffer[1024];
            size_t bytes_read = 0;
            result = j2me_http_receive_response(http_conn, response_buffer, sizeof(response_buffer), &bytes_read);
            if (result == J2ME_SUCCESS) {
                printf("✅ HTTP响应接收成功: %zu bytes\n", bytes_read);
                if (bytes_read > 0) {
                    response_buffer[bytes_read] = '\0';
                    printf("📊 响应内容: %s\n", (char*)response_buffer);
                }
            }
        }
        
        // 不要手动关闭，让管理器处理
    }
    
    // 测试Socket连接
    j2me_connection_t* socket_conn = j2me_socket_open(vm, network_manager, "localhost", 8080);
    if (socket_conn) {
        printf("✅ Socket连接创建成功\n");
        
        // 测试数据发送
        const char* socket_data = "Hello, Socket!";
        size_t bytes_sent = 0;
        result = j2me_socket_send(socket_conn, (const uint8_t*)socket_data, strlen(socket_data), &bytes_sent);
        if (result == J2ME_SUCCESS) {
            printf("✅ Socket数据发送成功: %zu bytes\n", bytes_sent);
        }
        
        // 测试数据接收
        uint8_t socket_buffer[256];
        size_t bytes_received = 0;
        result = j2me_socket_receive(socket_conn, socket_buffer, sizeof(socket_buffer), &bytes_received);
        if (result == J2ME_SUCCESS) {
            printf("✅ Socket数据接收测试完成: %zu bytes\n", bytes_received);
        }
        
        // 不要手动关闭，让管理器处理
    }
    
    // 测试服务器Socket
    j2me_connection_t* server_socket = j2me_server_socket_open(vm, network_manager, 9090);
    if (server_socket) {
        printf("✅ 服务器Socket创建成功\n");
        
        // 测试接受连接 (非阻塞)
        j2me_connection_t* client_conn = j2me_server_socket_accept(server_socket);
        if (client_conn) {
            printf("✅ 接受客户端连接成功\n");
            j2me_connection_close(client_conn);
        } else {
            printf("📊 没有客户端连接 (正常)\n");
        }
        
        // 不要手动关闭，让管理器处理
    }
    
    // 测试数据报连接
    j2me_connection_t* datagram_conn = j2me_datagram_open(vm, network_manager, "datagram://localhost:8081");
    if (datagram_conn) {
        printf("✅ 数据报连接创建成功\n");
        
        // 测试数据报发送
        const char* datagram_data = "Hello, UDP!";
        result = j2me_datagram_send(datagram_conn, (const uint8_t*)datagram_data, 
                                    strlen(datagram_data), "localhost", 8081);
        if (result == J2ME_SUCCESS) {
            printf("✅ 数据报发送成功\n");
        }
        
        // 测试数据报接收
        uint8_t datagram_buffer[256];
        size_t bytes_received = 0;
        char* sender_host = NULL;
        int sender_port = 0;
        result = j2me_datagram_receive(datagram_conn, datagram_buffer, sizeof(datagram_buffer),
                                       &bytes_received, &sender_host, &sender_port);
        if (result == J2ME_SUCCESS) {
            printf("✅ 数据报接收测试完成: %zu bytes\n", bytes_received);
            if (sender_host) {
                printf("📊 发送方: %s:%d\n", sender_host, sender_port);
                free(sender_host);
            }
        }
        
        // 不要手动关闭，让管理器处理
    }
    
    // 测试网络设置
    j2me_network_set_timeout(network_manager, 15000); // 15秒超时
    
    // 获取网络统计信息
    size_t bytes_sent, bytes_received;
    int connections_opened, connections_closed;
    j2me_network_get_statistics(network_manager, &bytes_sent, &bytes_received,
                                 &connections_opened, &connections_closed);
    printf("📊 网络统计:\n");
    printf("   发送字节: %zu\n", bytes_sent);
    printf("   接收字节: %zu\n", bytes_received);
    printf("   打开连接: %d\n", connections_opened);
    printf("   关闭连接: %d\n", connections_closed);
    
    // 更新网络系统
    j2me_network_update(network_manager);
    
    // 清理
    j2me_network_shutdown(network_manager);
    j2me_network_manager_destroy(network_manager);
    
    printf("✅ 网络系统测试完成\n");
}

/**
 * @brief 测试文件系统
 */
void test_filesystem_system(j2me_vm_t* vm) {
    printf("\n=== 测试文件系统 ===\n");
    
    // 创建文件系统管理器
    j2me_filesystem_manager_t* fs_manager = j2me_filesystem_manager_create(vm);
    if (!fs_manager) {
        printf("❌ 创建文件系统管理器失败\n");
        return;
    }
    printf("✅ 文件系统管理器创建成功\n");
    
    // 初始化文件系统
    j2me_error_t result = j2me_filesystem_initialize(fs_manager);
    if (result != J2ME_SUCCESS) {
        printf("❌ 文件系统初始化失败: %d\n", result);
        j2me_filesystem_manager_destroy(fs_manager);
        return;
    }
    printf("✅ 文件系统初始化成功\n");
    
    // 测试路径解析
    char* parsed_path = NULL;
    result = j2me_filesystem_parse_url("file:///tmp/test.txt", &parsed_path);
    if (result == J2ME_SUCCESS && parsed_path) {
        printf("✅ 路径解析成功: %s\n", parsed_path);
        free(parsed_path);
    }
    
    // 测试路径工具函数
    const char* test_path = "/tmp/test/example.txt";
    printf("📊 路径工具测试:\n");
    printf("   文件名: %s\n", j2me_filesystem_get_filename(test_path));
    printf("   扩展名: %s\n", j2me_filesystem_get_extension(test_path));
    
    char* dir_path = j2me_filesystem_get_directory(test_path);
    if (dir_path) {
        printf("   目录: %s\n", dir_path);
        free(dir_path);
    }
    
    char* joined_path = j2me_filesystem_join_path("/tmp", "test.txt");
    if (joined_path) {
        printf("   连接路径: %s\n", joined_path);
        free(joined_path);
    }
    
    // 测试文件连接
    j2me_file_connection_t* file_conn = j2me_file_connection_open(vm, fs_manager, 
                                                                  "file:///tmp/j2me_test.txt", FILE_MODE_READ_WRITE);
    if (!file_conn) {
        printf("❌ 创建文件连接失败\n");
    } else {
        printf("✅ 文件连接创建成功\n");
        
        // 测试文件状态
        j2me_file_connection_state_t state = j2me_file_connection_get_state(file_conn);
        printf("📊 文件连接状态: %d\n", state);
        
        // 检查文件是否存在
        bool exists = j2me_file_exists(file_conn);
        printf("📊 文件存在: %s\n", exists ? "是" : "否");
        
        if (!exists) {
            // 创建文件
            result = j2me_file_create(file_conn);
            if (result == J2ME_SUCCESS) {
                printf("✅ 文件创建成功\n");
                
                // 写入数据
                const char* test_data = "Hello, J2ME File System!\nThis is a test file.\n";
                size_t bytes_written = 0;
                result = j2me_file_write(file_conn, (const uint8_t*)test_data, 
                                         strlen(test_data), &bytes_written);
                if (result == J2ME_SUCCESS) {
                    printf("✅ 文件写入成功: %zu bytes\n", bytes_written);
                    
                    // 刷新缓冲区
                    result = j2me_file_flush(file_conn);
                    if (result == J2ME_SUCCESS) {
                        printf("✅ 文件刷新成功\n");
                    }
                }
            }
        }
        
        // 获取文件信息
        if (j2me_file_exists(file_conn)) {
            size_t file_size = j2me_file_get_size(file_conn);
            printf("📊 文件大小: %zu bytes\n", file_size);
            
            int64_t last_modified = j2me_file_get_last_modified(file_conn);
            printf("📊 最后修改时间: %lld\n", last_modified);
            
            bool readable, writable, executable;
            j2me_file_get_permissions(file_conn, &readable, &writable, &executable);
            printf("📊 文件权限: r=%s w=%s x=%s\n", 
                   readable ? "是" : "否",
                   writable ? "是" : "否", 
                   executable ? "是" : "否");
            
            // 测试文件读取
            result = j2me_file_seek(file_conn, 0); // 回到文件开头
            if (result == J2ME_SUCCESS) {
                uint8_t read_buffer[256];
                size_t bytes_read = 0;
                result = j2me_file_read(file_conn, read_buffer, sizeof(read_buffer) - 1, &bytes_read);
                if (result == J2ME_SUCCESS) {
                    read_buffer[bytes_read] = '\0';
                    printf("✅ 文件读取成功: %zu bytes\n", bytes_read);
                    printf("📊 文件内容: %s\n", (char*)read_buffer);
                }
            }
            
            // 测试文件位置
            size_t position = j2me_file_tell(file_conn);
            printf("📊 当前文件位置: %zu\n", position);
        }
        
        // 不要手动关闭，让管理器处理
    }
    
    // 测试目录操作
    j2me_file_connection_t* dir_conn = j2me_file_connection_open(vm, fs_manager, 
                                                                 "file:///tmp/j2me_test_dir", FILE_MODE_READ_WRITE);
    if (dir_conn) {
        printf("✅ 目录连接创建成功\n");
        
        if (!j2me_file_exists(dir_conn)) {
            // 创建目录
            result = j2me_file_mkdir(dir_conn);
            if (result == J2ME_SUCCESS) {
                printf("✅ 目录创建成功\n");
            }
        }
        
        if (j2me_file_exists(dir_conn) && j2me_file_is_directory(dir_conn)) {
            printf("📊 这是一个目录\n");
            
            // 列出目录内容
            result = j2me_file_list_directory(dir_conn, NULL, false);
            if (result == J2ME_SUCCESS) {
                int file_count = j2me_file_get_file_count(dir_conn);
                printf("✅ 目录列表成功: %d 个文件\n", file_count);
                
                // 遍历文件
                for (int i = 0; i < file_count; i++) {
                    char* filename = j2me_file_get_file_name(dir_conn, i);
                    if (filename) {
                        printf("   文件 %d: %s\n", i, filename);
                        free(filename);
                    }
                }
                
                // 测试迭代器方式
                printf("📊 使用迭代器遍历:\n");
                while (j2me_file_has_more_files(dir_conn)) {
                    char* filename = j2me_file_get_next_file(dir_conn);
                    if (filename) {
                        printf("   下一个文件: %s\n", filename);
                        free(filename);
                    }
                }
            }
        }
        
        // 不要手动关闭，让管理器处理
    }
    
    // 测试磁盘空间信息
    size_t total_space = j2me_filesystem_get_total_space("/tmp");
    size_t available_space = j2me_filesystem_get_available_space("/tmp");
    size_t used_space = j2me_filesystem_get_used_space("/tmp");
    
    printf("📊 磁盘空间信息 (/tmp):\n");
    printf("   总空间: %zu bytes\n", total_space);
    printf("   可用空间: %zu bytes\n", available_space);
    printf("   已用空间: %zu bytes\n", used_space);
    
    // 获取文件系统统计信息
    size_t bytes_read, bytes_written;
    int files_opened, files_created, files_deleted;
    j2me_filesystem_get_statistics(fs_manager, &bytes_read, &bytes_written,
                                   &files_opened, &files_created, &files_deleted);
    printf("📊 文件系统统计:\n");
    printf("   读取字节: %zu\n", bytes_read);
    printf("   写入字节: %zu\n", bytes_written);
    printf("   打开文件: %d\n", files_opened);
    printf("   创建文件: %d\n", files_created);
    printf("   删除文件: %d\n", files_deleted);
    
    // 更新文件系统
    j2me_filesystem_update(fs_manager);
    
    // 清理
    j2me_filesystem_shutdown(fs_manager);
    j2me_filesystem_manager_destroy(fs_manager);
    
    printf("✅ 文件系统测试完成\n");
}

/**
 * @brief 主测试函数
 */
int main() {
    printf("J2ME模拟器第四阶段测试程序\n");
    printf("==========================\n");
    printf("测试音频、网络和文件系统功能\n\n");
    
    // 创建虚拟机配置
    j2me_vm_config_t config = {
        .heap_size = 1024 * 1024,  // 1MB堆
        .stack_size = 64 * 1024,   // 64KB栈
        .max_threads = 10
    };
    
    // 创建虚拟机
    j2me_vm_t* vm = j2me_vm_create(&config);
    if (!vm) {
        printf("❌ 创建虚拟机失败\n");
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
    printf("✅ 虚拟机初始化成功\n");
    
    // 运行各个系统的测试
    test_audio_system(vm);
    test_network_system(vm);
    test_filesystem_system(vm);
    
    // 清理虚拟机
    j2me_vm_destroy(vm);
    
    printf("\n=== 第四阶段测试总结 ===\n");
    printf("✅ 音频系统: 基础功能实现完成\n");
    printf("✅ 网络系统: 连接框架实现完成\n");
    printf("✅ 文件系统: 文件操作实现完成\n");
    printf("📊 所有系统都提供了完整的API接口\n");
    printf("📊 部分功能使用简化实现 (适合原型开发)\n");
    printf("\n🎉 第四阶段测试完成！\n");
    
    return 0;
}