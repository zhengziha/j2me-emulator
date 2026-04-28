#include <SDL2/SDL.h>
#include "j2me_log.h"
#include <stdio.h>
#include <stdbool.h>

/**
 * @file canvas_texture_test.c
 * @brief Canvas Texture渲染测试
 * 
 * 测试渲染到texture然后复制到屏幕的流程
 */

int main() {
    LOG_DEBUG("===========================================\n");
    LOG_DEBUG("  Canvas Texture渲染测试\n");
    LOG_DEBUG("===========================================\n");
    LOG_DEBUG("\n");
    
    // 初始化SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        LOG_DEBUG("✗ SDL初始化失败: %s\n", SDL_GetError());
        return 1;
    }
    LOG_DEBUG("✓ SDL初始化成功\n");
    
    // 创建窗口
    SDL_Window* window = SDL_CreateWindow(
        "Canvas Texture测试",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        240, 320,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        LOG_DEBUG("✗ 窗口创建失败: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    LOG_DEBUG("✓ 窗口创建成功\n");
    
    SDL_RaiseWindow(window);
    
    // 创建渲染器
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (!renderer) {
        LOG_DEBUG("✗ 渲染器创建失败: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    LOG_DEBUG("✓ 渲染器创建成功\n");
    
    // 创建canvas texture（与J2ME相同的参数）
    SDL_Texture* canvas = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        240, 320
    );
    
    if (!canvas) {
        LOG_DEBUG("✗ Canvas纹理创建失败: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    LOG_DEBUG("✓ Canvas纹理创建成功\n");
    
    // 查询纹理信息
    Uint32 format;
    int access, w, h;
    SDL_QueryTexture(canvas, &format, &access, &w, &h);
    LOG_DEBUG("  纹理信息: 格式=%u, 访问模式=%d, 尺寸=%dx%d\n", format, access, w, h);
    
    if (access != SDL_TEXTUREACCESS_TARGET) {
        LOG_DEBUG("  ⚠ 警告: 纹理访问模式不是TARGET (%d)\n", access);
    }
    
    LOG_DEBUG("\n开始渲染测试...\n");
    LOG_DEBUG("窗口应该显示：\n");
    LOG_DEBUG("  - 深蓝色背景\n");
    LOG_DEBUG("  - 黄色移动矩形\n");
    LOG_DEBUG("  - 红色移动矩形（反向）\n");
    LOG_DEBUG("  - 绿色边框\n");
    LOG_DEBUG("\n");
    LOG_DEBUG("你看到了什么？\n");
    LOG_DEBUG("  1 = 看到所有内容  |  0 = 只看到黑色  |  2 = 看到部分内容\n");
    LOG_DEBUG("按键反馈: ");
    fflush(stdout);
    
    bool running = true;
    int frame_count = 0;
    bool feedback_received = false;
    int feedback = -1;
    
    while (running && frame_count < 300) {
        // 处理事件
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                SDL_Keycode key = event.key.keysym.sym;
                if (key == SDLK_ESCAPE) {
                    running = false;
                } else if (!feedback_received && (key == SDLK_1 || key == SDLK_0 || key == SDLK_2)) {
                    feedback_received = true;
                    if (key == SDLK_1) {
                        feedback = 1;
                        LOG_DEBUG("1 (看到所有内容)\n");
                    } else if (key == SDLK_0) {
                        feedback = 0;
                        LOG_DEBUG("0 (只看到黑色)\n");
                    } else if (key == SDLK_2) {
                        feedback = 2;
                        LOG_DEBUG("2 (看到部分内容)\n");
                    }
                }
            }
        }
        
        // === 关键：渲染到canvas texture ===
        int result = SDL_SetRenderTarget(renderer, canvas);
        if (result != 0 && frame_count == 0) {
            LOG_DEBUG("\n✗ SDL_SetRenderTarget失败: %s\n", SDL_GetError());
        }
        
        // 验证渲染目标
        if (frame_count == 0) {
            SDL_Texture* current_target = SDL_GetRenderTarget(renderer);
            LOG_DEBUG("\n渲染目标验证:\n");
            LOG_DEBUG("  Canvas地址: %p\n", (void*)canvas);
            LOG_DEBUG("  当前目标: %p\n", (void*)current_target);
            if (current_target == NULL) {
                LOG_DEBUG("  ✗ 错误: 渲染目标是屏幕（NULL）\n");
            } else {
                LOG_DEBUG("  ✓ 渲染目标已设置\n");
            }
        }
        
        // 清屏 - 深蓝色背景
        SDL_SetRenderDrawColor(renderer, 0, 0, 50, 255);
        SDL_RenderClear(renderer);
        
        // 绘制黄色移动矩形
        int x1 = (frame_count % 200);
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_Rect rect1 = {x1, 150, 40, 40};
        SDL_RenderFillRect(renderer, &rect1);
        
        // 绘制红色移动矩形（反向）
        int x2 = 200 - (frame_count % 200);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_Rect rect2 = {x2, 100, 30, 30};
        SDL_RenderFillRect(renderer, &rect2);
        
        // 绘制绿色边框
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_Rect border = {5, 5, 230, 310};
        SDL_RenderDrawRect(renderer, &border);
        
        // === 关键：恢复渲染目标为屏幕 ===
        SDL_SetRenderTarget(renderer, NULL);
        
        // === 关键：清除屏幕并复制canvas到屏幕 ===
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        SDL_Rect dest_rect = {0, 0, 240, 320};
        int copy_result = SDL_RenderCopy(renderer, canvas, NULL, &dest_rect);
        
        if (copy_result != 0 && frame_count == 0) {
            LOG_DEBUG("✗ SDL_RenderCopy失败: %s\n", SDL_GetError());
        }
        
        // === 关键：显示到屏幕 ===
        SDL_RenderPresent(renderer);
        
        frame_count++;
        SDL_Delay(16);
        
        // 每60帧打印一次状态
        if (frame_count % 60 == 0) {
            LOG_DEBUG("  [帧: %d] 渲染中...\n", frame_count);
        }
    }
    
    LOG_DEBUG("\n");
    LOG_DEBUG("===========================================\n");
    LOG_DEBUG("  测试结果\n");
    LOG_DEBUG("===========================================\n");
    LOG_DEBUG("\n");
    
    if (feedback == 1) {
        LOG_DEBUG("✓ Canvas texture渲染正常！\n");
        LOG_DEBUG("  你看到了所有预期的内容。\n");
        LOG_DEBUG("  这说明canvas texture渲染流程是正确的。\n");
        LOG_DEBUG("\n");
        LOG_DEBUG("  问题可能在于：\n");
        LOG_DEBUG("  1. J2ME代码中渲染目标设置的时机\n");
        LOG_DEBUG("  2. MIDP Graphics API的实现\n");
        LOG_DEBUG("  3. 渲染内容的坐标或颜色\n");
    } else if (feedback == 0) {
        LOG_DEBUG("✗ Canvas texture渲染失败！\n");
        LOG_DEBUG("  你只看到了黑色屏幕。\n");
        LOG_DEBUG("  这说明canvas texture渲染流程有问题。\n");
        LOG_DEBUG("\n");
        LOG_DEBUG("  可能原因：\n");
        LOG_DEBUG("  1. SDL_SetRenderTarget失败\n");
        LOG_DEBUG("  2. SDL_RenderCopy失败\n");
        LOG_DEBUG("  3. Canvas texture格式不正确\n");
        LOG_DEBUG("  4. 渲染器不支持texture target\n");
    } else if (feedback == 2) {
        LOG_DEBUG("⚠ Canvas texture部分工作！\n");
        LOG_DEBUG("  你看到了部分内容。\n");
        LOG_DEBUG("  这说明渲染流程基本正确，但有些问题。\n");
        LOG_DEBUG("\n");
        LOG_DEBUG("  可能原因：\n");
        LOG_DEBUG("  1. 颜色混合问题\n");
        LOG_DEBUG("  2. 部分渲染操作失败\n");
        LOG_DEBUG("  3. 坐标系统问题\n");
    } else {
        LOG_DEBUG("未收到反馈\n");
    }
    
    LOG_DEBUG("\n清理资源...\n");
    SDL_DestroyTexture(canvas);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    LOG_DEBUG("✓ 测试完成\n");
    LOG_DEBUG("\n");
    
    return 0;
}
