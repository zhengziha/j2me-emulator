#include "j2me_graphics.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

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
    context->current_font = (j2me_font_t){12, 0, "Arial"}; // 默认字体
    context->clip_x = 0;
    context->clip_y = 0;
    context->clip_width = width;
    context->clip_height = height;
    context->clipping_enabled = false;
    context->translate_x = 0;
    context->translate_y = 0;
    
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

void j2me_graphics_draw_oval(j2me_graphics_context_t* context, int x, int y, int width, int height, bool filled) {
    if (!context || !context->renderer) {
        return;
    }
    
    // 应用坐标变换
    x += context->translate_x;
    y += context->translate_y;
    
    // 简化的椭圆绘制：使用多个直线近似
    int cx = x + width / 2;
    int cy = y + height / 2;
    int rx = width / 2;
    int ry = height / 2;
    
    if (filled) {
        // 填充椭圆：绘制多条水平线
        for (int dy = -ry; dy <= ry; dy++) {
            int dx = (int)(rx * sqrt(1.0 - (double)(dy * dy) / (ry * ry)));
            SDL_RenderDrawLine(context->renderer, cx - dx, cy + dy, cx + dx, cy + dy);
        }
    } else {
        // 椭圆轮廓：使用参数方程绘制点
        for (int angle = 0; angle < 360; angle += 2) {
            double rad = angle * M_PI / 180.0;
            int px = cx + (int)(rx * cos(rad));
            int py = cy + (int)(ry * sin(rad));
            SDL_RenderDrawPoint(context->renderer, px, py);
        }
    }
}

void j2me_graphics_draw_arc(j2me_graphics_context_t* context, int x, int y, int width, int height, 
                           int start_angle, int arc_angle, bool filled) {
    if (!context || !context->renderer) {
        return;
    }
    
    // 应用坐标变换
    x += context->translate_x;
    y += context->translate_y;
    
    int cx = x + width / 2;
    int cy = y + height / 2;
    int rx = width / 2;
    int ry = height / 2;
    
    int end_angle = start_angle + arc_angle;
    
    if (filled) {
        // 填充扇形：从中心点绘制到弧上的点
        for (int angle = start_angle; angle < end_angle; angle += 2) {
            double rad = angle * M_PI / 180.0;
            int px = cx + (int)(rx * cos(rad));
            int py = cy + (int)(ry * sin(rad));
            SDL_RenderDrawLine(context->renderer, cx, cy, px, py);
        }
    } else {
        // 弧线：只绘制弧的轮廓
        for (int angle = start_angle; angle < end_angle; angle += 2) {
            double rad = angle * M_PI / 180.0;
            int px = cx + (int)(rx * cos(rad));
            int py = cy + (int)(ry * sin(rad));
            SDL_RenderDrawPoint(context->renderer, px, py);
        }
    }
}

void j2me_graphics_draw_polygon(j2me_graphics_context_t* context, int* x_points, int* y_points, 
                               int num_points, bool filled) {
    if (!context || !context->renderer || !x_points || !y_points || num_points < 3) {
        return;
    }
    
    // 应用坐标变换
    int* transformed_x = malloc(num_points * sizeof(int));
    int* transformed_y = malloc(num_points * sizeof(int));
    
    for (int i = 0; i < num_points; i++) {
        transformed_x[i] = x_points[i] + context->translate_x;
        transformed_y[i] = y_points[i] + context->translate_y;
    }
    
    if (filled) {
        // 简化的多边形填充：使用扫描线算法的简化版本
        // 这里使用一个简单的方法：绘制从多边形中心到各顶点的三角形
        
        // 计算多边形中心
        int cx = 0, cy = 0;
        for (int i = 0; i < num_points; i++) {
            cx += transformed_x[i];
            cy += transformed_y[i];
        }
        cx /= num_points;
        cy /= num_points;
        
        // 绘制三角形扇形
        for (int i = 0; i < num_points; i++) {
            int next = (i + 1) % num_points;
            
            // 绘制三角形的三条边
            SDL_RenderDrawLine(context->renderer, cx, cy, transformed_x[i], transformed_y[i]);
            SDL_RenderDrawLine(context->renderer, transformed_x[i], transformed_y[i], 
                             transformed_x[next], transformed_y[next]);
            SDL_RenderDrawLine(context->renderer, transformed_x[next], transformed_y[next], cx, cy);
        }
    } else {
        // 多边形轮廓：连接各个顶点
        for (int i = 0; i < num_points; i++) {
            int next = (i + 1) % num_points;
            SDL_RenderDrawLine(context->renderer, transformed_x[i], transformed_y[i], 
                             transformed_x[next], transformed_y[next]);
        }
    }
    
    free(transformed_x);
    free(transformed_y);
}

void j2me_graphics_draw_string(j2me_graphics_context_t* context, const char* text, 
                              int x, int y, int anchor) {
    if (!context || !context->renderer || !text) {
        return;
    }
    
    // 应用坐标变换
    x += context->translate_x;
    y += context->translate_y;
    
    // 简化的文本渲染：每个字符用一个小矩形表示
    int char_width = context->current_font.size * 0.6; // 字符宽度约为字体大小的60%
    int char_height = context->current_font.size;
    
    // 处理锚点
    int text_width = strlen(text) * char_width;
    int text_height = char_height;
    
    // 根据锚点调整位置
    if (anchor & 0x01) { // RIGHT
        x -= text_width;
    } else if (anchor & 0x02) { // HCENTER
        x -= text_width / 2;
    }
    
    if (anchor & 0x10) { // BOTTOM
        y -= text_height;
    } else if (anchor & 0x20) { // VCENTER
        y -= text_height / 2;
    }
    
    // 绘制每个字符
    for (int i = 0; text[i] != '\0'; i++) {
        int char_x = x + i * char_width;
        int char_y = y;
        
        // 绘制字符边框
        SDL_Rect char_rect = {char_x, char_y, char_width - 1, char_height - 1};
        SDL_RenderDrawRect(context->renderer, &char_rect);
        
        // 在字符中心绘制一个点表示字符内容
        SDL_RenderDrawPoint(context->renderer, char_x + char_width/2, char_y + char_height/2);
    }
}

void j2me_graphics_set_font(j2me_graphics_context_t* context, j2me_font_t font) {
    if (!context) {
        return;
    }
    
    context->current_font = font;
}

int j2me_graphics_get_string_width(j2me_graphics_context_t* context, const char* text) {
    if (!context || !text) {
        return 0;
    }
    
    int char_width = context->current_font.size * 0.6;
    return strlen(text) * char_width;
}

int j2me_graphics_get_font_height(j2me_graphics_context_t* context) {
    if (!context) {
        return 0;
    }
    
    return context->current_font.size;
}

void j2me_graphics_translate(j2me_graphics_context_t* context, int x, int y) {
    if (!context) {
        return;
    }
    
    context->translate_x += x;
    context->translate_y += y;
}

j2me_image_t* j2me_image_create(int width, int height) {
    j2me_image_t* image = malloc(sizeof(j2me_image_t));
    if (!image) {
        return NULL;
    }
    
    // 这里需要一个渲染器来创建纹理，暂时返回NULL
    // 在实际实现中，需要传入渲染器参数
    image->texture = NULL;
    image->width = width;
    image->height = height;
    image->mutable = true;
    
    return image;
}

j2me_image_t* j2me_image_load(const char* filename) {
    if (!filename) {
        return NULL;
    }
    
    // 简化实现：创建一个占位符图像
    j2me_image_t* image = malloc(sizeof(j2me_image_t));
    if (!image) {
        return NULL;
    }
    
    image->texture = NULL;
    image->width = 32;  // 默认尺寸
    image->height = 32;
    image->mutable = false;
    
    printf("[图形] 图像加载占位符: %s (%dx%d)\n", filename, image->width, image->height);
    return image;
}

void j2me_image_destroy(j2me_image_t* image) {
    if (!image) {
        return;
    }
    
    if (image->texture) {
        SDL_DestroyTexture(image->texture);
    }
    
    free(image);
}

void j2me_graphics_draw_image(j2me_graphics_context_t* context, j2me_image_t* image, 
                             int x, int y, int anchor) {
    if (!context || !context->renderer || !image) {
        return;
    }
    
    // 应用坐标变换
    x += context->translate_x;
    y += context->translate_y;
    
    // 处理锚点
    if (anchor & 0x01) { // RIGHT
        x -= image->width;
    } else if (anchor & 0x02) { // HCENTER
        x -= image->width / 2;
    }
    
    if (anchor & 0x10) { // BOTTOM
        y -= image->height;
    } else if (anchor & 0x20) { // VCENTER
        y -= image->height / 2;
    }
    
    if (image->texture) {
        // 如果有实际纹理，绘制纹理
        SDL_Rect dst_rect = {x, y, image->width, image->height};
        SDL_RenderCopy(context->renderer, image->texture, NULL, &dst_rect);
    } else {
        // 占位符：绘制一个矩形表示图像
        SDL_Rect image_rect = {x, y, image->width, image->height};
        SDL_RenderDrawRect(context->renderer, &image_rect);
        
        // 绘制对角线表示这是一个图像
        SDL_RenderDrawLine(context->renderer, x, y, x + image->width, y + image->height);
        SDL_RenderDrawLine(context->renderer, x + image->width, y, x, y + image->height);
    }
}