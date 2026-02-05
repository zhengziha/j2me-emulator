#include "j2me_vm.h"
#include "j2me_graphics.h"
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
    
    // 主循环
    bool running = true;
    uint32_t last_time = SDL_GetTicks();
    const uint32_t frame_time = 1000 / 60; // 60 FPS
    
    while (running) {
        uint32_t current_time = SDL_GetTicks();
        uint32_t delta_time = current_time - last_time;
        
        // 处理事件
        handle_events(&running);
        
        // 执行虚拟机时间片
        if (delta_time >= frame_time) {
            j2me_vm_execute_time_slice(vm, delta_time);
            
            // 渲染图形
            render_test_graphics(display);
            
            last_time = current_time;
        }
        
        // 避免CPU占用过高
        SDL_Delay(1);
    }
    
    printf("=== J2ME模拟器关闭 ===\n");
    
    // 清理资源
    j2me_vm_destroy(vm);
    j2me_display_destroy(display);
    
    return 0;
}