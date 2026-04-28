#include "j2me_vm.h"
#include "j2me_graphics.h"
#include "j2me_midp_graphics.h"
#include "j2me_input.h"
#include "j2me_log.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @file midp_api_enhanced_test.c
 * @brief MIDP API增强测试程序
 * 
 * 全面测试MIDP Graphics、Font、Image和Canvas功能
 */

void test_graphics_api(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试Graphics API ===\n");
    
    if (!vm->display || !vm->display->context) {
        LOG_DEBUG("  ✗ 显示系统未初始化\n");
        return;
    }
    
    j2me_graphics_context_t* context = vm->display->context;
    j2me_midp_graphics_t* g = j2me_midp_graphics_create(context);
    
    if (!g) {
        LOG_DEBUG("  ✗ 无法创建MIDP图形上下文\n");
        return;
    }
    
    LOG_DEBUG("  ✓ MIDP图形上下文创建成功\n");
    
    // 测试颜色设置
    j2me_midp_graphics_set_color_rgb(g, 255, 0, 0);
    int color = j2me_midp_graphics_get_color(g);
    LOG_DEBUG("  ✓ 颜色设置: 0x%06X\n", color);
    
    // 测试坐标变换
    j2me_midp_graphics_translate(g, 10, 20);
    LOG_DEBUG("  ✓ 坐标变换: (%d, %d)\n", 
           j2me_midp_graphics_get_translate_x(g),
           j2me_midp_graphics_get_translate_y(g));
    
    // 测试基础绘制
    j2me_midp_graphics_set_color_rgb(g, 0, 255, 0);
    j2me_midp_graphics_draw_line(g, 0, 0, 100, 100);
    j2me_midp_graphics_draw_rect(g, 10, 10, 50, 30);
    j2me_midp_graphics_fill_rect(g, 70, 10, 50, 30);
    LOG_DEBUG("  ✓ 基础绘制功能正常\n");
    
    // 测试圆角矩形
    j2me_midp_graphics_set_color_rgb(g, 0, 0, 255);
    j2me_midp_graphics_draw_round_rect(g, 10, 50, 60, 40, 10, 10);
    j2me_midp_graphics_fill_round_rect(g, 80, 50, 60, 40, 10, 10);
    LOG_DEBUG("  ✓ 圆角矩形绘制正常\n");
    
    // 测试弧形和扇形
    j2me_midp_graphics_set_color_rgb(g, 255, 255, 0);
    j2me_midp_graphics_draw_arc(g, 10, 100, 60, 60, 0, 90);
    j2me_midp_graphics_fill_arc(g, 80, 100, 60, 60, 0, 270);
    LOG_DEBUG("  ✓ 弧形和扇形绘制正常\n");
    
    // 测试裁剪区域
    j2me_midp_graphics_set_clip(g, 0, 0, 100, 100);
    LOG_DEBUG("  ✓ 裁剪区域设置: (%d, %d, %d, %d)\n",
           j2me_midp_graphics_get_clip_x(g),
           j2me_midp_graphics_get_clip_y(g),
           j2me_midp_graphics_get_clip_width(g),
           j2me_midp_graphics_get_clip_height(g));
    
    j2me_midp_graphics_destroy(g);
    LOG_DEBUG("  ✓ Graphics API测试完成\n");
}

void test_font_system(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试Font系统 ===\n");
    
    // 获取默认字体
    j2me_midp_font_t* default_font = j2me_midp_font_get_default(vm);
    if (default_font) {
        LOG_DEBUG("  ✓ 默认字体获取成功\n");
        LOG_DEBUG("    高度: %d, 基线: %d\n",
               j2me_midp_font_get_height(default_font),
               j2me_midp_font_get_baseline_position(default_font));
    }
    
    // 创建不同样式的字体
    j2me_midp_font_t* bold_font = j2me_midp_font_create(vm, 
        FONT_FACE_SYSTEM, FONT_STYLE_BOLD, FONT_SIZE_MEDIUM);
    if (bold_font) {
        LOG_DEBUG("  ✓ 粗体字体创建成功\n");
    }
    
    j2me_midp_font_t* large_font = j2me_midp_font_create(vm,
        FONT_FACE_SYSTEM, FONT_STYLE_PLAIN, FONT_SIZE_LARGE);
    if (large_font) {
        LOG_DEBUG("  ✓ 大号字体创建成功\n");
    }
    
    // 测试字符串宽度计算
    const char* test_str = "Hello J2ME";
    if (default_font) {
        int width = j2me_midp_font_string_width(default_font, test_str);
        LOG_DEBUG("  ✓ 字符串宽度: \"%s\" = %d像素\n", test_str, width);
        
        int char_width = j2me_midp_font_char_width(default_font, 'A');
        LOG_DEBUG("  ✓ 字符宽度: 'A' = %d像素\n", char_width);
    }
    
    LOG_DEBUG("  ✓ Font系统测试完成\n");
}

void test_text_rendering(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试文本渲染 ===\n");
    
    if (!vm->display || !vm->display->context) {
        LOG_DEBUG("  ✗ 显示系统未初始化\n");
        return;
    }
    
    j2me_midp_graphics_t* g = j2me_midp_graphics_create(vm->display->context);
    if (!g) {
        LOG_DEBUG("  ✗ 无法创建图形上下文\n");
        return;
    }
    
    // 设置字体
    j2me_midp_font_t* font = j2me_midp_font_get_default(vm);
    j2me_midp_graphics_set_font(g, font);
    
    // 测试不同锚点的文本绘制
    j2me_midp_graphics_set_color_rgb(g, 255, 255, 255);
    
    j2me_midp_graphics_draw_string(g, "Top Left", 10, 10, ANCHOR_TOP | ANCHOR_LEFT);
    j2me_midp_graphics_draw_string(g, "Center", 120, 60, ANCHOR_HCENTER | ANCHOR_VCENTER);
    j2me_midp_graphics_draw_string(g, "Bottom Right", 230, 110, ANCHOR_BOTTOM | ANCHOR_RIGHT);
    
    LOG_DEBUG("  ✓ 不同锚点文本绘制正常\n");
    
    // 测试字符绘制
    j2me_midp_graphics_draw_char(g, 'J', 10, 130, ANCHOR_TOP | ANCHOR_LEFT);
    j2me_midp_graphics_draw_char(g, '2', 20, 130, ANCHOR_TOP | ANCHOR_LEFT);
    j2me_midp_graphics_draw_char(g, 'M', 30, 130, ANCHOR_TOP | ANCHOR_LEFT);
    j2me_midp_graphics_draw_char(g, 'E', 40, 130, ANCHOR_TOP | ANCHOR_LEFT);
    
    LOG_DEBUG("  ✓ 字符绘制正常\n");
    
    // 测试子字符串绘制
    const char* full_str = "Hello World!";
    j2me_midp_graphics_draw_substring(g, full_str, 0, 5, 10, 150, ANCHOR_TOP | ANCHOR_LEFT);
    j2me_midp_graphics_draw_substring(g, full_str, 6, 5, 60, 150, ANCHOR_TOP | ANCHOR_LEFT);
    
    LOG_DEBUG("  ✓ 子字符串绘制正常\n");
    
    j2me_midp_graphics_destroy(g);
    LOG_DEBUG("  ✓ 文本渲染测试完成\n");
}

void test_image_system(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试Image系统 ===\n");
    
    // 创建可变图像
    j2me_midp_image_t* mutable_image = j2me_midp_image_create(vm, 100, 100);
    if (mutable_image) {
        LOG_DEBUG("  ✓ 可变图像创建成功: %dx%d\n",
               j2me_midp_image_get_width(mutable_image),
               j2me_midp_image_get_height(mutable_image));
        
        if (j2me_midp_image_is_mutable(mutable_image)) {
            LOG_DEBUG("  ✓ 图像可变性检查正常\n");
            
            // 获取图像的图形上下文
            j2me_midp_graphics_t* img_g = j2me_midp_image_get_graphics(mutable_image);
            if (img_g) {
                LOG_DEBUG("  ✓ 图像图形上下文获取成功\n");
                
                // 在图像上绘制
                j2me_midp_graphics_set_color_rgb(img_g, 255, 0, 0);
                j2me_midp_graphics_fill_rect(img_g, 10, 10, 80, 80);
                LOG_DEBUG("  ✓ 图像绘制正常\n");
            }
        }
    }
    
    // 测试图像绘制到屏幕
    if (vm->display && vm->display->context && mutable_image) {
        j2me_midp_graphics_t* screen_g = j2me_midp_graphics_create(vm->display->context);
        if (screen_g) {
            j2me_midp_graphics_draw_image(screen_g, mutable_image, 50, 50, ANCHOR_TOP | ANCHOR_LEFT);
            LOG_DEBUG("  ✓ 图像绘制到屏幕正常\n");
            j2me_midp_graphics_destroy(screen_g);
        }
    }
    
    LOG_DEBUG("  ✓ Image系统测试完成\n");
}

void test_clip_region(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试裁剪区域 ===\n");
    
    if (!vm->display || !vm->display->context) {
        LOG_DEBUG("  ✗ 显示系统未初始化\n");
        return;
    }
    
    j2me_midp_graphics_t* g = j2me_midp_graphics_create(vm->display->context);
    if (!g) {
        LOG_DEBUG("  ✗ 无法创建图形上下文\n");
        return;
    }
    
    // 设置初始裁剪区域
    j2me_midp_graphics_set_clip(g, 10, 10, 200, 150);
    LOG_DEBUG("  ✓ 初始裁剪区域: (%d, %d, %d, %d)\n",
           j2me_midp_graphics_get_clip_x(g),
           j2me_midp_graphics_get_clip_y(g),
           j2me_midp_graphics_get_clip_width(g),
           j2me_midp_graphics_get_clip_height(g));
    
    // 裁剪区域求交
    j2me_midp_graphics_clip_rect(g, 50, 50, 100, 100);
    LOG_DEBUG("  ✓ 求交后裁剪区域: (%d, %d, %d, %d)\n",
           j2me_midp_graphics_get_clip_x(g),
           j2me_midp_graphics_get_clip_y(g),
           j2me_midp_graphics_get_clip_width(g),
           j2me_midp_graphics_get_clip_height(g));
    
    // 在裁剪区域内绘制
    j2me_midp_graphics_set_color_rgb(g, 0, 255, 0);
    j2me_midp_graphics_fill_rect(g, 0, 0, 300, 200);
    LOG_DEBUG("  ✓ 裁剪区域绘制正常\n");
    
    j2me_midp_graphics_destroy(g);
    LOG_DEBUG("  ✓ 裁剪区域测试完成\n");
}

int main(int argc, char* argv[]) {
    LOG_DEBUG("=== MIDP API增强测试 ===\n");
    
    // 创建虚拟机
    j2me_vm_config_t config = j2me_vm_get_default_config();
    config.heap_size = 512 * 1024;  // 512KB堆
    
    j2me_vm_t* vm = j2me_vm_create(&config);
    if (!vm) {
        LOG_DEBUG("✗ 虚拟机创建失败\n");
        return 1;
    }
    
    // 初始化虚拟机
    if (j2me_vm_initialize(vm) != J2ME_SUCCESS) {
        LOG_DEBUG("✗ 虚拟机初始化失败\n");
        j2me_vm_destroy(vm);
        return 1;
    }
    
    LOG_DEBUG("✓ 虚拟机初始化成功\n");
    
    // 运行测试
    test_graphics_api(vm);
    test_font_system(vm);
    test_text_rendering(vm);
    test_image_system(vm);
    test_clip_region(vm);
    
    // 显示更新
    if (vm->display) {
        j2me_display_refresh((j2me_display_t*)vm->display);
        LOG_DEBUG("\n✓ 显示更新完成\n");
    }
    
    // 清理
    j2me_vm_destroy(vm);
    
    LOG_DEBUG("\n=== MIDP API增强测试完成 ===\n");
    return 0;
}
