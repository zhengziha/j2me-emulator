#ifndef J2ME_GRAPHICS_H
#define J2ME_GRAPHICS_H

#include "j2me_types.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

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

// 字体定义
typedef struct {
    int size;               // 字体大小
    int style;              // 字体样式 (PLAIN, BOLD, ITALIC)
    char name[64];          // 字体名称
    TTF_Font* ttf_font;     // SDL_ttf字体对象
} j2me_font_t;

// 图像定义
typedef struct {
    SDL_Texture* texture;   // SDL纹理
    int width, height;      // 图像尺寸
    bool mutable;           // 是否可变
} j2me_image_t;

// 图形上下文
typedef struct {
    SDL_Renderer* renderer;     // SDL渲染器
    SDL_Texture* canvas;        // 画布纹理
    int width, height;          // 画布尺寸
    j2me_color_t current_color; // 当前颜色
    j2me_font_t current_font;   // 当前字体
    int clip_x, clip_y;         // 裁剪区域
    int clip_width, clip_height;
    bool clipping_enabled;      // 是否启用裁剪
    int translate_x, translate_y; // 坐标变换
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

/**
 * @brief 绘制椭圆
 * @param context 图形上下文
 * @param x X坐标
 * @param y Y坐标
 * @param width 宽度
 * @param height 高度
 * @param filled 是否填充
 */
void j2me_graphics_draw_oval(j2me_graphics_context_t* context, int x, int y, int width, int height, bool filled);

/**
 * @brief 绘制圆弧
 * @param context 图形上下文
 * @param x X坐标
 * @param y Y坐标
 * @param width 宽度
 * @param height 高度
 * @param start_angle 起始角度
 * @param arc_angle 弧度角度
 * @param filled 是否填充
 */
void j2me_graphics_draw_arc(j2me_graphics_context_t* context, int x, int y, int width, int height, 
                           int start_angle, int arc_angle, bool filled);

/**
 * @brief 绘制多边形
 * @param context 图形上下文
 * @param x_points X坐标数组
 * @param y_points Y坐标数组
 * @param num_points 点数
 * @param filled 是否填充
 */
void j2me_graphics_draw_polygon(j2me_graphics_context_t* context, int* x_points, int* y_points, 
                               int num_points, bool filled);

/**
 * @brief 绘制图像
 * @param context 图形上下文
 * @param image 图像
 * @param x X坐标
 * @param y Y坐标
 * @param anchor 锚点
 */
void j2me_graphics_draw_image(j2me_graphics_context_t* context, j2me_image_t* image, 
                             int x, int y, int anchor);

/**
 * @brief 绘制字符串
 * @param context 图形上下文
 * @param text 文本
 * @param x X坐标
 * @param y Y坐标
 * @param anchor 锚点
 */
void j2me_graphics_draw_string(j2me_graphics_context_t* context, const char* text, 
                              int x, int y, int anchor);

/**
 * @brief 设置字体
 * @param context 图形上下文
 * @param font 字体
 */
void j2me_graphics_set_font(j2me_graphics_context_t* context, j2me_font_t font);

/**
 * @brief 获取字符串宽度
 * @param context 图形上下文
 * @param text 文本
 * @return 字符串宽度
 */
int j2me_graphics_get_string_width(j2me_graphics_context_t* context, const char* text);

/**
 * @brief 获取字体高度
 * @param context 图形上下文
 * @return 字体高度
 */
int j2me_graphics_get_font_height(j2me_graphics_context_t* context);

/**
 * @brief 设置坐标变换
 * @param context 图形上下文
 * @param x X偏移
 * @param y Y偏移
 */
void j2me_graphics_translate(j2me_graphics_context_t* context, int x, int y);

/**
 * @brief 创建图像
 * @param context 图形上下文
 * @param width 宽度
 * @param height 高度
 * @return 图像指针
 */
j2me_image_t* j2me_image_create(j2me_graphics_context_t* context, int width, int height);

/**
 * @brief 从文件加载图像
 * @param context 图形上下文
 * @param filename 文件名
 * @return 图像指针
 */
j2me_image_t* j2me_image_load(j2me_graphics_context_t* context, const char* filename);

/**
 * @brief 从内存数据创建图像
 * @param context 图形上下文
 * @param data 图像数据
 * @param data_size 数据大小
 * @return 图像指针
 */
j2me_image_t* j2me_image_create_from_data(j2me_graphics_context_t* context, 
                                          const uint8_t* data, size_t data_size);

/**
 * @brief 销毁图像
 * @param image 图像
 */
void j2me_image_destroy(j2me_image_t* image);

/**
 * @brief 加载默认字体
 * @param context 图形上下文
 */
void j2me_graphics_load_default_font(j2me_graphics_context_t* context);

/**
 * @brief 加载指定字体
 * @param context 图形上下文
 * @param font_name 字体名称
 * @param size 字体大小
 * @param style 字体样式
 * @return 是否成功
 */
bool j2me_graphics_load_font(j2me_graphics_context_t* context, const char* font_name, 
                            int size, int style);

/**
 * @brief 使用TTF字体渲染文本
 * @param context 图形上下文
 * @param text 文本内容
 * @param x X坐标
 * @param y Y坐标
 * @param anchor 锚点
 */
void j2me_graphics_render_ttf_text(j2me_graphics_context_t* context, const char* text, 
                                  int x, int y, int anchor);

/**
 * @brief 创建字体对象
 * @param name 字体名称
 * @param size 字体大小
 * @param style 字体样式
 * @return 字体对象
 */
j2me_font_t j2me_graphics_create_font(const char* name, int size, int style);

/**
 * @brief 获取字体基线
 * @param context 图形上下文
 * @return 基线位置
 */
int j2me_graphics_get_font_baseline(j2me_graphics_context_t* context);

/**
 * @brief 获取字符宽度
 * @param context 图形上下文
 * @param ch 字符
 * @return 字符宽度
 */
int j2me_graphics_get_char_width(j2me_graphics_context_t* context, char ch);

#endif // J2ME_GRAPHICS_H