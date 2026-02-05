#include "j2me_graphics.h"
#include <stdlib.h>
#include <stdio.h>

/**
 * @file j2me_graphics.c
 * @brief J2ME图形系统实现
 * 
 * 基于SDL2的高性能图形渲染实现
 */

j2me_display_t* j2me_display_initialize(int width, int height, const char* title) {
    // 初始化SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("[图形] SDL初始化失败: %s\n", SDL_GetError());
        return NULL;
    }
    
    j2me_display_t* display = (j2me_display_t*)malloc(sizeof(j2me_display_t));
    if (!display) {
        SDL_Quit();
        return NULL;
    }
    
    // 创建窗口
    display->window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!display->window) {
        printf("[图形] 窗口创建失败: %s\n", SDL_GetError());
        free(display);
        SDL_Quit();
        return NULL;
    }
    
    // 创建渲染器
    display->renderer = SDL_CreateRenderer(
        display->window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (!display->renderer) {
        printf("[图形] 渲染器创建失败: %s\n", SDL_GetError());
        SDL_DestroyWindow(display->window);
        free(display);
        SDL_Quit();
        return NULL;
    }
    
    display->screen_width = width;
    display->screen_height = height;
    display->fullscreen = false;
    display->context = NULL;
    
    // 设置渲染器混合模式
    SDL_SetRenderDrawBlendMode(display->renderer, SDL_BLENDMODE_BLEND);
    
    printf("[图形] 显示系统初始化成功 (%dx%d)\n", width, height);
    return display;
}

void j2me_display_destroy(j2me_display_t* display) {
    if (!display) {
        return;
    }
    
    if (display->context) {
        j2me_graphics_destroy_context(display->context);
    }
    
    if (display->renderer) {
        SDL_DestroyRenderer(display->renderer);
    }
    
    if (display->window) {
        SDL_DestroyWindow(display->window);
    }
    
    free(display);
    SDL_Quit();
    printf("[图形] 显示系统已销毁\n");
}

j2me_graphics_context_t* j2me_graphics_create_context(j2me_display_t* display, int width, int height) {
    if (!display || !display->renderer) {
        return NULL;
    }
    
    j2me_graphics_context_t* context = (j2me_graphics_context_t*)malloc(sizeof(j2me_graphics_context_t));
    if (!context) {
        return NULL;
    }
    
    context->renderer = display->renderer;
    context->width = width;
    context->height = height;
    
    // 创建画布纹理
    context->canvas = SDL_CreateTexture(
        display->renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        width, height
    );
    
    if (!context->canvas) {
        printf("[图形] 画布纹理创建失败: %s\n", SDL_GetError());
        free(context);
        return NULL;
    }
    
    // 初始化默认值
    context->current_color = (j2me_color_t){0, 0, 0, 255}; // 黑色
    context->clip_x = 0;
    context->clip_y = 0;
    context->clip_width = width;
    context->clip_height = height;
    context->clipping_enabled = false;
    
    display->context = context;
    
    printf("[图形] 图形上下文创建成功 (%dx%d)\n", width, height);
    return context;
}

void j2me_graphics_destroy_context(j2me_graphics_context_t* context) {
    if (!context) {
        return;
    }
    
    if (context->canvas) {
        SDL_DestroyTexture(context->canvas);
    }
    
    free(context);
}

void j2me_graphics_set_color(j2me_graphics_context_t* context, j2me_color_t color) {
    if (!context) {
        return;
    }
    
    context->current_color = color;
    SDL_SetRenderDrawColor(context->renderer, color.r, color.g, color.b, color.a);
}

void j2me_graphics_draw_pixel(j2me_graphics_context_t* context, int x, int y) {
    if (!context || !context->renderer) {
        return;
    }
    
    // 检查裁剪区域
    if (context->clipping_enabled) {
        if (x < context->clip_x || x >= context->clip_x + context->clip_width ||
            y < context->clip_y || y >= context->clip_y + context->clip_height) {
            return;
        }
    }
    
    SDL_RenderDrawPoint(context->renderer, x, y);
}

void j2me_graphics_draw_line(j2me_graphics_context_t* context, int x1, int y1, int x2, int y2) {
    if (!context || !context->renderer) {
        return;
    }
    
    SDL_RenderDrawLine(context->renderer, x1, y1, x2, y2);
}

void j2me_graphics_draw_rect(j2me_graphics_context_t* context, int x, int y, int width, int height, bool filled) {
    if (!context || !context->renderer) {
        return;
    }
    
    SDL_Rect rect = {x, y, width, height};
    
    if (filled) {
        SDL_RenderFillRect(context->renderer, &rect);
    } else {
        SDL_RenderDrawRect(context->renderer, &rect);
    }
}

void j2me_graphics_set_clip(j2me_graphics_context_t* context, int x, int y, int width, int height) {
    if (!context) {
        return;
    }
    
    context->clip_x = x;
    context->clip_y = y;
    context->clip_width = width;
    context->clip_height = height;
    context->clipping_enabled = true;
    
    // 设置SDL裁剪区域
    SDL_Rect clip_rect = {x, y, width, height};
    SDL_RenderSetClipRect(context->renderer, &clip_rect);
}

void j2me_graphics_clear(j2me_graphics_context_t* context) {
    if (!context || !context->renderer) {
        return;
    }
    
    // 保存当前颜色
    j2me_color_t saved_color = context->current_color;
    
    // 设置为白色并清除
    SDL_SetRenderDrawColor(context->renderer, 255, 255, 255, 255);
    SDL_RenderClear(context->renderer);
    
    // 恢复原来的颜色
    SDL_SetRenderDrawColor(context->renderer, saved_color.r, saved_color.g, saved_color.b, saved_color.a);
}

void j2me_display_refresh(j2me_display_t* display) {
    if (!display || !display->renderer) {
        return;
    }
    
    SDL_RenderPresent(display->renderer);
}