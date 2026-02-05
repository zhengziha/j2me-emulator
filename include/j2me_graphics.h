#ifndef J2ME_GRAPHICS_H
#define J2ME_GRAPHICS_H

#include "j2me_types.h"
#include <SDL2/SDL.h>

/**
 * @file j2me_graphics.h
 * @brief J2ME图形系统接口
 * 
 * 基于SDL2的图形渲染系统，支持MIDP图形API
 */

// 颜色定义
typedef struct {
    uint8_t r, g, b, a;
} j2me_color_t;

// 图形上下文
typedef struct {
    SDL_Renderer* renderer;     // SDL渲染器
    SDL_Texture* canvas;        // 画布纹理
    int width, height;          // 画布尺寸
    j2me_color_t current_color; // 当前颜色
    int clip_x, clip_y;         // 裁剪区域
    int clip_width, clip_height;
    bool clipping_enabled;      // 是否启用裁剪
} j2me_graphics_context_t;

// 显示系统
typedef struct {
    SDL_Window* window;         // SDL窗口
    SDL_Renderer* renderer;     // SDL渲染器
    j2me_graphics_context_t* context; // 图形上下文
    int screen_width, screen_height;  // 屏幕尺寸
    bool fullscreen;            // 是否全屏
} j2me_display_t;

/**
 * @brief 初始化显示系统
 * @param width 窗口宽度
 * @param height 窗口高度
 * @param title 窗口标题
 * @return 显示系统指针
 */
j2me_display_t* j2me_display_initialize(int width, int height, const char* title);

/**
 * @brief 销毁显示系统
 * @param display 显示系统
 */
void j2me_display_destroy(j2me_display_t* display);

/**
 * @brief 创建图形上下文
 * @param display 显示系统
 * @param width 画布宽度
 * @param height 画布高度
 * @return 图形上下文指针
 */
j2me_graphics_context_t* j2me_graphics_create_context(j2me_display_t* display, int width, int height);

/**
 * @brief 销毁图形上下文
 * @param context 图形上下文
 */
void j2me_graphics_destroy_context(j2me_graphics_context_t* context);

/**
 * @brief 设置绘制颜色
 * @param context 图形上下文
 * @param color 颜色
 */
void j2me_graphics_set_color(j2me_graphics_context_t* context, j2me_color_t color);

/**
 * @brief 绘制像素点
 * @param context 图形上下文
 * @param x X坐标
 * @param y Y坐标
 */
void j2me_graphics_draw_pixel(j2me_graphics_context_t* context, int x, int y);

/**
 * @brief 绘制直线
 * @param context 图形上下文
 * @param x1 起点X坐标
 * @param y1 起点Y坐标
 * @param x2 终点X坐标
 * @param y2 终点Y坐标
 */
void j2me_graphics_draw_line(j2me_graphics_context_t* context, int x1, int y1, int x2, int y2);

/**
 * @brief 绘制矩形
 * @param context 图形上下文
 * @param x X坐标
 * @param y Y坐标
 * @param width 宽度
 * @param height 高度
 * @param filled 是否填充
 */
void j2me_graphics_draw_rect(j2me_graphics_context_t* context, int x, int y, int width, int height, bool filled);

/**
 * @brief 设置裁剪区域
 * @param context 图形上下文
 * @param x X坐标
 * @param y Y坐标
 * @param width 宽度
 * @param height 高度
 */
void j2me_graphics_set_clip(j2me_graphics_context_t* context, int x, int y, int width, int height);

/**
 * @brief 清除画布
 * @param context 图形上下文
 */
void j2me_graphics_clear(j2me_graphics_context_t* context);

/**
 * @brief 刷新显示
 * @param display 显示系统
 */
void j2me_display_refresh(j2me_display_t* display);

#endif // J2ME_GRAPHICS_H