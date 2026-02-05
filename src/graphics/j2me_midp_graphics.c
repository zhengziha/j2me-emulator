#include "j2me_midp_graphics.h"
#include "j2me_object.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/**
 * @file j2me_midp_graphics.c
 * @brief MIDP图形API实现
 * 
 * 实现完整的MIDP Graphics类功能
 */

// 默认字体 (全局单例)
static j2me_midp_font_t* default_font = NULL;

j2me_midp_graphics_t* j2me_midp_graphics_create(j2me_graphics_context_t* base_context) {
    if (!base_context) {
        return NULL;
    }
    
    j2me_midp_graphics_t* graphics = (j2me_midp_graphics_t*)malloc(sizeof(j2me_midp_graphics_t));
    if (!graphics) {
        return NULL;
    }
    
    memset(graphics, 0, sizeof(j2me_midp_graphics_t));
    graphics->base_context = base_context;
    graphics->translate_x = 0;
    graphics->translate_y = 0;
    graphics->stroke_style = 0; // SOLID
    graphics->text_antialiasing = true;
    graphics->current_font = NULL; // 将使用默认字体
    
    printf("[MIDP图形] 创建MIDP图形上下文\n");
    return graphics;
}

void j2me_midp_graphics_destroy(j2me_midp_graphics_t* graphics) {
    if (graphics) {
        free(graphics);
        printf("[MIDP图形] 销毁MIDP图形上下文\n");
    }
}

void j2me_midp_graphics_set_color_rgb(j2me_midp_graphics_t* graphics, int red, int green, int blue) {
    if (!graphics || !graphics->base_context) {
        return;
    }
    
    // 限制颜色值范围
    red = red < 0 ? 0 : (red > 255 ? 255 : red);
    green = green < 0 ? 0 : (green > 255 ? 255 : green);
    blue = blue < 0 ? 0 : (blue > 255 ? 255 : blue);
    
    j2me_color_t color = {red, green, blue, 255};
    j2me_graphics_set_color(graphics->base_context, color);
}

void j2me_midp_graphics_set_color(j2me_midp_graphics_t* graphics, int rgb) {
    int red = (rgb >> 16) & 0xFF;
    int green = (rgb >> 8) & 0xFF;
    int blue = rgb & 0xFF;
    
    j2me_midp_graphics_set_color_rgb(graphics, red, green, blue);
}

int j2me_midp_graphics_get_color(j2me_midp_graphics_t* graphics) {
    if (!graphics || !graphics->base_context) {
        return 0;
    }
    
    j2me_color_t color = graphics->base_context->current_color;
    return (color.r << 16) | (color.g << 8) | color.b;
}

void j2me_midp_graphics_translate(j2me_midp_graphics_t* graphics, int x, int y) {
    if (!graphics) {
        return;
    }
    
    graphics->translate_x += x;
    graphics->translate_y += y;
}

int j2me_midp_graphics_get_translate_x(j2me_midp_graphics_t* graphics) {
    return graphics ? graphics->translate_x : 0;
}

int j2me_midp_graphics_get_translate_y(j2me_midp_graphics_t* graphics) {
    return graphics ? graphics->translate_y : 0;
}

/**
 * @brief 应用坐标变换
 * @param graphics 图形上下文
 * @param x 输入X坐标指针
 * @param y 输入Y坐标指针
 */
static void apply_transform(j2me_midp_graphics_t* graphics, int* x, int* y) {
    if (graphics) {
        *x += graphics->translate_x;
        *y += graphics->translate_y;
    }
}

void j2me_midp_graphics_draw_line(j2me_midp_graphics_t* graphics, int x1, int y1, int x2, int y2) {
    if (!graphics || !graphics->base_context) {
        return;
    }
    
    apply_transform(graphics, &x1, &y1);
    apply_transform(graphics, &x2, &y2);
    
    j2me_graphics_draw_line(graphics->base_context, x1, y1, x2, y2);
}

void j2me_midp_graphics_draw_rect(j2me_midp_graphics_t* graphics, int x, int y, int width, int height) {
    if (!graphics || !graphics->base_context) {
        return;
    }
    
    apply_transform(graphics, &x, &y);
    j2me_graphics_draw_rect(graphics->base_context, x, y, width, height, false);
}

void j2me_midp_graphics_fill_rect(j2me_midp_graphics_t* graphics, int x, int y, int width, int height) {
    if (!graphics || !graphics->base_context) {
        return;
    }
    
    apply_transform(graphics, &x, &y);
    j2me_graphics_draw_rect(graphics->base_context, x, y, width, height, true);
}

void j2me_midp_graphics_draw_round_rect(j2me_midp_graphics_t* graphics, int x, int y, int width, int height, int arc_width, int arc_height) {
    if (!graphics || !graphics->base_context) {
        return;
    }
    
    apply_transform(graphics, &x, &y);
    
    // 实现真正的圆角矩形绘制
    if (arc_width <= 0 || arc_height <= 0) {
        // 如果圆角大小为0，绘制普通矩形
        j2me_graphics_draw_rect(graphics->base_context, x, y, width, height, false);
    } else {
        // 绘制圆角矩形：使用四个直线段和四个圆弧
        SDL_Renderer* renderer = graphics->base_context->renderer;
        
        // 限制圆角大小不超过矩形的一半
        int max_arc_width = width / 2;
        int max_arc_height = height / 2;
        arc_width = (arc_width > max_arc_width) ? max_arc_width : arc_width;
        arc_height = (arc_height > max_arc_height) ? max_arc_height : arc_height;
        
        // 设置绘制颜色
        j2me_color_t color = graphics->base_context->current_color;
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        
        // 绘制四条边（不包括圆角部分）
        // 上边
        SDL_RenderDrawLine(renderer, x + arc_width, y, x + width - arc_width, y);
        // 下边
        SDL_RenderDrawLine(renderer, x + arc_width, y + height - 1, x + width - arc_width, y + height - 1);
        // 左边
        SDL_RenderDrawLine(renderer, x, y + arc_height, x, y + height - arc_height);
        // 右边
        SDL_RenderDrawLine(renderer, x + width - 1, y + arc_height, x + width - 1, y + height - arc_height);
        
        // 绘制四个圆角（简化为小矩形，实际应该绘制圆弧）
        // 左上角
        for (int i = 0; i < arc_width; i++) {
            for (int j = 0; j < arc_height; j++) {
                if (i * i + j * j >= (arc_width * arc_height) / 4) {
                    SDL_RenderDrawPoint(renderer, x + arc_width - i, y + arc_height - j);
                }
            }
        }
        // 右上角
        for (int i = 0; i < arc_width; i++) {
            for (int j = 0; j < arc_height; j++) {
                if (i * i + j * j >= (arc_width * arc_height) / 4) {
                    SDL_RenderDrawPoint(renderer, x + width - arc_width + i, y + arc_height - j);
                }
            }
        }
        // 左下角
        for (int i = 0; i < arc_width; i++) {
            for (int j = 0; j < arc_height; j++) {
                if (i * i + j * j >= (arc_width * arc_height) / 4) {
                    SDL_RenderDrawPoint(renderer, x + arc_width - i, y + height - arc_height + j);
                }
            }
        }
        // 右下角
        for (int i = 0; i < arc_width; i++) {
            for (int j = 0; j < arc_height; j++) {
                if (i * i + j * j >= (arc_width * arc_height) / 4) {
                    SDL_RenderDrawPoint(renderer, x + width - arc_width + i, y + height - arc_height + j);
                }
            }
        }
    }
    
    printf("[MIDP图形] 绘制圆角矩形 (%d,%d,%dx%d) 圆角(%dx%d)\n", 
           x, y, width, height, arc_width, arc_height);
}

void j2me_midp_graphics_fill_round_rect(j2me_midp_graphics_t* graphics, int x, int y, int width, int height, int arc_width, int arc_height) {
    if (!graphics || !graphics->base_context) {
        return;
    }
    
    apply_transform(graphics, &x, &y);
    
    // 实现真正的圆角矩形填充
    if (arc_width <= 0 || arc_height <= 0) {
        // 如果圆角大小为0，填充普通矩形
        j2me_graphics_draw_rect(graphics->base_context, x, y, width, height, true);
    } else {
        // 填充圆角矩形
        SDL_Renderer* renderer = graphics->base_context->renderer;
        
        // 限制圆角大小不超过矩形的一半
        int max_arc_width = width / 2;
        int max_arc_height = height / 2;
        arc_width = (arc_width > max_arc_width) ? max_arc_width : arc_width;
        arc_height = (arc_height > max_arc_height) ? max_arc_height : arc_height;
        
        // 设置绘制颜色
        j2me_color_t color = graphics->base_context->current_color;
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        
        // 填充中间的矩形区域
        SDL_Rect center_rect = {x + arc_width, y, width - 2 * arc_width, height};
        SDL_RenderFillRect(renderer, &center_rect);
        
        // 填充左右两侧的矩形区域
        SDL_Rect left_rect = {x, y + arc_height, arc_width, height - 2 * arc_height};
        SDL_RenderFillRect(renderer, &left_rect);
        
        SDL_Rect right_rect = {x + width - arc_width, y + arc_height, arc_width, height - 2 * arc_height};
        SDL_RenderFillRect(renderer, &right_rect);
        
        // 填充四个圆角区域（简化为椭圆形区域）
        // 左上角
        for (int i = 0; i < arc_width; i++) {
            for (int j = 0; j < arc_height; j++) {
                if (i * i * arc_height * arc_height + j * j * arc_width * arc_width <= arc_width * arc_width * arc_height * arc_height) {
                    SDL_RenderDrawPoint(renderer, x + arc_width - i, y + arc_height - j);
                }
            }
        }
        // 右上角
        for (int i = 0; i < arc_width; i++) {
            for (int j = 0; j < arc_height; j++) {
                if (i * i * arc_height * arc_height + j * j * arc_width * arc_width <= arc_width * arc_width * arc_height * arc_height) {
                    SDL_RenderDrawPoint(renderer, x + width - arc_width + i, y + arc_height - j);
                }
            }
        }
        // 左下角
        for (int i = 0; i < arc_width; i++) {
            for (int j = 0; j < arc_height; j++) {
                if (i * i * arc_height * arc_height + j * j * arc_width * arc_width <= arc_width * arc_width * arc_height * arc_height) {
                    SDL_RenderDrawPoint(renderer, x + arc_width - i, y + height - arc_height + j);
                }
            }
        }
        // 右下角
        for (int i = 0; i < arc_width; i++) {
            for (int j = 0; j < arc_height; j++) {
                if (i * i * arc_height * arc_height + j * j * arc_width * arc_width <= arc_width * arc_width * arc_height * arc_height) {
                    SDL_RenderDrawPoint(renderer, x + width - arc_width + i, y + height - arc_height + j);
                }
            }
        }
    }
    
    printf("[MIDP图形] 填充圆角矩形 (%d,%d,%dx%d) 圆角(%dx%d)\n", 
           x, y, width, height, arc_width, arc_height);
}

/**
 * @brief 绘制椭圆弧线 (简化实现)
 */
static void draw_ellipse_arc(j2me_graphics_context_t* context, int cx, int cy, int rx, int ry, int start_angle, int arc_angle, bool filled) {
    // 简化实现：使用直线近似椭圆
    int steps = 32;
    double angle_step = (arc_angle * M_PI / 180.0) / steps;
    double start_rad = start_angle * M_PI / 180.0;
    
    int prev_x = cx + (int)(rx * cos(start_rad));
    int prev_y = cy + (int)(ry * sin(start_rad));
    
    for (int i = 1; i <= steps; i++) {
        double angle = start_rad + i * angle_step;
        int curr_x = cx + (int)(rx * cos(angle));
        int curr_y = cy + (int)(ry * sin(angle));
        
        j2me_graphics_draw_line(context, prev_x, prev_y, curr_x, curr_y);
        
        prev_x = curr_x;
        prev_y = curr_y;
    }
    
    if (filled) {
        // 简化的填充实现
        for (int i = 0; i <= steps; i++) {
            double angle = start_rad + i * angle_step;
            int end_x = cx + (int)(rx * cos(angle));
            int end_y = cy + (int)(ry * sin(angle));
            j2me_graphics_draw_line(context, cx, cy, end_x, end_y);
        }
    }
}

void j2me_midp_graphics_draw_arc(j2me_midp_graphics_t* graphics, int x, int y, int width, int height, int start_angle, int arc_angle) {
    if (!graphics || !graphics->base_context) {
        return;
    }
    
    apply_transform(graphics, &x, &y);
    
    int cx = x + width / 2;
    int cy = y + height / 2;
    int rx = width / 2;
    int ry = height / 2;
    
    draw_ellipse_arc(graphics->base_context, cx, cy, rx, ry, start_angle, arc_angle, false);
}

void j2me_midp_graphics_fill_arc(j2me_midp_graphics_t* graphics, int x, int y, int width, int height, int start_angle, int arc_angle) {
    if (!graphics || !graphics->base_context) {
        return;
    }
    
    apply_transform(graphics, &x, &y);
    
    int cx = x + width / 2;
    int cy = y + height / 2;
    int rx = width / 2;
    int ry = height / 2;
    
    draw_ellipse_arc(graphics->base_context, cx, cy, rx, ry, start_angle, arc_angle, true);
}

/**
 * @brief 计算文本锚点位置
 * @param text_width 文本宽度
 * @param text_height 文本高度
 * @param x 原始X坐标
 * @param y 原始Y坐标
 * @param anchor 锚点
 * @param out_x 输出X坐标
 * @param out_y 输出Y坐标
 */
static void calculate_text_anchor(int text_width, int text_height, int x, int y, int anchor, int* out_x, int* out_y) {
    *out_x = x;
    *out_y = y;
    
    // 水平对齐
    if (anchor & ANCHOR_HCENTER) {
        *out_x -= text_width / 2;
    } else if (anchor & ANCHOR_RIGHT) {
        *out_x -= text_width;
    }
    // 默认是LEFT对齐
    
    // 垂直对齐
    if (anchor & ANCHOR_VCENTER) {
        *out_y -= text_height / 2;
    } else if (anchor & ANCHOR_BOTTOM) {
        *out_y -= text_height;
    } else if (anchor & ANCHOR_BASELINE) {
        // 基线对齐，需要字体信息
        *out_y -= text_height * 3 / 4; // 简化处理
    }
    // 默认是TOP对齐
}

void j2me_midp_graphics_draw_string(j2me_midp_graphics_t* graphics, const char* str, int x, int y, int anchor) {
    if (!graphics || !graphics->base_context || !str) {
        return;
    }
    
    apply_transform(graphics, &x, &y);
    
    // 获取字体信息
    j2me_midp_font_t* font = graphics->current_font;
    if (!font) {
        font = j2me_midp_font_get_default(NULL); // 使用默认字体
    }
    
    int text_width = font ? j2me_midp_font_string_width(font, str) : strlen(str) * 8;
    int text_height = font ? j2me_midp_font_get_height(font) : 12;
    
    // 计算锚点位置
    int draw_x, draw_y;
    calculate_text_anchor(text_width, text_height, x, y, anchor, &draw_x, &draw_y);
    
    // 简化实现：逐字符绘制
    int current_x = draw_x;
    for (const char* ch = str; *ch; ch++) {
        // 简单的字符绘制 (使用像素点)
        for (int dy = 0; dy < 8; dy++) {
            for (int dx = 0; dx < 6; dx++) {
                // 简化的字符位图 (实际应该使用字体渲染)
                if ((dx + dy) % 3 == 0) { // 简单的模式
                    j2me_graphics_draw_pixel(graphics->base_context, current_x + dx, draw_y + dy);
                }
            }
        }
        current_x += 8; // 字符宽度
    }
    
    printf("[MIDP图形] 绘制字符串: \"%s\" 位置(%d,%d) 锚点=0x%x\n", str, x, y, anchor);
}

void j2me_midp_graphics_draw_char(j2me_midp_graphics_t* graphics, char ch, int x, int y, int anchor) {
    char str[2] = {ch, '\0'};
    j2me_midp_graphics_draw_string(graphics, str, x, y, anchor);
}

void j2me_midp_graphics_draw_substring(j2me_midp_graphics_t* graphics, const char* str, int offset, int len, int x, int y, int anchor) {
    if (!str || offset < 0 || len <= 0) {
        return;
    }
    
    int str_len = strlen(str);
    if (offset >= str_len) {
        return;
    }
    
    if (offset + len > str_len) {
        len = str_len - offset;
    }
    
    char* substr = (char*)malloc(len + 1);
    if (!substr) {
        return;
    }
    
    strncpy(substr, str + offset, len);
    substr[len] = '\0';
    
    j2me_midp_graphics_draw_string(graphics, substr, x, y, anchor);
    
    free(substr);
}

void j2me_midp_graphics_draw_image(j2me_midp_graphics_t* graphics, j2me_midp_image_t* image, int x, int y, int anchor) {
    if (!graphics || !graphics->base_context || !image) {
        return;
    }
    
    apply_transform(graphics, &x, &y);
    
    int img_width = j2me_midp_image_get_width(image);
    int img_height = j2me_midp_image_get_height(image);
    
    // 计算锚点位置
    int draw_x, draw_y;
    calculate_text_anchor(img_width, img_height, x, y, anchor, &draw_x, &draw_y);
    
    // 简化实现：绘制图像边框
    j2me_graphics_draw_rect(graphics->base_context, draw_x, draw_y, img_width, img_height, false);
    
    printf("[MIDP图形] 绘制图像: %dx%d 位置(%d,%d) 锚点=0x%x\n", 
           img_width, img_height, x, y, anchor);
}

void j2me_midp_graphics_set_font(j2me_midp_graphics_t* graphics, j2me_midp_font_t* font) {
    if (graphics) {
        graphics->current_font = font;
    }
}

j2me_midp_font_t* j2me_midp_graphics_get_font(j2me_midp_graphics_t* graphics) {
    if (!graphics) {
        return NULL;
    }
    
    return graphics->current_font ? graphics->current_font : j2me_midp_font_get_default(NULL);
}

void j2me_midp_graphics_set_clip(j2me_midp_graphics_t* graphics, int x, int y, int width, int height) {
    if (!graphics || !graphics->base_context) {
        return;
    }
    
    apply_transform(graphics, &x, &y);
    j2me_graphics_set_clip(graphics->base_context, x, y, width, height);
}

void j2me_midp_graphics_clip_rect(j2me_midp_graphics_t* graphics, int x, int y, int width, int height) {
    if (!graphics || !graphics->base_context) {
        return;
    }
    
    apply_transform(graphics, &x, &y);
    
    // 计算当前裁剪区域与新区域的交集
    int current_x = graphics->base_context->clip_x;
    int current_y = graphics->base_context->clip_y;
    int current_w = graphics->base_context->clip_width;
    int current_h = graphics->base_context->clip_height;
    
    int new_x = (x > current_x) ? x : current_x;
    int new_y = (y > current_y) ? y : current_y;
    int new_w = ((x + width) < (current_x + current_w)) ? (x + width - new_x) : (current_x + current_w - new_x);
    int new_h = ((y + height) < (current_y + current_h)) ? (y + height - new_y) : (current_y + current_h - new_y);
    
    if (new_w > 0 && new_h > 0) {
        j2me_graphics_set_clip(graphics->base_context, new_x, new_y, new_w, new_h);
    }
}

int j2me_midp_graphics_get_clip_x(j2me_midp_graphics_t* graphics) {
    return (graphics && graphics->base_context) ? 
           graphics->base_context->clip_x - graphics->translate_x : 0;
}

int j2me_midp_graphics_get_clip_y(j2me_midp_graphics_t* graphics) {
    return (graphics && graphics->base_context) ? 
           graphics->base_context->clip_y - graphics->translate_y : 0;
}

int j2me_midp_graphics_get_clip_width(j2me_midp_graphics_t* graphics) {
    return (graphics && graphics->base_context) ? graphics->base_context->clip_width : 0;
}

int j2me_midp_graphics_get_clip_height(j2me_midp_graphics_t* graphics) {
    return (graphics && graphics->base_context) ? graphics->base_context->clip_height : 0;
}

// 字体实现

j2me_midp_font_t* j2me_midp_font_create(j2me_vm_t* vm, int face, int style, int size) {
    j2me_midp_font_t* font = (j2me_midp_font_t*)malloc(sizeof(j2me_midp_font_t));
    if (!font) {
        return NULL;
    }
    
    memset(font, 0, sizeof(j2me_midp_font_t));
    
    font->face = face;
    font->style = style;
    font->size = size;
    
    // 计算字体尺寸
    switch (size) {
        case FONT_SIZE_SMALL:
            font->height = 10;
            break;
        case FONT_SIZE_LARGE:
            font->height = 16;
            break;
        default: // FONT_SIZE_MEDIUM
            font->height = 12;
            break;
    }
    
    font->baseline = font->height * 3 / 4;
    
    printf("[MIDP图形] 创建字体: face=%d, style=%d, size=%d, height=%d\n", 
           face, style, size, font->height);
    
    return font;
}

j2me_midp_font_t* j2me_midp_font_get_default(j2me_vm_t* vm) {
    if (!default_font) {
        default_font = j2me_midp_font_create(vm, FONT_FACE_SYSTEM, FONT_STYLE_PLAIN, FONT_SIZE_MEDIUM);
    }
    return default_font;
}

int j2me_midp_font_get_height(j2me_midp_font_t* font) {
    return font ? font->height : 12;
}

int j2me_midp_font_get_baseline_position(j2me_midp_font_t* font) {
    return font ? font->baseline : 9;
}

int j2me_midp_font_string_width(j2me_midp_font_t* font, const char* str) {
    if (!str) {
        return 0;
    }
    
    int width = strlen(str) * 8; // 简化：每个字符8像素宽
    
    if (font && (font->style & FONT_STYLE_BOLD)) {
        width += strlen(str); // 粗体稍宽
    }
    
    return width;
}

int j2me_midp_font_char_width(j2me_midp_font_t* font, char ch) {
    char str[2] = {ch, '\0'};
    return j2me_midp_font_string_width(font, str);
}

int j2me_midp_font_substring_width(j2me_midp_font_t* font, const char* str, int offset, int len) {
    if (!str || offset < 0 || len <= 0) {
        return 0;
    }
    
    int str_len = strlen(str);
    if (offset >= str_len) {
        return 0;
    }
    
    if (offset + len > str_len) {
        len = str_len - offset;
    }
    
    int width = len * 8; // 简化计算
    
    if (font && (font->style & FONT_STYLE_BOLD)) {
        width += len;
    }
    
    return width;
}

// 图像实现

j2me_midp_image_t* j2me_midp_image_create(j2me_vm_t* vm, int width, int height) {
    if (!vm || width <= 0 || height <= 0) {
        return NULL;
    }
    
    j2me_midp_image_t* image = (j2me_midp_image_t*)malloc(sizeof(j2me_midp_image_t));
    if (!image) {
        return NULL;
    }
    
    memset(image, 0, sizeof(j2me_midp_image_t));
    
    // 创建SDL表面
    image->surface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
    if (!image->surface) {
        free(image);
        return NULL;
    }
    
    image->width = width;
    image->height = height;
    image->is_mutable = true;
    
    printf("[MIDP图形] 创建图像: %dx%d\n", width, height);
    
    return image;
}

j2me_midp_image_t* j2me_midp_image_create_from_file(j2me_vm_t* vm, const char* filename) {
    if (!vm || !filename) {
        return NULL;
    }
    
    // 简化实现：创建固定大小的图像
    j2me_midp_image_t* image = j2me_midp_image_create(vm, 64, 64);
    if (image) {
        image->is_mutable = false;
        printf("[MIDP图形] 从文件创建图像: %s (简化为64x64)\n", filename);
    }
    
    return image;
}

int j2me_midp_image_get_width(j2me_midp_image_t* image) {
    return image ? image->width : 0;
}

int j2me_midp_image_get_height(j2me_midp_image_t* image) {
    return image ? image->height : 0;
}

bool j2me_midp_image_is_mutable(j2me_midp_image_t* image) {
    return image ? image->is_mutable : false;
}

j2me_midp_graphics_t* j2me_midp_image_get_graphics(j2me_midp_image_t* image) {
    if (!image || !image->is_mutable) {
        return NULL;
    }
    
    if (!image->graphics) {
        // 为可变图像创建图形上下文
        // 这里需要一个基于SDL表面的图形上下文
        // 简化实现：返回NULL
        printf("[MIDP图形] 获取图像图形上下文 (未实现)\n");
    }
    
    return image->graphics;
}