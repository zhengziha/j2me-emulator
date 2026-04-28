#include "j2me_log.h"
#include <stdio.h>
#include <SDL2/SDL.h>
#include "j2me_graphics.h"

int main(int argc, char* argv[]) {
    LOG_DEBUG("=== Graphics绘制测试 ===\n");
    
    // 初始化SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        LOG_DEBUG("SDL初始化失败: %s\n", SDL_GetError());
        return 1;
    }
    
    // 创建显示系统
    j2me_display_t* display = j2me_display_initialize(240, 320, "Graphics测试");
    if (!display) {
        LOG_DEBUG("显示系统初始化失败\n");
        SDL_Quit();
        return 1;
    }
    
    // 创建图形上下文
    j2me_graphics_context_t* graphics = j2me_graphics_create_context(display, 240, 320);
    if (!graphics) {
        LOG_DEBUG("图形上下文创建失败\n");
        j2me_display_destroy(display);
        SDL_Quit();
        return 1;
    }
    
    LOG_DEBUG("开始绘制测试图形...\n");
    
    // 设置渲染目标为画布
    SDL_SetRenderTarget(display->context->renderer, display->context->canvas);
    
    // 清除画布为白色背景
    SDL_SetRenderDrawColor(display->context->renderer, 255, 255, 255, 255);
    SDL_RenderClear(display->context->renderer);
    
    // 绘制红色矩形
    j2me_color_t red = {255, 0, 0, 255};
    j2me_graphics_set_color(graphics, red);
    j2me_graphics_draw_rect(graphics, 10, 10, 100, 50, true);
    LOG_DEBUG("绘制红色矩形: (10, 10) 100x50\n");
    
    // 绘制绿色线条
    j2me_color_t green = {0, 255, 0, 255};
    j2me_graphics_set_color(graphics, green);
    j2me_graphics_draw_line(graphics, 0, 0, 239, 319);
    j2me_graphics_draw_line(graphics, 239, 0, 0, 319);
    LOG_DEBUG("绘制绿色线条: (0,0)->(239,319) 和 (239,0)->(0,319)\n");
    
    // 绘制蓝色边框
    j2me_color_t blue = {0, 0, 255, 255};
    j2me_graphics_set_color(graphics, blue);
    j2me_graphics_draw_rect(graphics, 5, 5, 230, 310, false);
    LOG_DEBUG("绘制蓝色边框: (5, 5) 230x310\n");
    
    // 恢复渲染目标为屏幕
    SDL_SetRenderTarget(display->context->renderer, NULL);
    
    // 清除屏幕
    SDL_SetRenderDrawColor(display->context->renderer, 0, 0, 0, 255);
    SDL_RenderClear(display->context->renderer);
    
    // 将画布内容复制到屏幕
    SDL_Rect dest_rect = {0, 0, 240, 320};
    SDL_RenderCopy(display->context->renderer, display->context->canvas, NULL, &dest_rect);
    
    // 刷新显示
    j2me_display_refresh(display);
    
    LOG_DEBUG("绘制完成！按任意键退出...\n");
    
    // 等待用户按键
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || 
                (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                running = false;
            }
        }
        SDL_Delay(100);
    }
    
    // 清理资源
    j2me_graphics_destroy_context(graphics);
    j2me_display_destroy(display);
    SDL_Quit();
    
    LOG_DEBUG("测试完成！\n");
    return 0;
}