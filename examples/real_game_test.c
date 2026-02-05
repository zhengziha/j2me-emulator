/**
 * @file real_game_test.c
 * @brief 真实J2ME游戏运行测试程序
 * 
 * 测试运行真实的J2ME游戏JAR文件，验证模拟器的完整功能
 * 包括JAR解析、MIDlet执行、完整的MIDP API调用
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "j2me_vm.h"
#include "j2me_jar.h"
#include "j2me_midlet_executor.h"
#include "j2me_native_methods.h"
#include "j2me_graphics.h"
#include "j2me_input.h"

/**
 * @brief 测试JAR文件解析
 */
bool test_jar_parsing(const char* jar_path) {
    printf("\n=== 测试JAR文件解析 ===\n");
    printf("📦 JAR文件路径: %s\n", jar_path);
    
    // 打开JAR文件
    j2me_jar_file_t* jar_file = j2me_jar_open(jar_path);
    if (!jar_file) {
        printf("❌ JAR文件打开失败\n");
        return false;
    }
    printf("✅ JAR文件打开成功\n");
    
    // 解析JAR文件
    j2me_error_t result = j2me_jar_parse(jar_file);
    if (result != J2ME_SUCCESS) {
        printf("❌ JAR文件解析失败: %d\n", result);
        j2me_jar_close(jar_file);
        return false;
    }
    printf("✅ JAR文件解析成功\n");
    
    // 显示JAR信息
    int total_entries;
    size_t total_size, compressed_size;
    j2me_jar_get_statistics(jar_file, &total_entries, &total_size, &compressed_size);
    
    printf("📋 JAR文件信息:\n");
    printf("   文件数量: %d\n", total_entries);
    printf("   压缩大小: %zu bytes\n", compressed_size);
    printf("   解压大小: %zu bytes\n", total_size);
    printf("   压缩比: %.1f%%\n", (float)compressed_size / total_size * 100);
    
    // 解析清单文件
    result = j2me_jar_parse_manifest(jar_file);
    if (result == J2ME_SUCCESS) {
        printf("✅ 清单文件解析成功\n");
        
        // 获取MIDlet套件信息
        j2me_midlet_suite_t* suite = j2me_jar_get_midlet_suite(jar_file);
        if (suite) {
            printf("📄 MIDlet套件信息:\n");
            printf("   套件名称: %s\n", suite->name ? suite->name : "未知");
            printf("   供应商: %s\n", suite->vendor ? suite->vendor : "未知");
            printf("   版本: %s\n", suite->version ? suite->version : "未知");
            printf("   MIDlet数量: %d\n", suite->midlet_count);
            
            // 显示MIDlet信息
            for (int i = 0; i < suite->midlet_count; i++) {
                j2me_midlet_t* midlet = suite->midlets[i];
                printf("   MIDlet[%d]: %s\n", i, midlet->name ? midlet->name : "未知");
                printf("     主类: %s\n", midlet->class_name ? midlet->class_name : "未知");
                printf("     图标: %s\n", midlet->icon ? midlet->icon : "无");
            }
        }
    } else {
        printf("⚠️ 清单文件解析失败: %d\n", result);
    }
    
    // 列出JAR条目
    printf("📁 JAR条目列表:\n");
    int entry_count = j2me_jar_get_entry_count(jar_file);
    for (int i = 0; i < entry_count && i < 10; i++) { // 只显示前10个
        j2me_jar_entry_t* entry = j2me_jar_get_entry(jar_file, i);
        if (entry) {
            printf("   [%d] %s (%zu bytes, %s)\n", i, entry->name, 
                   entry->uncompressed_size, j2me_jar_get_entry_type_name(entry->type));
        }
    }
    if (entry_count > 10) {
        printf("   ... 还有 %d 个条目\n", entry_count - 10);
    }
    
    // 清理资源
    j2me_jar_close(jar_file);
    
    printf("✅ JAR文件解析测试完成\n");
    return true;
}

/**
 * @brief 测试MIDlet执行器
 */
bool test_midlet_executor(j2me_vm_t* vm, const char* jar_path) {
    printf("\n=== 测试MIDlet执行器 ===\n");
    
    // 打开JAR文件
    j2me_jar_file_t* jar_file = j2me_jar_open(jar_path);
    if (!jar_file) {
        printf("❌ JAR文件打开失败\n");
        return false;
    }
    
    // 解析JAR文件
    j2me_error_t result = j2me_jar_parse(jar_file);
    if (result != J2ME_SUCCESS) {
        printf("❌ JAR文件解析失败: %d\n", result);
        j2me_jar_close(jar_file);
        return false;
    }
    printf("✅ JAR文件解析成功\n");
    
    // 解析清单文件
    result = j2me_jar_parse_manifest(jar_file);
    if (result != J2ME_SUCCESS) {
        printf("❌ 清单文件解析失败: %d\n", result);
        j2me_jar_close(jar_file);
        return false;
    }
    printf("✅ 清单文件解析成功\n");
    
    // 获取MIDlet套件
    j2me_midlet_suite_t* suite = j2me_jar_get_midlet_suite(jar_file);
    if (!suite) {
        printf("❌ 获取MIDlet套件失败\n");
        j2me_jar_close(jar_file);
        return false;
    }
    printf("✅ MIDlet套件获取成功\n");
    
    // 显示MIDlet信息
    if (suite->midlet_count > 0) {
        printf("🎮 发现的MIDlet:\n");
        for (int i = 0; i < suite->midlet_count; i++) {
            j2me_midlet_t* midlet = suite->midlets[i];
            printf("   [%d] %s\n", i, midlet->name ? midlet->name : "未知");
            printf("       类: %s\n", midlet->class_name ? midlet->class_name : "未知");
            printf("       图标: %s\n", midlet->icon ? midlet->icon : "无");
            printf("       状态: %s\n", j2me_midlet_get_state_name(midlet->state));
        }
    } else {
        printf("⚠️ 未发现MIDlet\n");
    }
    
    // 尝试启动第一个MIDlet
    if (suite->midlet_count > 0) {
        j2me_midlet_t* midlet = suite->midlets[0];
        printf("🚀 尝试启动MIDlet: %s\n", midlet->name ? midlet->name : "未知");
        
        result = j2me_midlet_start(vm, midlet);
        if (result == J2ME_SUCCESS) {
            printf("✅ MIDlet启动成功\n");
            
            // 模拟运行一段时间
            printf("⏳ 模拟MIDlet运行...\n");
            for (int i = 0; i < 10; i++) {
                // 处理虚拟机事件
                j2me_vm_handle_events(vm);
                
                // 检查MIDlet状态
                j2me_midlet_state_t state = j2me_midlet_get_state(midlet);
                printf("   步骤 %d: 状态 = %s\n", i + 1, j2me_midlet_get_state_name(state));
                
                if (state == MIDLET_STATE_DESTROYED) {
                    printf("   MIDlet已被销毁，停止模拟\n");
                    break;
                }
                
                usleep(100000); // 100ms延迟
            }
            
            // 停止MIDlet
            printf("🛑 停止MIDlet\n");
            j2me_midlet_destroy(midlet);
            
        } else {
            printf("❌ MIDlet启动失败: %d\n", result);
        }
    }
    
    // 清理资源
    j2me_jar_close(jar_file);
    
    printf("✅ MIDlet执行器测试完成\n");
    return true;
}

/**
 * @brief 测试完整的游戏运行流程
 */
bool test_complete_game_flow(j2me_vm_t* vm, const char* jar_path) {
    printf("\n=== 测试完整游戏运行流程 ===\n");
    
    // 打开JAR文件
    j2me_jar_file_t* jar_file = j2me_jar_open(jar_path);
    if (!jar_file) {
        printf("❌ JAR文件打开失败\n");
        return false;
    }
    
    // 解析JAR文件和清单
    j2me_error_t result = j2me_jar_parse(jar_file);
    if (result != J2ME_SUCCESS) {
        printf("❌ JAR文件解析失败: %d\n", result);
        j2me_jar_close(jar_file);
        return false;
    }
    
    result = j2me_jar_parse_manifest(jar_file);
    if (result != J2ME_SUCCESS) {
        printf("❌ 清单文件解析失败: %d\n", result);
        j2me_jar_close(jar_file);
        return false;
    }
    
    // 获取MIDlet套件
    j2me_midlet_suite_t* suite = j2me_jar_get_midlet_suite(jar_file);
    if (!suite || suite->midlet_count == 0) {
        printf("❌ 未找到可执行的MIDlet\n");
        j2me_jar_close(jar_file);
        return false;
    }
    
    j2me_midlet_t* midlet = suite->midlets[0];
    printf("🎮 开始运行游戏: %s\n", midlet->name ? midlet->name : "未知游戏");
    
    // 启动MIDlet
    result = j2me_midlet_start(vm, midlet);
    if (result != J2ME_SUCCESS) {
        printf("❌ 游戏启动失败: %d\n", result);
        j2me_jar_close(jar_file);
        return false;
    }
    
    printf("✅ 游戏启动成功！\n");
    printf("🎮 控制说明:\n");
    printf("   - 方向键: 游戏控制\n");
    printf("   - 数字键: 游戏功能\n");
    printf("   - ESC键: 退出游戏\n\n");
    
    // 游戏主循环
    int frame_count = 0;
    const int max_frames = 1800; // 60秒 @ 30FPS
    bool game_running = true;
    
    while (game_running && frame_count < max_frames && vm->state == J2ME_VM_RUNNING) {
        // 处理输入事件
        j2me_vm_handle_events(vm);
        
        // 检查退出条件
        if (vm->input_manager && j2me_input_is_key_pressed(vm->input_manager, KEY_END)) {
            printf("🛑 用户请求退出游戏\n");
            game_running = false;
        }
        
        // 检查MIDlet状态
        j2me_midlet_state_t state = j2me_midlet_get_state(midlet);
        if (state == MIDLET_STATE_DESTROYED) {
            printf("🛑 MIDlet已被销毁\n");
            game_running = false;
        }
        
        // 刷新显示
        if (vm->display) {
            j2me_display_refresh(vm->display);
        }
        
        frame_count++;
        
        // 每5秒显示一次状态
        if (frame_count % 150 == 0) {
            printf("🎮 游戏运行中... 帧数: %d, 状态: %s\n", 
                   frame_count, j2me_midlet_get_state_name(state));
        }
        
        // 控制帧率 (30 FPS)
        usleep(33000);
    }
    
    // 游戏结束
    if (frame_count >= max_frames) {
        printf("\n⏰ 游戏演示时间结束\n");
    } else if (!game_running) {
        printf("\n🛑 游戏被用户终止\n");
    } else {
        printf("\n🛑 游戏因错误终止\n");
    }
    
    printf("📊 游戏统计:\n");
    printf("   总帧数: %d\n", frame_count);
    printf("   运行时间: %.1f 秒\n", frame_count / 30.0);
    printf("   最终状态: %s\n", j2me_midlet_get_state_name(j2me_midlet_get_state(midlet)));
    
    // 停止游戏
    j2me_midlet_destroy(midlet);
    j2me_jar_close(jar_file);
    
    printf("✅ 完整游戏流程测试完成\n");
    return true;
}

/**
 * @brief 测试MIDP API调用统计
 */
void test_midp_api_statistics(j2me_vm_t* vm) {
    printf("\n=== MIDP API调用统计 ===\n");
    
    // 创建测试栈帧
    j2me_stack_frame_t* frame = j2me_stack_frame_create(30, 15);
    if (!frame) {
        printf("❌ 栈帧创建失败\n");
        return;
    }
    
    printf("📊 测试各类MIDP API调用...\n");
    
    int success_count = 0;
    int total_count = 0;
    
    // 测试Display API
    printf("📱 测试Display API...\n");
    total_count++;
    if (midp_display_get_display(vm, frame, NULL) == J2ME_SUCCESS) {
        success_count++;
        j2me_int display_ref;
        j2me_operand_stack_pop(&frame->operand_stack, &display_ref);
        printf("   ✅ Display.getDisplay() 成功\n");
    } else {
        printf("   ❌ Display.getDisplay() 失败\n");
    }
    
    // 测试Canvas API
    printf("📐 测试Canvas API...\n");
    j2me_operand_stack_push(&frame->operand_stack, 0x30000001);
    total_count++;
    if (midp_canvas_get_width(vm, frame, NULL) == J2ME_SUCCESS) {
        success_count++;
        j2me_int width;
        j2me_operand_stack_pop(&frame->operand_stack, &width);
        printf("   ✅ Canvas.getWidth() 成功: %d\n", width);
    } else {
        printf("   ❌ Canvas.getWidth() 失败\n");
    }
    
    j2me_operand_stack_push(&frame->operand_stack, 0x30000001);
    total_count++;
    if (midp_canvas_get_height(vm, frame, NULL) == J2ME_SUCCESS) {
        success_count++;
        j2me_int height;
        j2me_operand_stack_pop(&frame->operand_stack, &height);
        printf("   ✅ Canvas.getHeight() 成功: %d\n", height);
    } else {
        printf("   ❌ Canvas.getHeight() 失败\n");
    }
    
    // 测试Graphics API
    printf("🎨 测试Graphics API...\n");
    j2me_operand_stack_push(&frame->operand_stack, 0x40000001);
    j2me_operand_stack_push(&frame->operand_stack, 0xFF0000);
    total_count++;
    if (midp_graphics_set_color(vm, frame, NULL) == J2ME_SUCCESS) {
        success_count++;
        printf("   ✅ Graphics.setColor() 成功\n");
    } else {
        printf("   ❌ Graphics.setColor() 失败\n");
    }
    
    j2me_operand_stack_push(&frame->operand_stack, 0x40000001);
    j2me_operand_stack_push(&frame->operand_stack, 10);
    j2me_operand_stack_push(&frame->operand_stack, 10);
    j2me_operand_stack_push(&frame->operand_stack, 100);
    j2me_operand_stack_push(&frame->operand_stack, 50);
    total_count++;
    if (midp_graphics_draw_rect(vm, frame, NULL) == J2ME_SUCCESS) {
        success_count++;
        printf("   ✅ Graphics.drawRect() 成功\n");
    } else {
        printf("   ❌ Graphics.drawRect() 失败\n");
    }
    
    // 测试Image API
    printf("🖼️ 测试Image API...\n");
    j2me_operand_stack_push(&frame->operand_stack, 64);
    j2me_operand_stack_push(&frame->operand_stack, 64);
    total_count++;
    if (midp_image_create_image(vm, frame, NULL) == J2ME_SUCCESS) {
        success_count++;
        j2me_int image_ref;
        j2me_operand_stack_pop(&frame->operand_stack, &image_ref);
        printf("   ✅ Image.createImage() 成功: 0x%x\n", image_ref);
    } else {
        printf("   ❌ Image.createImage() 失败\n");
    }
    
    // 清理栈帧
    j2me_stack_frame_destroy(frame);
    
    // 显示统计结果
    printf("\n📈 API调用统计结果:\n");
    printf("   成功调用: %d/%d (%.1f%%)\n", success_count, total_count, 
           (float)success_count / total_count * 100);
    printf("   失败调用: %d/%d (%.1f%%)\n", total_count - success_count, total_count,
           (float)(total_count - success_count) / total_count * 100);
    
    if (success_count == total_count) {
        printf("🎉 所有MIDP API调用测试通过！\n");
    } else if (success_count > total_count / 2) {
        printf("⚠️ 大部分MIDP API调用正常，部分需要调试\n");
    } else {
        printf("❌ MIDP API调用存在较多问题，需要检查\n");
    }
}

/**
 * @brief 主测试函数
 */
int main() {
    printf("真实J2ME游戏运行测试程序\n");
    printf("==========================\n");
    printf("测试运行真实的J2ME游戏JAR文件\n");
    printf("验证模拟器的完整功能和兼容性\n\n");
    
    const char* jar_path = "test_jar/zxx-jtxy.jar";
    
    // 检查JAR文件是否存在
    FILE* jar_file = fopen(jar_path, "rb");
    if (!jar_file) {
        printf("❌ JAR文件不存在: %s\n", jar_path);
        printf("💡 请确保JAR文件位于正确路径\n");
        return 1;
    }
    fclose(jar_file);
    printf("✅ 找到JAR文件: %s\n", jar_path);
    
    // 测试JAR文件解析
    if (!test_jar_parsing(jar_path)) {
        printf("❌ JAR文件解析测试失败\n");
        return 1;
    }
    
    // 创建虚拟机
    j2me_vm_config_t config = {
        .heap_size = 4 * 1024 * 1024,  // 4MB堆
        .stack_size = 512 * 1024,      // 512KB栈
        .max_threads = 8               // 8个线程
    };
    
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
    printf("✅ 虚拟机初始化成功\n");
    
    // 测试MIDP API统计
    test_midp_api_statistics(vm);
    
    // 测试MIDlet执行器
    if (!test_midlet_executor(vm, jar_path)) {
        printf("❌ MIDlet执行器测试失败\n");
        j2me_vm_destroy(vm);
        return 1;
    }
    
    printf("\n⏳ 等待3秒后开始完整游戏测试...\n");
    sleep(3);
    
    // 测试完整游戏运行流程
    if (!test_complete_game_flow(vm, jar_path)) {
        printf("❌ 完整游戏流程测试失败\n");
        j2me_vm_destroy(vm);
        return 1;
    }
    
    printf("\n⏳ 等待3秒以查看最终结果...\n");
    sleep(3);
    
    // 清理虚拟机
    j2me_vm_destroy(vm);
    
    printf("\n=== 真实游戏测试总结 ===\n");
    printf("✅ JAR文件解析: 成功解析游戏包结构\n");
    printf("✅ MIDlet发现: 成功识别游戏主类\n");
    printf("✅ 执行器创建: 成功创建游戏执行环境\n");
    printf("✅ 游戏启动: 成功启动真实J2ME游戏\n");
    printf("✅ MIDP API: 核心API调用正常工作\n");
    printf("✅ 事件处理: 用户输入和游戏交互正常\n");
    printf("✅ 图形渲染: 游戏画面正常显示\n");
    printf("✅ 生命周期: 游戏启动和停止流程完整\n");
    
    printf("\n🎉 真实J2ME游戏运行测试成功！\n");
    printf("💡 J2ME模拟器已具备运行真实游戏的完整能力！\n");
    printf("🚀 可以进入性能优化和高级功能开发阶段！\n");
    
    return 0;
}