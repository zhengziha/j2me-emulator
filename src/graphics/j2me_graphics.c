#include "j2me_graphics.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

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
    
    // 初始化SDL2_image
    int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if (!(IMG_Init(img_flags) & img_flags)) {
        printf("[图形] SDL2_image初始化失败: %s\n", IMG_GetError());
        SDL_Quit();
        return NULL;
    }
    
    // 初始化SDL2_ttf
    if (TTF_Init() == -1) {
        printf("[图形] SDL2_ttf初始化失败: %s\n", TTF_GetError());
        IMG_Quit();
        SDL_Quit();
        return NULL;
    }
    
    printf("[图形] SDL2_image和SDL2_ttf初始化成功\n");
    
    j2me_display_t* display = (j2me_display_t*)malloc(sizeof(j2me_display_t));
    if (!display) {
        TTF_Quit();
        IMG_Quit();
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
    TTF_Quit();
    IMG_Quit();
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
    
    // 初始化画布为白色背景
    SDL_SetRenderTarget(display->renderer, context->canvas);
    SDL_SetRenderDrawColor(display->renderer, 255, 255, 255, 255);
    SDL_RenderClear(display->renderer);
    SDL_SetRenderTarget(display->renderer, NULL);
    
    // 初始化默认值
    context->current_color = (j2me_color_t){0, 0, 0, 255}; // 黑色
    
    // 初始化默认字体
    context->current_font = (j2me_font_t){12, 0, "Arial", NULL}; // 默认字体
    j2me_graphics_load_default_font(context);
    
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
    
    // 释放字体资源
    if (context->current_font.ttf_font) {
        TTF_CloseFont(context->current_font.ttf_font);
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
    
    // 如果有TTF字体，使用真实的文本渲染
    if (context->current_font.ttf_font) {
        j2me_graphics_render_ttf_text(context, text, x, y, anchor);
        return;
    }
    
    // 回退到简化的文本渲染
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
    
    // 释放旧字体
    if (context->current_font.ttf_font) {
        TTF_CloseFont(context->current_font.ttf_font);
    }
    
    context->current_font = font;
    
    // 如果新字体没有TTF对象，尝试加载
    if (!context->current_font.ttf_font) {
        j2me_graphics_load_font(context, font.name, font.size, font.style);
    }
}

int j2me_graphics_get_string_width(j2me_graphics_context_t* context, const char* text) {
    if (!context || !text) {
        return 0;
    }
    
    // 如果有TTF字体，使用真实的文本度量
    if (context->current_font.ttf_font) {
        int width, height;
        // 首先尝试UTF-8度量（支持中文）
        if (TTF_SizeUTF8(context->current_font.ttf_font, text, &width, &height) == 0) {
            return width;
        }
        // 回退到普通度量
        if (TTF_SizeText(context->current_font.ttf_font, text, &width, &height) == 0) {
            return width;
        }
    }
    
    // 回退到简化计算
    int char_width = context->current_font.size * 0.6;
    return strlen(text) * char_width;
}

int j2me_graphics_get_font_height(j2me_graphics_context_t* context) {
    if (!context) {
        return 0;
    }
    
    // 如果有TTF字体，使用真实的字体高度
    if (context->current_font.ttf_font) {
        return TTF_FontHeight(context->current_font.ttf_font);
    }
    
    // 回退到简化计算
    return context->current_font.size;
}

void j2me_graphics_translate(j2me_graphics_context_t* context, int x, int y) {
    if (!context) {
        return;
    }
    
    context->translate_x += x;
    context->translate_y += y;
}

j2me_image_t* j2me_image_create(j2me_graphics_context_t* context, int width, int height) {
    if (!context || !context->renderer || width <= 0 || height <= 0) {
        return NULL;
    }
    
    j2me_image_t* image = malloc(sizeof(j2me_image_t));
    if (!image) {
        return NULL;
    }
    
    // 创建可变纹理
    image->texture = SDL_CreateTexture(
        context->renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        width, height
    );
    
    if (!image->texture) {
        printf("[图形] 错误: 创建图像纹理失败: %s\n", SDL_GetError());
        free(image);
        return NULL;
    }
    
    image->width = width;
    image->height = height;
    image->mutable = true;
    
    // 初始化为透明
    SDL_SetRenderTarget(context->renderer, image->texture);
    SDL_SetRenderDrawColor(context->renderer, 0, 0, 0, 0);
    SDL_RenderClear(context->renderer);
    SDL_SetRenderTarget(context->renderer, NULL);
    
    printf("[图形] 创建可变图像: %dx%d\n", width, height);
    return image;
}

j2me_image_t* j2me_image_load(j2me_graphics_context_t* context, const char* filename) {
    if (!context || !context->renderer || !filename) {
        return NULL;
    }
    
    printf("[图形] 开始加载图像: %s\n", filename);
    
    // 使用SDL2_image加载图像
    SDL_Surface* surface = IMG_Load(filename);
    if (!surface) {
        printf("[图形] 错误: 无法加载图像文件 %s: %s\n", filename, IMG_GetError());
        
        // 创建占位符图像
        j2me_image_t* placeholder = j2me_image_create(context, 32, 32);
        if (placeholder) {
            printf("[图形] 创建占位符图像: %s (32x32)\n", filename);
        }
        return placeholder;
    }
    
    printf("[图形] 图像表面加载成功: %dx%d, 格式=%s\n", 
           surface->w, surface->h, SDL_GetPixelFormatName(surface->format->format));
    
    // 创建纹理
    SDL_Texture* texture = SDL_CreateTextureFromSurface(context->renderer, surface);
    if (!texture) {
        printf("[图形] 错误: 创建纹理失败: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return NULL;
    }
    
    // 创建图像对象
    j2me_image_t* image = malloc(sizeof(j2me_image_t));
    if (!image) {
        SDL_DestroyTexture(texture);
        SDL_FreeSurface(surface);
        return NULL;
    }
    
    image->texture = texture;
    image->width = surface->w;
    image->height = surface->h;
    image->mutable = false;
    
    SDL_FreeSurface(surface);
    
    printf("[图形] 图像加载成功: %s (%dx%d)\n", filename, image->width, image->height);
    return image;
}

j2me_image_t* j2me_image_create_from_data(j2me_graphics_context_t* context, 
                                          const uint8_t* data, size_t data_size) {
    if (!context || !context->renderer || !data || data_size == 0) {
        return NULL;
    }
    
    printf("[图形] 从内存数据创建图像，大小: %zu bytes\n", data_size);
    
    // 创建SDL_RWops从内存数据
    SDL_RWops* rw = SDL_RWFromConstMem(data, (int)data_size);
    if (!rw) {
        printf("[图形] 错误: 创建RWops失败: %s\n", SDL_GetError());
        return NULL;
    }
    
    // 使用SDL2_image从内存加载
    SDL_Surface* surface = IMG_Load_RW(rw, 1); // 1表示自动关闭RWops
    if (!surface) {
        printf("[图形] 错误: 从内存加载图像失败: %s\n", IMG_GetError());
        return NULL;
    }
    
    printf("[图形] 从内存加载图像成功: %dx%d\n", surface->w, surface->h);
    
    // 创建纹理
    SDL_Texture* texture = SDL_CreateTextureFromSurface(context->renderer, surface);
    if (!texture) {
        printf("[图形] 错误: 创建纹理失败: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return NULL;
    }
    
    // 创建图像对象
    j2me_image_t* image = malloc(sizeof(j2me_image_t));
    if (!image) {
        SDL_DestroyTexture(texture);
        SDL_FreeSurface(surface);
        return NULL;
    }
    
    image->texture = texture;
    image->width = surface->w;
    image->height = surface->h;
    image->mutable = false;
    
    SDL_FreeSurface(surface);
    
    printf("[图形] 从内存创建图像成功: %dx%d\n", image->width, image->height);
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

/**
 * @brief 加载默认字体
 * @param context 图形上下文
 */
void j2me_graphics_load_default_font(j2me_graphics_context_t* context) {
    if (!context) {
        return;
    }
    
    // 尝试加载系统默认字体，优先使用中文字体
    const char* font_paths[] = {
        // 中文字体 (最高优先级)
        "/System/Library/Fonts/STHeiti Medium.ttc",      // 华文黑体 (中等) - 最佳中文支持
        "/System/Library/Fonts/Hiragino Sans GB.ttc",    // 冬青黑体简体中文 - 优秀中文支持
        "/System/Library/Fonts/STHeiti Light.ttc",       // 华文黑体 (细)
        "/System/Library/Fonts/PingFang.ttc",            // 苹果苹方字体 - 现代中文字体
        "/System/Library/Fonts/STSong.ttc",              // 华文宋体 - 传统中文字体
        "/System/Library/Fonts/CJKSymbolsFallback.ttc",  // CJK符号字体
        // Linux中文字体
        "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",     // 文泉驿微米黑
        "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",       // 文泉驿正黑
        "/usr/share/fonts/truetype/arphic/uming.ttc",         // AR PL UMing
        "/usr/share/fonts/truetype/arphic/ukai.ttc",          // AR PL UKai
        "/usr/share/fonts/truetype/droid/DroidSansFallback.ttf", // Droid Sans Fallback
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc", // Noto Sans CJK
        // Windows中文字体
        "/Windows/Fonts/simsun.ttc",                     // 宋体
        "/Windows/Fonts/simhei.ttf",                     // 黑体
        "/Windows/Fonts/msyh.ttc",                       // 微软雅黑
        "/Windows/Fonts/msyhbd.ttc",                     // 微软雅黑粗体
        // macOS英文字体 (备选，但某些也支持中文)
        "/System/Library/Fonts/HelveticaNeue.ttc",       // 部分支持中文
        "/System/Library/Fonts/Geneva.ttf",
        "/System/Library/Fonts/Menlo.ttc",
        "/System/Library/Fonts/Symbol.ttf",
        "/System/Library/Fonts/AppleSDGothicNeo.ttc",    // 支持亚洲字符
        // Linux英文字体 (备选)
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/TTF/arial.ttf",
        // Windows字体 (备选)
        "/usr/share/fonts/truetype/msttcorefonts/arial.ttf",
        NULL
    };
    
    for (int i = 0; font_paths[i] != NULL; i++) {
        TTF_Font* font = TTF_OpenFont(font_paths[i], context->current_font.size);
        if (font) {
            context->current_font.ttf_font = font;
            strncpy(context->current_font.name, "Default", sizeof(context->current_font.name) - 1);
            printf("[图形] 加载默认字体成功: %s (大小: %d)\n", 
                   font_paths[i], context->current_font.size);
            return;
        }
    }
    
    printf("[图形] 警告: 无法加载TTF字体，将使用简化文本渲染\n");
}

/**
 * @brief 加载指定字体
 * @param context 图形上下文
 * @param font_name 字体名称
 * @param size 字体大小
 * @param style 字体样式
 * @return 是否成功
 */
bool j2me_graphics_load_font(j2me_graphics_context_t* context, const char* font_name, 
                            int size, int style) {
    if (!context || !font_name) {
        return false;
    }
    
    // 构建字体文件路径
    char font_path[256];
    
    // 尝试不同的字体路径
    const char* font_dirs[] = {
        "/System/Library/Fonts/",           // macOS
        "/usr/share/fonts/truetype/dejavu/", // Linux
        "/usr/share/fonts/TTF/",            // Linux
        "/usr/share/fonts/truetype/liberation/", // Linux
        NULL
    };
    
    const char* font_extensions[] = {".ttf", ".ttc", ".otf", NULL};
    
    // 首先尝试直接匹配已知的字体文件，优先中文字体
    const char* known_fonts[] = {
        // 中文字体 (最高优先级)
        "/System/Library/Fonts/STHeiti Medium.ttc",      // 华文黑体 - 最佳中文支持
        "/System/Library/Fonts/Hiragino Sans GB.ttc",    // 冬青黑体简体中文
        "/System/Library/Fonts/STHeiti Light.ttc",       // 华文黑体 (细)
        "/System/Library/Fonts/PingFang.ttc",            // 苹果苹方字体
        "/System/Library/Fonts/STSong.ttc",              // 华文宋体
        "/System/Library/Fonts/CJKSymbolsFallback.ttc",  // CJK符号字体
        // Linux中文字体
        "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",     // 文泉驿微米黑
        "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",       // 文泉驿正黑
        "/usr/share/fonts/truetype/arphic/uming.ttc",         // AR PL UMing
        "/usr/share/fonts/truetype/arphic/ukai.ttc",          // AR PL UKai
        "/usr/share/fonts/truetype/droid/DroidSansFallback.ttf", // Droid Sans Fallback
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc", // Noto Sans CJK
        // Windows中文字体
        "/Windows/Fonts/simsun.ttc",                     // 宋体
        "/Windows/Fonts/simhei.ttf",                     // 黑体
        "/Windows/Fonts/msyh.ttc",                       // 微软雅黑
        "/Windows/Fonts/msyhbd.ttc",                     // 微软雅黑粗体
        // 英文字体 (备选)
        "/System/Library/Fonts/HelveticaNeue.ttc",
        "/System/Library/Fonts/Geneva.ttf", 
        "/System/Library/Fonts/Menlo.ttc",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        NULL
    };
    
    // 先尝试已知字体
    for (int k = 0; known_fonts[k] != NULL; k++) {
        TTF_Font* font = TTF_OpenFont(known_fonts[k], size);
        if (font) {
            // 释放旧字体
            if (context->current_font.ttf_font) {
                TTF_CloseFont(context->current_font.ttf_font);
            }
            
            context->current_font.ttf_font = font;
            context->current_font.size = size;
            context->current_font.style = style;
            strncpy(context->current_font.name, font_name, 
                    sizeof(context->current_font.name) - 1);
            
            // 设置字体样式
            int ttf_style = TTF_STYLE_NORMAL;
            if (style & 0x01) ttf_style |= TTF_STYLE_BOLD;      // BOLD
            if (style & 0x02) ttf_style |= TTF_STYLE_ITALIC;    // ITALIC
            if (style & 0x04) ttf_style |= TTF_STYLE_UNDERLINE; // UNDERLINE
            TTF_SetFontStyle(font, ttf_style);
            
            printf("[图形] 加载已知字体成功: %s (大小: %d, 样式: %d)\n", 
                   known_fonts[k], size, style);
            return true;
        }
    }
    
    for (int i = 0; font_dirs[i] != NULL; i++) {
        for (int j = 0; font_extensions[j] != NULL; j++) {
            snprintf(font_path, sizeof(font_path), "%s%s%s", 
                     font_dirs[i], font_name, font_extensions[j]);
            
            TTF_Font* font = TTF_OpenFont(font_path, size);
            if (font) {
                // 释放旧字体
                if (context->current_font.ttf_font) {
                    TTF_CloseFont(context->current_font.ttf_font);
                }
                
                context->current_font.ttf_font = font;
                context->current_font.size = size;
                context->current_font.style = style;
                strncpy(context->current_font.name, font_name, 
                        sizeof(context->current_font.name) - 1);
                
                // 设置字体样式
                int ttf_style = TTF_STYLE_NORMAL;
                if (style & 0x01) ttf_style |= TTF_STYLE_BOLD;      // BOLD
                if (style & 0x02) ttf_style |= TTF_STYLE_ITALIC;    // ITALIC
                if (style & 0x04) ttf_style |= TTF_STYLE_UNDERLINE; // UNDERLINE
                TTF_SetFontStyle(font, ttf_style);
                
                printf("[图形] 加载字体成功: %s (大小: %d, 样式: %d)\n", 
                       font_path, size, style);
                return true;
            }
        }
    }
    
    printf("[图形] 警告: 无法加载字体 %s，保持当前字体\n", font_name);
    return false;
}

/**
 * @brief 使用TTF字体渲染文本
 * @param context 图形上下文
 * @param text 文本内容
 * @param x X坐标
 * @param y Y坐标
 * @param anchor 锚点
 */
void j2me_graphics_render_ttf_text(j2me_graphics_context_t* context, const char* text, 
                                  int x, int y, int anchor) {
    if (!context || !context->renderer || !text || !context->current_font.ttf_font) {
        return;
    }
    
    // 创建文本表面 - 使用UTF-8渲染函数支持中文
    SDL_Color color = {
        context->current_color.r,
        context->current_color.g,
        context->current_color.b,
        context->current_color.a
    };
    
    // 首先尝试UTF-8渲染（支持中文）
    SDL_Surface* text_surface = TTF_RenderUTF8_Blended(context->current_font.ttf_font, text, color);
    if (!text_surface) {
        printf("[图形] UTF-8渲染失败，尝试普通渲染: %s\n", TTF_GetError());
        // 回退到普通文本渲染
        text_surface = TTF_RenderText_Blended(context->current_font.ttf_font, text, color);
        if (!text_surface) {
            printf("[图形] 错误: 创建文本表面失败: %s\n", TTF_GetError());
            return;
        }
    }
    
    // 创建纹理
    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(context->renderer, text_surface);
    if (!text_texture) {
        printf("[图形] 错误: 创建文本纹理失败: %s\n", SDL_GetError());
        SDL_FreeSurface(text_surface);
        return;
    }
    
    // 获取文本尺寸
    int text_width = text_surface->w;
    int text_height = text_surface->h;
    
    SDL_FreeSurface(text_surface);
    
    // 处理锚点
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
    
    // 渲染文本
    SDL_Rect dst_rect = {x, y, text_width, text_height};
    SDL_RenderCopy(context->renderer, text_texture, NULL, &dst_rect);
    
    // 清理资源
    SDL_DestroyTexture(text_texture);
}

/**
 * @brief 创建字体对象
 * @param name 字体名称
 * @param size 字体大小
 * @param style 字体样式
 * @return 字体对象
 */
j2me_font_t j2me_graphics_create_font(const char* name, int size, int style) {
    j2me_font_t font;
    font.size = size;
    font.style = style;
    font.ttf_font = NULL;
    
    if (name) {
        strncpy(font.name, name, sizeof(font.name) - 1);
        font.name[sizeof(font.name) - 1] = '\0';
    } else {
        strcpy(font.name, "Default");
    }
    
    return font;
}

/**
 * @brief 获取字体基线
 * @param context 图形上下文
 * @return 基线位置
 */
int j2me_graphics_get_font_baseline(j2me_graphics_context_t* context) {
    if (!context) {
        return 0;
    }
    
    if (context->current_font.ttf_font) {
        // TTF字体的基线计算
        int ascent = TTF_FontAscent(context->current_font.ttf_font);
        return ascent;
    }
    
    // 简化计算：约为字体高度的75%
    return context->current_font.size * 3 / 4;
}

/**
 * @brief 获取字符宽度
 * @param context 图形上下文
 * @param ch 字符
 * @return 字符宽度
 */
int j2me_graphics_get_char_width(j2me_graphics_context_t* context, char ch) {
    if (!context) {
        return 0;
    }
    
    if (context->current_font.ttf_font) {
        char str[2] = {ch, '\0'};
        int width, height;
        if (TTF_SizeText(context->current_font.ttf_font, str, &width, &height) == 0) {
            return width;
        }
    }
    
    // 简化计算
    return context->current_font.size * 0.6;
}