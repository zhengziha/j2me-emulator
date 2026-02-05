#ifndef J2ME_MIDP_GRAPHICS_H
#define J2ME_MIDP_GRAPHICS_H

#include "j2me_types.h"
#include "j2me_graphics.h"
#include "j2me_object.h"
#include <SDL2/SDL.h>

/**
 * @file j2me_midp_graphics.h
 * @brief MIDP图形API实现
 * 
 * 实现javax.microedition.lcdui.Graphics类的功能
 */

// 前向声明
typedef struct j2me_midp_graphics j2me_midp_graphics_t;
typedef struct j2me_midp_image j2me_midp_image_t;
typedef struct j2me_midp_font j2me_midp_font_t;

// 锚点常量 (javax.microedition.lcdui.Graphics)
#define ANCHOR_HCENTER      1
#define ANCHOR_VCENTER      2
#define ANCHOR_LEFT         4
#define ANCHOR_RIGHT        8
#define ANCHOR_TOP          16
#define ANCHOR_BOTTOM       32
#define ANCHOR_BASELINE     64

// 字体样式常量
#define FONT_STYLE_PLAIN    0
#define FONT_STYLE_BOLD     1
#define FONT_STYLE_ITALIC   2
#define FONT_STYLE_UNDERLINED 4

// 字体大小常量
#define FONT_SIZE_SMALL     8
#define FONT_SIZE_MEDIUM    0
#define FONT_SIZE_LARGE     16

// 字体类型常量
#define FONT_FACE_SYSTEM    0
#define FONT_FACE_MONOSPACE 32
#define FONT_FACE_PROPORTIONAL 64

// MIDP图形上下文
struct j2me_midp_graphics {
    j2me_graphics_context_t* base_context;  // 基础图形上下文
    j2me_midp_font_t* current_font;         // 当前字体
    int translate_x, translate_y;           // 坐标变换
    int stroke_style;                       // 线条样式
    bool text_antialiasing;                 // 文本抗锯齿
};

// MIDP图像
struct j2me_midp_image {
    j2me_object_header_t header;    // 对象头
    SDL_Surface* surface;           // SDL表面
    int width, height;              // 图像尺寸
    bool is_mutable;                // 是否可变
    j2me_midp_graphics_t* graphics; // 关联的图形上下文 (可变图像)
};

// MIDP字体
struct j2me_midp_font {
    j2me_object_header_t header;    // 对象头
    int face;                       // 字体类型
    int style;                      // 字体样式
    int size;                       // 字体大小
    int height;                     // 字体高度
    int baseline;                   // 基线位置
    SDL_Surface* char_cache[256];   // 字符缓存
};

/**
 * @brief 创建MIDP图形上下文
 * @param base_context 基础图形上下文
 * @return MIDP图形上下文指针
 */
j2me_midp_graphics_t* j2me_midp_graphics_create(j2me_graphics_context_t* base_context);

/**
 * @brief 销毁MIDP图形上下文
 * @param graphics MIDP图形上下文
 */
void j2me_midp_graphics_destroy(j2me_midp_graphics_t* graphics);

/**
 * @brief 设置颜色 (RGB)
 * @param graphics 图形上下文
 * @param red 红色分量 (0-255)
 * @param green 绿色分量 (0-255)
 * @param blue 蓝色分量 (0-255)
 */
void j2me_midp_graphics_set_color_rgb(j2me_midp_graphics_t* graphics, int red, int green, int blue);

/**
 * @brief 设置颜色 (整数RGB)
 * @param graphics 图形上下文
 * @param rgb RGB颜色值 (0xRRGGBB)
 */
void j2me_midp_graphics_set_color(j2me_midp_graphics_t* graphics, int rgb);

/**
 * @brief 获取当前颜色
 * @param graphics 图形上下文
 * @return RGB颜色值
 */
int j2me_midp_graphics_get_color(j2me_midp_graphics_t* graphics);

/**
 * @brief 平移坐标系
 * @param graphics 图形上下文
 * @param x X偏移量
 * @param y Y偏移量
 */
void j2me_midp_graphics_translate(j2me_midp_graphics_t* graphics, int x, int y);

/**
 * @brief 获取平移X坐标
 * @param graphics 图形上下文
 * @return X偏移量
 */
int j2me_midp_graphics_get_translate_x(j2me_midp_graphics_t* graphics);

/**
 * @brief 获取平移Y坐标
 * @param graphics 图形上下文
 * @return Y偏移量
 */
int j2me_midp_graphics_get_translate_y(j2me_midp_graphics_t* graphics);

/**
 * @brief 绘制直线
 * @param graphics 图形上下文
 * @param x1 起点X坐标
 * @param y1 起点Y坐标
 * @param x2 终点X坐标
 * @param y2 终点Y坐标
 */
void j2me_midp_graphics_draw_line(j2me_midp_graphics_t* graphics, int x1, int y1, int x2, int y2);

/**
 * @brief 绘制矩形
 * @param graphics 图形上下文
 * @param x X坐标
 * @param y Y坐标
 * @param width 宽度
 * @param height 高度
 */
void j2me_midp_graphics_draw_rect(j2me_midp_graphics_t* graphics, int x, int y, int width, int height);

/**
 * @brief 填充矩形
 * @param graphics 图形上下文
 * @param x X坐标
 * @param y Y坐标
 * @param width 宽度
 * @param height 高度
 */
void j2me_midp_graphics_fill_rect(j2me_midp_graphics_t* graphics, int x, int y, int width, int height);

/**
 * @brief 绘制圆角矩形
 * @param graphics 图形上下文
 * @param x X坐标
 * @param y Y坐标
 * @param width 宽度
 * @param height 高度
 * @param arc_width 圆角宽度
 * @param arc_height 圆角高度
 */
void j2me_midp_graphics_draw_round_rect(j2me_midp_graphics_t* graphics, int x, int y, int width, int height, int arc_width, int arc_height);

/**
 * @brief 填充圆角矩形
 * @param graphics 图形上下文
 * @param x X坐标
 * @param y Y坐标
 * @param width 宽度
 * @param height 高度
 * @param arc_width 圆角宽度
 * @param arc_height 圆角高度
 */
void j2me_midp_graphics_fill_round_rect(j2me_midp_graphics_t* graphics, int x, int y, int width, int height, int arc_width, int arc_height);

/**
 * @brief 绘制椭圆
 * @param graphics 图形上下文
 * @param x X坐标
 * @param y Y坐标
 * @param width 宽度
 * @param height 高度
 */
void j2me_midp_graphics_draw_arc(j2me_midp_graphics_t* graphics, int x, int y, int width, int height, int start_angle, int arc_angle);

/**
 * @brief 填充扇形
 * @param graphics 图形上下文
 * @param x X坐标
 * @param y Y坐标
 * @param width 宽度
 * @param height 高度
 * @param start_angle 起始角度
 * @param arc_angle 扇形角度
 */
void j2me_midp_graphics_fill_arc(j2me_midp_graphics_t* graphics, int x, int y, int width, int height, int start_angle, int arc_angle);

/**
 * @brief 绘制字符串
 * @param graphics 图形上下文
 * @param str 字符串
 * @param x X坐标
 * @param y Y坐标
 * @param anchor 锚点
 */
void j2me_midp_graphics_draw_string(j2me_midp_graphics_t* graphics, const char* str, int x, int y, int anchor);

/**
 * @brief 绘制字符
 * @param graphics 图形上下文
 * @param ch 字符
 * @param x X坐标
 * @param y Y坐标
 * @param anchor 锚点
 */
void j2me_midp_graphics_draw_char(j2me_midp_graphics_t* graphics, char ch, int x, int y, int anchor);

/**
 * @brief 绘制子字符串
 * @param graphics 图形上下文
 * @param str 字符串
 * @param offset 起始偏移
 * @param len 长度
 * @param x X坐标
 * @param y Y坐标
 * @param anchor 锚点
 */
void j2me_midp_graphics_draw_substring(j2me_midp_graphics_t* graphics, const char* str, int offset, int len, int x, int y, int anchor);

/**
 * @brief 绘制图像
 * @param graphics 图形上下文
 * @param image 图像对象
 * @param x X坐标
 * @param y Y坐标
 * @param anchor 锚点
 */
void j2me_midp_graphics_draw_image(j2me_midp_graphics_t* graphics, j2me_midp_image_t* image, int x, int y, int anchor);

/**
 * @brief 设置字体
 * @param graphics 图形上下文
 * @param font 字体对象
 */
void j2me_midp_graphics_set_font(j2me_midp_graphics_t* graphics, j2me_midp_font_t* font);

/**
 * @brief 获取当前字体
 * @param graphics 图形上下文
 * @return 字体对象
 */
j2me_midp_font_t* j2me_midp_graphics_get_font(j2me_midp_graphics_t* graphics);

/**
 * @brief 设置裁剪区域
 * @param graphics 图形上下文
 * @param x X坐标
 * @param y Y坐标
 * @param width 宽度
 * @param height 高度
 */
void j2me_midp_graphics_set_clip(j2me_midp_graphics_t* graphics, int x, int y, int width, int height);

/**
 * @brief 裁剪区域与指定区域求交
 * @param graphics 图形上下文
 * @param x X坐标
 * @param y Y坐标
 * @param width 宽度
 * @param height 高度
 */
void j2me_midp_graphics_clip_rect(j2me_midp_graphics_t* graphics, int x, int y, int width, int height);

/**
 * @brief 获取裁剪区域X坐标
 * @param graphics 图形上下文
 * @return X坐标
 */
int j2me_midp_graphics_get_clip_x(j2me_midp_graphics_t* graphics);

/**
 * @brief 获取裁剪区域Y坐标
 * @param graphics 图形上下文
 * @return Y坐标
 */
int j2me_midp_graphics_get_clip_y(j2me_midp_graphics_t* graphics);

/**
 * @brief 获取裁剪区域宽度
 * @param graphics 图形上下文
 * @return 宽度
 */
int j2me_midp_graphics_get_clip_width(j2me_midp_graphics_t* graphics);

/**
 * @brief 获取裁剪区域高度
 * @param graphics 图形上下文
 * @return 高度
 */
int j2me_midp_graphics_get_clip_height(j2me_midp_graphics_t* graphics);

// 字体相关函数

/**
 * @brief 创建字体
 * @param vm 虚拟机实例
 * @param face 字体类型
 * @param style 字体样式
 * @param size 字体大小
 * @return 字体对象
 */
j2me_midp_font_t* j2me_midp_font_create(j2me_vm_t* vm, int face, int style, int size);

/**
 * @brief 获取默认字体
 * @param vm 虚拟机实例
 * @return 默认字体对象
 */
j2me_midp_font_t* j2me_midp_font_get_default(j2me_vm_t* vm);

/**
 * @brief 获取字体高度
 * @param font 字体对象
 * @return 字体高度
 */
int j2me_midp_font_get_height(j2me_midp_font_t* font);

/**
 * @brief 获取字体基线位置
 * @param font 字体对象
 * @return 基线位置
 */
int j2me_midp_font_get_baseline_position(j2me_midp_font_t* font);

/**
 * @brief 获取字符串宽度
 * @param font 字体对象
 * @param str 字符串
 * @return 字符串宽度
 */
int j2me_midp_font_string_width(j2me_midp_font_t* font, const char* str);

/**
 * @brief 获取字符宽度
 * @param font 字体对象
 * @param ch 字符
 * @return 字符宽度
 */
int j2me_midp_font_char_width(j2me_midp_font_t* font, char ch);

/**
 * @brief 获取子字符串宽度
 * @param font 字体对象
 * @param str 字符串
 * @param offset 起始偏移
 * @param len 长度
 * @return 子字符串宽度
 */
int j2me_midp_font_substring_width(j2me_midp_font_t* font, const char* str, int offset, int len);

// 图像相关函数

/**
 * @brief 创建图像
 * @param vm 虚拟机实例
 * @param width 宽度
 * @param height 高度
 * @return 图像对象
 */
j2me_midp_image_t* j2me_midp_image_create(j2me_vm_t* vm, int width, int height);

/**
 * @brief 从文件创建图像
 * @param vm 虚拟机实例
 * @param filename 文件名
 * @return 图像对象
 */
j2me_midp_image_t* j2me_midp_image_create_from_file(j2me_vm_t* vm, const char* filename);

/**
 * @brief 获取图像宽度
 * @param image 图像对象
 * @return 宽度
 */
int j2me_midp_image_get_width(j2me_midp_image_t* image);

/**
 * @brief 获取图像高度
 * @param image 图像对象
 * @return 高度
 */
int j2me_midp_image_get_height(j2me_midp_image_t* image);

/**
 * @brief 检查图像是否可变
 * @param image 图像对象
 * @return 是否可变
 */
bool j2me_midp_image_is_mutable(j2me_midp_image_t* image);

/**
 * @brief 获取图像的图形上下文
 * @param image 图像对象
 * @return 图形上下文 (仅可变图像)
 */
j2me_midp_graphics_t* j2me_midp_image_get_graphics(j2me_midp_image_t* image);

#endif // J2ME_MIDP_GRAPHICS_H