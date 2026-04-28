#include "j2me_vm.h"
#include "j2me_graphics.h"
#include "j2me_native_methods.h"
#include "j2me_jar.h"
#include "j2me_midlet_executor.h"
#include "j2me_input.h"
#include "j2me_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

/**
 * @file main.c
 * @brief J2ME模拟器主程序
 * 
 * 程序入口点，初始化各个子系统并运行主循环
 */

// 程序配置
#define WINDOW_WIDTH    240
#define WINDOW_HEIGHT   320
#define WINDOW_TITLE    "J2ME Emulator v1.0"

/**
 * @brief 处理SDL事件
 * @param running 运行状态指针
 * @param input_manager 输入管理器
 */
static void handle_events(bool* running, j2me_input_manager_t* input_manager) {
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        // 首先尝试将事件传递给输入系统处理
        if (input_manager && j2me_input_handle_sdl_event(input_manager, &event)) {
            // 输入系统已处理该事件
            continue;
        }
        
        // 处理其他事件
        switch (event.type) {
            case SDL_QUIT:
                *running = false;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        *running = false;
                        break;
                    default:
                        break;
                }
                break;
                
            default:
                break;
        }
    }
}

int main(int argc, char* argv[]) {
    // 设置日志级别为INFO（减少调试输出）
    j2me_log_set_level(J2ME_LOG_LEVEL_INFO);
    
    printf("=== J2ME模拟器启动 ===\n");
    
    // 检查命令行参数
    if (argc < 2) {
        printf("用法: %s <JAR文件路径> [选项]\n", argv[0]);
        printf("选项:\n");
        printf("  -v, --verbose    显示详细调试信息\n");
        printf("  -q, --quiet      只显示错误信息\n");
        printf("示例: %s test_jar/zxfml.jar\n", argv[0]);
        return 1;
    }
    
    const char* jar_path = argv[1];
    
    // 处理命令行选项
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            j2me_log_set_level(J2ME_LOG_LEVEL_DEBUG);
            printf("调试模式已启用\n");
        } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
            j2me_log_set_level(J2ME_LOG_LEVEL_ERROR);
        }
    }
    
    printf("📦 加载JAR文件: %s\n", jar_path);
    
    // 初始化显示系统
    j2me_display_t* display = j2me_display_initialize(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
    if (!display) {
        printf("错误: 显示系统初始化失败\n");
        return 1;
    }
    
    // 创建图形上下文
    display->context = j2me_graphics_create_context(display, WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!display->context) {
        printf("错误: 图形上下文创建失败\n");
        j2me_display_destroy(display);
        return 1;
    }
    printf("✅ 图形上下文创建成功\n");
    
    // 创建虚拟机（使用Phase 1的堆系统）
    j2me_vm_config_t vm_config = j2me_vm_get_default_config();
    vm_config.heap_size = 2 * 1024 * 1024;  // 2MB堆
    
    j2me_vm_t* vm = j2me_vm_create(&vm_config);
    if (!vm) {
        printf("错误: 虚拟机创建失败\n");
        j2me_display_destroy(display);
        return 1;
    }
    
    printf("✅ 虚拟机初始化完成\n");
    
    // 创建输入管理器
    j2me_input_manager_t* input_manager = j2me_input_manager_create();
    if (!input_manager) {
        printf("错误: 输入管理器创建失败\n");
        j2me_vm_destroy(vm);
        j2me_display_destroy(display);
        return 1;
    }
    
    // 将display设置到虚拟机中，避免重复创建
    vm->display = display;
    
    // 初始化虚拟机（但跳过display创建，因为我们已经设置了）
    j2me_error_t vm_result = j2me_vm_initialize(vm);
    if (vm_result != J2ME_SUCCESS) {
        printf("错误: 虚拟机初始化失败 (错误码: %d)\n", vm_result);
        j2me_vm_destroy(vm);
        return 1;
    }
    
    printf("所有子系统初始化完成\n");
    
    // 加载JAR文件
    printf("🎮 加载游戏...\n");
    j2me_jar_file_t* jar_file = j2me_jar_open(jar_path);
    if (!jar_file) {
        printf("❌ JAR文件打开失败: %s\n", jar_path);
        j2me_vm_destroy(vm);
        j2me_display_destroy(display);
        return 1;
    }
    
    // 解析JAR文件
    vm_result = j2me_jar_parse(jar_file);
    if (vm_result != J2ME_SUCCESS) {
        printf("❌ JAR文件解析失败: %d\n", vm_result);
        j2me_jar_close(jar_file);
        j2me_vm_destroy(vm);
        j2me_display_destroy(display);
        return 1;
    }
    
    // 解析清单文件
    vm_result = j2me_jar_parse_manifest(jar_file);
    if (vm_result != J2ME_SUCCESS) {
        printf("❌ 清单文件解析失败: %d\n", vm_result);
        j2me_jar_close(jar_file);
        j2me_vm_destroy(vm);
        j2me_display_destroy(display);
        return 1;
    }
    
    // 将JAR文件设置到类加载器
    if (vm->class_loader) {
        j2me_error_t loader_result = j2me_class_loader_set_jar_file(vm->class_loader, jar_file);
        if (loader_result != J2ME_SUCCESS) {
            printf("❌ 设置JAR文件到类加载器失败: %d\n", loader_result);
        }
    }
    
    // 获取MIDlet套件
    j2me_midlet_suite_t* suite = j2me_jar_get_midlet_suite(jar_file);
    if (!suite || suite->midlet_count == 0) {
        printf("❌ 未找到可执行的MIDlet\n");
        j2me_jar_close(jar_file);
        j2me_vm_destroy(vm);
        j2me_display_destroy(display);
        return 1;
    }
    
    // 启动第一个MIDlet
    j2me_midlet_t* midlet = suite->midlets[0];
    printf("🚀 启动游戏: %s\n", midlet->name ? midlet->name : "未知游戏");
    printf("📦 MIDlet主类: %s\n", midlet->class_name ? midlet->class_name : "未知");
    
    printf("🚀 启动游戏（这可能需要几秒钟）...\n");
    fflush(stdout);
    
    vm_result = j2me_midlet_start(vm, midlet);
    
    printf("📍 j2me_midlet_start 返回: %d\n", vm_result);
    fflush(stdout);
    
    if (vm_result != J2ME_SUCCESS) {
        printf("❌ 游戏启动失败: %d\n", vm_result);
        j2me_jar_close(jar_file);
        j2me_vm_destroy(vm);
        j2me_display_destroy(display);
        return 1;
    }
    
    printf("✅ 游戏启动成功！\n");
    printf("🎮 按ESC键退出\n\n");
    
    // 主循环
    bool running = true;
    uint32_t last_time = SDL_GetTicks();
    uint32_t start_time = SDL_GetTicks();
    const uint32_t frame_time = 1000 / 60; // 60 FPS
    int frame_counter = 0;
    
    printf("进入主循环...\n");
    
    while (running) {
        uint32_t current_time = SDL_GetTicks();
        uint32_t delta_time = current_time - last_time;
        uint32_t elapsed_time = current_time - start_time;
        
        // 每5秒输出一次状态
        if (frame_counter % 300 == 0) {
            printf("🎮 游戏运行中... 帧数: %d, 运行时间: %.1f秒, 线程数: %zu\n", 
                   frame_counter, elapsed_time / 1000.0, vm->thread_count);
            fflush(stdout);
        }
        
        // 处理事件
        handle_events(&running, input_manager);
        
        // 每帧都执行，不要等待frame_time
        j2me_vm_execute_time_slice(vm, delta_time);
        
        // 处理虚拟机事件（包括Canvas重绘）
        j2me_vm_handle_events(vm);
        
        // 执行所有活动线程（包括游戏线程）
        j2me_vm_execute_all_threads(vm, 1000);
        
        // 触发Canvas重绘（如果有活动的Canvas）
        if (vm->state == J2ME_VM_RUNNING && vm->current_canvas_ref != 0) {
            // 使用真实的堆对象引用触发重绘
            static int repaint_counter = 0;
            repaint_counter++;
            
            // 每30帧触发一次重绘（约2 FPS），降低频率避免崩溃
            if (repaint_counter % 30 == 0) {
                printf("[主循环] 触发Canvas重绘 (Canvas=0x%x)\n", vm->current_canvas_ref);
                
                j2me_stack_frame_t* frame = j2me_stack_frame_create(10, 5);
                if (frame) {
                    // 压入Canvas对象引用
                    j2me_operand_stack_push(&frame->operand_stack, vm->current_canvas_ref);
                    
                    // 调用Canvas.repaint()
                    j2me_error_t result = midp_canvas_repaint(vm, frame, NULL);
                    if (result != J2ME_SUCCESS) {
                        printf("[主循环] Canvas.repaint()失败: %d\n", result);
                    }
                    
                    j2me_stack_frame_destroy(frame);
                }
            }
        }
        
        // 更新时间和帧计数
        if (delta_time >= frame_time) {
            last_time = current_time;
        }
        frame_counter++;
        
        // 控制帧率
        SDL_Delay(1);
    }
    
    printf("=== J2ME模拟器关闭 ===\n");
    
    // 停止MIDlet
    if (midlet) {
        printf("🛑 停止游戏...\n");
        j2me_midlet_destroy(midlet);
    }
    
    // 关闭JAR文件
    if (jar_file) {
        j2me_jar_close(jar_file);
    }
    
    // 清理资源 - 注意：j2me_vm_destroy会自动清理display，所以不需要单独清理
    j2me_input_manager_destroy(input_manager);
    j2me_vm_destroy(vm);
    // j2me_display_destroy(display); // 已经在j2me_vm_destroy中清理了
    
    printf("👋 再见！\n");
    return 0;
}