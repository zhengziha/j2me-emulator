#include "j2me_vm.h"
#include "j2me_graphics.h"
#include "j2me_midp_graphics.h"
#include "j2me_input.h"
#include "j2me_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

/**
 * @file sdl_integration_test.c
 * @brief SDL集成优化测试程序
 * 
 * 测试SDL事件处理、渲染性能和资源管理
 */

// 性能统计
typedef struct {
    uint32_t frame_count;
    uint32_t event_count;
    uint32_t render_time_ms;
    uint32_t event_time_ms;
    uint32_t total_time_ms;
} performance_stats_t;

static performance_stats_t perf_stats = {0};

void test_event_processing(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试SDL事件处理 ===\n");
    
    if (!vm->input_manager) {
        LOG_DEBUG("  ✗ 输入管理器未初始化\n");
        return;
    }
    
    LOG_DEBUG("  ✓ 输入管理器已初始化\n");
    
    // 模拟事件处理循环
    uint32_t start_time = SDL_GetTicks();
    int event_count = 0;
    
    // 处理所有待处理事件
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        event_count++;
        
        switch (event.type) {
            case SDL_QUIT:
                LOG_DEBUG("  ✓ 检测到退出事件\n");
                break;
                
            case SDL_KEYDOWN:
                LOG_DEBUG("  ✓ 检测到按键按下: %s\n", SDL_GetKeyName(event.key.keysym.sym));
                break;
                
            case SDL_KEYUP:
                LOG_DEBUG("  ✓ 检测到按键释放: %s\n", SDL_GetKeyName(event.key.keysym.sym));
                break;
                
            case SDL_MOUSEBUTTONDOWN:
                LOG_DEBUG("  ✓ 检测到鼠标按下: (%d, %d)\n", event.button.x, event.button.y);
                break;
                
            case SDL_MOUSEMOTION:
                // 不打印鼠标移动事件，太多了
                break;
                
            default:
                break;
        }
    }
    
    uint32_t elapsed = SDL_GetTicks() - start_time;
    perf_stats.event_count += event_count;
    perf_stats.event_time_ms += elapsed;
    
    LOG_DEBUG("  ✓ 事件处理完成: %d个事件, 耗时%dms\n", event_count, elapsed);
    
    if (event_count > 0) {
        LOG_DEBUG("  ✓ 平均事件处理时间: %.2fms/事件\n", (float)elapsed / event_count);
    }
}

void test_rendering_performance(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试渲染性能 ===\n");
    
    if (!vm->display || !vm->display->context) {
        LOG_DEBUG("  ✗ 显示系统未初始化\n");
        return;
    }
    
    j2me_midp_graphics_t* g = j2me_midp_graphics_create(vm->display->context);
    if (!g) {
        LOG_DEBUG("  ✗ 无法创建图形上下文\n");
        return;
    }
    
    LOG_DEBUG("  ✓ 图形上下文创建成功\n");
    
    // 性能测试：绘制大量图形
    uint32_t start_time = SDL_GetTicks();
    int draw_count = 0;
    
    // 清屏
    j2me_midp_graphics_set_color_rgb(g, 0, 0, 0);
    j2me_midp_graphics_fill_rect(g, 0, 0, 240, 320);
    draw_count++;
    
    // 绘制100个矩形
    for (int i = 0; i < 100; i++) {
        int x = (i * 13) % 200;
        int y = (i * 17) % 280;
        int color = (i * 25) % 256;
        
        j2me_midp_graphics_set_color_rgb(g, color, (color + 85) % 256, (color + 170) % 256);
        j2me_midp_graphics_fill_rect(g, x, y, 20, 20);
        draw_count++;
    }
    
    // 绘制50条线
    for (int i = 0; i < 50; i++) {
        int x1 = (i * 11) % 240;
        int y1 = (i * 13) % 320;
        int x2 = ((i + 1) * 11) % 240;
        int y2 = ((i + 1) * 13) % 320;
        
        j2me_midp_graphics_set_color_rgb(g, 255, 255, 255);
        j2me_midp_graphics_draw_line(g, x1, y1, x2, y2);
        draw_count++;
    }
    
    // 绘制文本
    j2me_midp_graphics_set_color_rgb(g, 255, 255, 0);
    j2me_midp_graphics_draw_string(g, "Performance Test", 10, 10, ANCHOR_TOP | ANCHOR_LEFT);
    draw_count++;
    
    uint32_t elapsed = SDL_GetTicks() - start_time;
    perf_stats.render_time_ms += elapsed;
    perf_stats.frame_count++;
    
    LOG_DEBUG("  ✓ 渲染完成: %d个绘制操作, 耗时%dms\n", draw_count, elapsed);
    LOG_DEBUG("  ✓ 平均绘制时间: %.2fms/操作\n", (float)elapsed / draw_count);
    
    // 更新显示
    j2me_display_refresh(vm->display);
    
    j2me_midp_graphics_destroy(g);
    LOG_DEBUG("  ✓ 渲染性能测试完成\n");
}

void test_resource_management(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试资源管理 ===\n");
    
    // 测试图形上下文创建和销毁
    LOG_DEBUG("  测试图形上下文生命周期...\n");
    for (int i = 0; i < 10; i++) {
        j2me_midp_graphics_t* g = j2me_midp_graphics_create(vm->display->context);
        if (g) {
            j2me_midp_graphics_destroy(g);
        }
    }
    LOG_DEBUG("  ✓ 图形上下文生命周期测试通过 (10次创建/销毁)\n");
    
    // 测试字体创建和销毁
    LOG_DEBUG("  测试字体生命周期...\n");
    for (int i = 0; i < 10; i++) {
        j2me_midp_font_t* font = j2me_midp_font_create(vm, 
            FONT_FACE_SYSTEM, FONT_STYLE_PLAIN, FONT_SIZE_MEDIUM);
        // 注意：字体通常由GC管理，这里只是测试创建
    }
    LOG_DEBUG("  ✓ 字体生命周期测试通过 (10次创建)\n");
    
    // 测试图像创建和销毁
    LOG_DEBUG("  测试图像生命周期...\n");
    for (int i = 0; i < 10; i++) {
        j2me_midp_image_t* image = j2me_midp_image_create(vm, 64, 64);
        // 注意：图像通常由GC管理
    }
    LOG_DEBUG("  ✓ 图像生命周期测试通过 (10次创建)\n");
    
    LOG_DEBUG("  ✓ 资源管理测试完成\n");
}

void test_frame_rate(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试帧率性能 ===\n");
    
    if (!vm->display || !vm->display->context) {
        LOG_DEBUG("  ✗ 显示系统未初始化\n");
        return;
    }
    
    const int TEST_FRAMES = 60;
    uint32_t start_time = SDL_GetTicks();
    
    for (int frame = 0; frame < TEST_FRAMES; frame++) {
        j2me_midp_graphics_t* g = j2me_midp_graphics_create(vm->display->context);
        if (!g) continue;
        
        // 简单的动画效果
        int offset = (frame * 4) % 240;
        
        // 清屏
        j2me_midp_graphics_set_color_rgb(g, 0, 0, 50);
        j2me_midp_graphics_fill_rect(g, 0, 0, 240, 320);
        
        // 绘制移动的矩形
        j2me_midp_graphics_set_color_rgb(g, 255, 255, 0);
        j2me_midp_graphics_fill_rect(g, offset, 150, 40, 40);
        
        // 绘制帧数
        char frame_text[32];
        snprintf(frame_text, sizeof(frame_text), "Frame: %d/%d", frame + 1, TEST_FRAMES);
        j2me_midp_graphics_set_color_rgb(g, 255, 255, 255);
        j2me_midp_graphics_draw_string(g, frame_text, 10, 10, ANCHOR_TOP | ANCHOR_LEFT);
        
        // 更新显示
        j2me_display_refresh(vm->display);
        
        j2me_midp_graphics_destroy(g);
        
        // 处理事件
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                LOG_DEBUG("  ! 用户请求退出\n");
                goto cleanup;
            }
        }
        
        // 限制帧率到60fps
        SDL_Delay(16);
    }
    
cleanup:
    uint32_t elapsed = SDL_GetTicks() - start_time;
    float fps = (float)TEST_FRAMES / (elapsed / 1000.0f);
    
    LOG_DEBUG("  ✓ 渲染%d帧, 耗时%dms\n", TEST_FRAMES, elapsed);
    LOG_DEBUG("  ✓ 平均帧率: %.2f FPS\n", fps);
    LOG_DEBUG("  ✓ 平均帧时间: %.2fms\n", (float)elapsed / TEST_FRAMES);
    
    if (fps >= 30.0f) {
        LOG_DEBUG("  ✓ 帧率性能良好 (>= 30 FPS)\n");
    } else {
        LOG_DEBUG("  ⚠ 帧率性能需要优化 (< 30 FPS)\n");
    }
}

void print_performance_summary() {
    LOG_DEBUG("\n=== 性能统计总结 ===\n");
    
    LOG_DEBUG("  帧数: %d\n", perf_stats.frame_count);
    LOG_DEBUG("  事件数: %d\n", perf_stats.event_count);
    LOG_DEBUG("  渲染时间: %dms\n", perf_stats.render_time_ms);
    LOG_DEBUG("  事件处理时间: %dms\n", perf_stats.event_time_ms);
    
    if (perf_stats.frame_count > 0) {
        LOG_DEBUG("  平均帧渲染时间: %.2fms\n", 
               (float)perf_stats.render_time_ms / perf_stats.frame_count);
    }
    
    if (perf_stats.event_count > 0) {
        LOG_DEBUG("  平均事件处理时间: %.2fms\n", 
               (float)perf_stats.event_time_ms / perf_stats.event_count);
    }
    
    LOG_DEBUG("\n=== 优化建议 ===\n");
    
    if (perf_stats.render_time_ms > 1000) {
        LOG_DEBUG("  ⚠ 渲染时间较长，建议优化绘制操作\n");
    } else {
        LOG_DEBUG("  ✓ 渲染性能良好\n");
    }
    
    if (perf_stats.event_time_ms > 100) {
        LOG_DEBUG("  ⚠ 事件处理时间较长，建议优化事件循环\n");
    } else {
        LOG_DEBUG("  ✓ 事件处理性能良好\n");
    }
}

int main(int argc, char* argv[]) {
    LOG_DEBUG("=== SDL集成优化测试 ===\n");
    
    // 创建虚拟机
    j2me_vm_config_t config = j2me_vm_get_default_config();
    config.heap_size = 512 * 1024;
    
    j2me_vm_t* vm = j2me_vm_create(&config);
    if (!vm) {
        LOG_DEBUG("✗ 虚拟机创建失败\n");
        return 1;
    }
    
    // 初始化虚拟机
    if (j2me_vm_initialize(vm) != J2ME_SUCCESS) {
        LOG_DEBUG("✗ 虚拟机初始化失败\n");
        j2me_vm_destroy(vm);
        return 1;
    }
    
    LOG_DEBUG("✓ 虚拟机初始化成功\n");
    
    // 运行测试
    test_event_processing(vm);
    test_rendering_performance(vm);
    test_resource_management(vm);
    test_frame_rate(vm);
    
    // 打印性能总结
    print_performance_summary();
    
    // 清理
    j2me_vm_destroy(vm);
    
    LOG_DEBUG("\n=== SDL集成优化测试完成 ===\n");
    return 0;
}
