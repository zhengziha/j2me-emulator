#include <SDL2/SDL.h>
#include "j2me_log.h"
#include <stdio.h>
#include <stdbool.h>

/**
 * @file sdl_direct_test.c
 * @brief 最简单的SDL渲染测试 - 直接在窗口上绘制
 */

int main(void) {
    LOG_DEBUG("=== SDL直接渲染测试 ===\n\n");
    
    // 初始化SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        LOG_DEBUG("SDL初始化失败: %s\n", SDL_GetError());
        return 1;
    }
    LOG_DEBUG("✓ SDL初始化成功\n");
    
    // 创建窗口
    SDL_Window* window = SDL_CreateWindow(
        "SDL Direct Test - 应该看到4个彩色矩形",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        480, 640,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        LOG_DEBUG("窗口创建失败: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    LOG_DEBUG("✓ 窗口创建成功 (480x640)\n");
    
    // 创建渲染器
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (!renderer) {
        LOG_DEBUG("渲染器创建失败: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    LOG_DEBUG("✓ 渲染器创建成功\n\n");
    
    // 清空屏幕（黑色）
    LOG_DEBUG("绘制黑色背景...\n");
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    // 绘制红色矩形
    LOG_DEBUG("绘制红色矩形 (20, 20, 200, 200)...\n");
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_Rect red_rect = {20, 20, 200, 200};
    SDL_RenderFillRect(renderer, &red_rect);
    
    // 绘制绿色矩形
    LOG_DEBUG("绘制绿色矩形 (240, 20, 200, 200)...\n");
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_Rect green_rect = {240, 20, 200, 200};
    SDL_RenderFillRect(renderer, &green_rect);
    
    // 绘制蓝色矩形
    LOG_DEBUG("绘制蓝色矩形 (20, 240, 200, 200)...\n");
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    SDL_Rect blue_rect = {20, 240, 200, 200};
    SDL_RenderFillRect(renderer, &blue_rect);
    
    // 绘制黄色矩形
    LOG_DEBUG("绘制黄色矩形 (240, 240, 200, 200)...\n");
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    SDL_Rect yellow_rect = {240, 240, 200, 200};
    SDL_RenderFillRect(renderer, &yellow_rect);
    
    // 呈现到屏幕
    LOG_DEBUG("呈现到屏幕...\n");
    SDL_RenderPresent(renderer);
    LOG_DEBUG("✓ 渲染完成\n\n");
    
    LOG_DEBUG("窗口将显示5秒，应该看到4个大的彩色矩形\n");
    LOG_DEBUG("按任意键或点击关闭窗口\n\n");
    
    // 等待5秒
    bool running = true;
    SDL_Event event;
    for (int i = 0; i < 50 && running; i++) {
        if (i % 10 == 0) {
            LOG_DEBUG("  %d秒...\n", 5 - i/10);
        }
        
        SDL_Delay(100);
        
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || 
                event.type == SDL_KEYDOWN || 
                event.type == SDL_MOUSEBUTTONDOWN) {
                LOG_DEBUG("用户关闭窗口\n");
                running = false;
                break;
            }
        }
    }
    
    // 清理
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    LOG_DEBUG("\n测试完成！\n");
    return 0;
}
