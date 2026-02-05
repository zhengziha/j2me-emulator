#include "j2me_vm.h"
#include "j2me_graphics.h"
#include "j2me_native_methods.h"
#include "j2me_jar.h"
#include "j2me_midlet_executor.h"
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
 */
static void handle_events(bool* running) {
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
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

/**
 * @brief 渲染测试图形
 * @param display 显示系统
 */
static void render_test_graphics(j2me_display_t* display) {
    if (!display || !display->context) {
        return;
    }
    
    j2me_graphics_context_t* ctx = display->context;
    
    // 清除画布
    j2me_graphics_clear(ctx);
    
    // 绘制测试图形
    j2me_color_t red = {255, 0, 0, 255};
    j2me_color_t green = {0, 255, 0, 255};
    j2me_color_t blue = {0, 0, 255, 255};
    
    // 绘制红色矩形
    j2me_graphics_set_color(ctx, red);
    j2me_graphics_draw_rect(ctx, 10, 10, 50, 30, true);
    
    // 绘制绿色线条
    j2me_graphics_set_color(ctx, green);
    j2me_graphics_draw_line(ctx, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    j2me_graphics_draw_line(ctx, WINDOW_WIDTH, 0, 0, WINDOW_HEIGHT);
    
    // 绘制蓝色边框
    j2me_graphics_set_color(ctx, blue);
    j2me_graphics_draw_rect(ctx, 5, 5, WINDOW_WIDTH-10, WINDOW_HEIGHT-10, false);
    
    // 刷新显示
    j2me_display_refresh(display);
}

int main(int argc, char* argv[]) {
    printf("=== J2ME模拟器启动 ===\n");
    
    // 检查命令行参数
    if (argc < 2) {
        printf("用法: %s <JAR文件路径>\n", argv[0]);
        printf("示例: %s test_jar/zxx-jtxy.jar\n", argv[0]);
        return 1;
    }
    
    const char* jar_path = argv[1];
    printf("📦 加载JAR文件: %s\n", jar_path);
    
    // 初始化显示系统
    j2me_display_t* display = j2me_display_initialize(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
    if (!display) {
        printf("错误: 显示系统初始化失败\n");
        return 1;
    }
    
    // 创建图形上下文
    j2me_graphics_context_t* graphics = j2me_graphics_create_context(display, WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!graphics) {
        printf("错误: 图形上下文创建失败\n");
        j2me_display_destroy(display);
        return 1;
    }
    
    // 创建虚拟机
    j2me_vm_config_t vm_config = j2me_vm_get_default_config();
    j2me_vm_t* vm = j2me_vm_create(&vm_config);
    if (!vm) {
        printf("错误: 虚拟机创建失败\n");
        j2me_display_destroy(display);
        return 1;
    }
    
    // 初始化虚拟机
    j2me_error_t vm_result = j2me_vm_initialize(vm);
    if (vm_result != J2ME_SUCCESS) {
        printf("错误: 虚拟机初始化失败 (错误码: %d)\n", vm_result);
        j2me_vm_destroy(vm);
        j2me_display_destroy(display);
        return 1;
    }
    
    printf("所有子系统初始化完成\n");
    
    // 加载JAR文件
    printf("🎮 开始加载游戏...\n");
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
        } else {
            printf("✅ JAR文件已设置到类加载器\n");
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
    
    // 尝试加载主类以验证类加载器工作
    if (vm->class_loader && midlet->class_name) {
        printf("📚 尝试加载主类: %s\n", midlet->class_name);
        j2me_class_t* main_class = j2me_class_loader_load_class(vm->class_loader, midlet->class_name);
        if (main_class) {
            printf("✅ 主类加载成功: %s\n", midlet->class_name);
        } else {
            printf("⚠️  主类加载失败，但继续运行: %s\n", midlet->class_name);
        }
        
        // 尝试预加载一些可能的Canvas类以便后续paint方法查找
        printf("📚 预加载可能的Canvas类...\n");
        const char* possible_canvas_classes[] = {"a", "b", "c", "d", "e", NULL};
        for (int i = 0; possible_canvas_classes[i] != NULL; i++) {
            j2me_class_t* canvas_class = j2me_class_loader_load_class(vm->class_loader, possible_canvas_classes[i]);
            if (canvas_class) {
                printf("✅ 预加载类成功: %s (方法数: %d)\n", possible_canvas_classes[i], canvas_class->methods_count);
                
                // 检查是否有paint方法
                j2me_method_t* paint_method = j2me_class_find_method(canvas_class, "paint", NULL);
                if (paint_method) {
                    printf("🎨 发现paint方法: %s.paint (字节码长度: %d)\n", 
                           possible_canvas_classes[i], paint_method->bytecode_length);
                }
            }
        }
    }
    
    vm_result = j2me_midlet_start(vm, midlet);
    if (vm_result != J2ME_SUCCESS) {
        printf("❌ 游戏启动失败: %d\n", vm_result);
        j2me_jar_close(jar_file);
        j2me_vm_destroy(vm);
        j2me_display_destroy(display);
        return 1;
    }
    
    printf("✅ 游戏启动成功！\n");
    printf("🎮 控制说明: ESC键退出游戏\n\n");
    
    // 主循环
    bool running = true;
    uint32_t last_time = SDL_GetTicks();
    const uint32_t frame_time = 1000 / 60; // 60 FPS
    
    printf("🎮 进入主循环，开始持续执行游戏逻辑...\n");
    
    while (running) {
        uint32_t current_time = SDL_GetTicks();
        uint32_t delta_time = current_time - last_time;
        
        // 处理事件
        handle_events(&running);
        
        // 执行虚拟机时间片
        if (delta_time >= frame_time) {
            j2me_vm_execute_time_slice(vm, delta_time);
            
            // 处理虚拟机事件（包括Canvas重绘）
            j2me_vm_handle_events(vm);
            
            // 如果游戏有线程在运行，继续执行更多指令
            if (vm->current_thread && vm->current_thread->is_running) {
                // 尝试调用游戏的主循环方法来推进游戏逻辑
                // 查找并调用可能的游戏循环方法
                if (vm->class_loader) {
                    j2me_class_t* xmidlet_class = j2me_class_loader_find_class(vm->class_loader, "XMIDlet");
                    if (xmidlet_class) {
                        // 尝试调用run方法 (如果实现了Runnable接口)
                        j2me_method_t* run_method = j2me_class_find_method(xmidlet_class, "run", "()V");
                        if (run_method) {
                            printf("🎮 调用XMIDlet.run()方法推进游戏逻辑\n");
                            j2me_error_t exec_result = j2me_interpreter_execute_method(vm, run_method, NULL, NULL);
                            if (exec_result != J2ME_SUCCESS) {
                                printf("⚠️  XMIDlet.run()执行失败: %d\n", exec_result);
                            }
                        }
                        
                        // 尝试调用其他可能的游戏循环方法
                        const char* possible_methods[] = {"a", "b", "c", "d", "e", "f", NULL};
                        for (int i = 0; possible_methods[i] != NULL; i++) {
                            j2me_method_t* method = j2me_class_find_method(xmidlet_class, possible_methods[i], "()V");
                            if (method && method->bytecode_length > 10) { // 只调用有实际逻辑的方法
                                printf("🎮 调用XMIDlet.%s()方法\n", possible_methods[i]);
                                j2me_error_t exec_result = j2me_interpreter_execute_method(vm, method, NULL, NULL);
                                if (exec_result != J2ME_SUCCESS) {
                                    printf("⚠️  XMIDlet.%s()执行失败: %d\n", possible_methods[i], exec_result);
                                }
                                break; // 只调用一个方法，避免过度执行
                            }
                        }
                    }
                }
                
                // 继续执行当前线程的指令
                if (vm->current_thread->current_frame) {
                    j2me_error_t exec_result = j2me_interpreter_execute_batch(vm, vm->current_thread, 500);
                    if (exec_result != J2ME_SUCCESS && exec_result != J2ME_SUCCESS) {
                        printf("⚠️  游戏逻辑执行遇到问题: %d\n", exec_result);
                    }
                }
            }
            
            // 触发Canvas重绘（如果有活动的MIDlet）
            if (vm->state == J2ME_VM_RUNNING && vm->current_canvas_ref != 0) {
                // 使用真实的Canvas对象引用并触发重绘
                j2me_stack_frame_t* frame = j2me_stack_frame_create(10, 5);
                if (frame) {
                    j2me_operand_stack_push(&frame->operand_stack, vm->current_canvas_ref);
                    
                    // 调用repaint方法来更新显示
                    midp_canvas_repaint(vm, frame, NULL);
                    
                    j2me_stack_frame_destroy(frame);
                }
            }
            
            last_time = current_time;
        }
        
        // 避免CPU占用过高
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
    
    // 清理资源
    j2me_vm_destroy(vm);
    j2me_display_destroy(display);
    
    printf("👋 再见！\n");
    return 0;
}