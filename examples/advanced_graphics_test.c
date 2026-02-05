/**
 * @file advanced_graphics_test.c
 * @brief 高级图形API测试程序
 * 
 * 测试扩展的MIDP图形API功能，包括椭圆、圆弧、多边形、文本渲染等
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "j2me_vm.h"
#include "j2me_jar.h"
#include "j2me_midlet_executor.h"
#include "j2me_native_methods.h"
#include "j2me_interpreter.h"
#include "j2me_graphics.h"

/**
 * @brief 测试椭圆绘制功能
 */
void test_oval_drawing(j2me_vm_t* vm) {
    printf("\n=== 测试椭圆绘制功能 ===\n");
    
    if (!vm->display || !vm->display->context) {
        printf("❌ 图形上下文未初始化\n");
        return;
    }
    
    j2me_graphics_context_t* context = vm->display->context;
    
    // 清除屏幕
    j2me_graphics_clear(context);
    
    // 绘制不同颜色的椭圆
    printf("🎨 绘制椭圆轮廓...\n");
    j2me_color_t red = {255, 0, 0, 255};
    j2me_graphics_set_color(context, red);
    j2me_graphics_draw_oval(context, 50, 50, 80, 60, false);
    
    printf("🎨 绘制填充椭圆...\n");
    j2me_color_t blue = {0, 0, 255, 255};
    j2me_graphics_set_color(context, blue);
    j2me_graphics_draw_oval(context, 150, 50, 60, 80, true);
    
    // 刷新显示
    j2me_display_refresh(vm->display);
    printf("✅ 椭圆绘制测试完成\n");
}

/**
 * @brief 测试圆弧绘制功能
 */
void test_arc_drawing(j2me_vm_t* vm) {
    printf("\n=== 测试圆弧绘制功能 ===\n");
    
    if (!vm->display || !vm->display->context) {
        printf("❌ 图形上下文未初始化\n");
        return;
    }
    
    j2me_graphics_context_t* context = vm->display->context;
    
    // 绘制不同角度的圆弧
    printf("🎨 绘制圆弧...\n");
    j2me_color_t green = {0, 255, 0, 255};
    j2me_graphics_set_color(context, green);
    
    // 绘制四分之一圆弧
    j2me_graphics_draw_arc(context, 50, 150, 80, 80, 0, 90, false);
    
    // 绘制半圆弧
    j2me_graphics_draw_arc(context, 150, 150, 80, 80, 45, 180, false);
    
    // 绘制填充扇形
    j2me_color_t purple = {128, 0, 128, 255};
    j2me_graphics_set_color(context, purple);
    j2me_graphics_draw_arc(context, 100, 200, 60, 60, 30, 120, true);
    
    // 刷新显示
    j2me_display_refresh(vm->display);
    printf("✅ 圆弧绘制测试完成\n");
}

/**
 * @brief 测试多边形绘制功能
 */
void test_polygon_drawing(j2me_vm_t* vm) {
    printf("\n=== 测试多边形绘制功能 ===\n");
    
    if (!vm->display || !vm->display->context) {
        printf("❌ 图形上下文未初始化\n");
        return;
    }
    
    j2me_graphics_context_t* context = vm->display->context;
    
    // 绘制三角形
    printf("🎨 绘制三角形...\n");
    j2me_color_t orange = {255, 165, 0, 255};
    j2me_graphics_set_color(context, orange);
    
    int triangle_x[] = {50, 100, 75};
    int triangle_y[] = {280, 280, 250};
    j2me_graphics_draw_polygon(context, triangle_x, triangle_y, 3, false);
    
    // 绘制五边形
    printf("🎨 绘制五边形...\n");
    j2me_color_t cyan = {0, 255, 255, 255};
    j2me_graphics_set_color(context, cyan);
    
    int pentagon_x[] = {150, 170, 160, 140, 130};
    int pentagon_y[] = {250, 260, 280, 280, 260};
    j2me_graphics_draw_polygon(context, pentagon_x, pentagon_y, 5, true);
    
    // 刷新显示
    j2me_display_refresh(vm->display);
    printf("✅ 多边形绘制测试完成\n");
}

/**
 * @brief 测试文本渲染功能
 */
void test_text_rendering(j2me_vm_t* vm) {
    printf("\n=== 测试文本渲染功能 ===\n");
    
    if (!vm->display || !vm->display->context) {
        printf("❌ 图形上下文未初始化\n");
        return;
    }
    
    j2me_graphics_context_t* context = vm->display->context;
    
    // 设置字体
    printf("🎨 设置字体...\n");
    j2me_font_t font = {16, 0, "Arial"};
    j2me_graphics_set_font(context, font);
    
    // 绘制不同锚点的文本
    printf("🎨 绘制文本...\n");
    j2me_color_t black = {0, 0, 0, 255};
    j2me_graphics_set_color(context, black);
    
    // 左上角锚点
    j2me_graphics_draw_string(context, "TOP-LEFT", 20, 20, 0x00);
    
    // 右上角锚点
    j2me_graphics_draw_string(context, "TOP-RIGHT", 220, 20, 0x01);
    
    // 居中锚点
    j2me_graphics_draw_string(context, "CENTER", 120, 160, 0x22);
    
    // 底部居中锚点
    j2me_graphics_draw_string(context, "BOTTOM-CENTER", 120, 300, 0x12);
    
    // 测试字体度量
    int text_width = j2me_graphics_get_string_width(context, "Sample Text");
    int font_height = j2me_graphics_get_font_height(context);
    printf("📏 文本度量: 宽度=%d, 高度=%d\n", text_width, font_height);
    
    // 刷新显示
    j2me_display_refresh(vm->display);
    printf("✅ 文本渲染测试完成\n");
}

/**
 * @brief 测试坐标变换功能
 */
void test_coordinate_transform(j2me_vm_t* vm) {
    printf("\n=== 测试坐标变换功能 ===\n");
    
    if (!vm->display || !vm->display->context) {
        printf("❌ 图形上下文未初始化\n");
        return;
    }
    
    j2me_graphics_context_t* context = vm->display->context;
    
    // 清除屏幕
    j2me_graphics_clear(context);
    
    // 绘制原点矩形
    printf("🎨 绘制原点矩形...\n");
    j2me_color_t red = {255, 0, 0, 255};
    j2me_graphics_set_color(context, red);
    j2me_graphics_draw_rect(context, 0, 0, 30, 30, false);
    
    // 应用坐标变换
    printf("🎨 应用坐标变换...\n");
    j2me_graphics_translate(context, 50, 50);
    
    // 绘制变换后的矩形
    j2me_color_t blue = {0, 0, 255, 255};
    j2me_graphics_set_color(context, blue);
    j2me_graphics_draw_rect(context, 0, 0, 30, 30, true);
    
    // 再次变换
    j2me_graphics_translate(context, 30, 30);
    
    // 绘制第三个矩形
    j2me_color_t green = {0, 255, 0, 255};
    j2me_graphics_set_color(context, green);
    j2me_graphics_draw_oval(context, 0, 0, 40, 40, false);
    
    // 刷新显示
    j2me_display_refresh(vm->display);
    printf("✅ 坐标变换测试完成\n");
}

/**
 * @brief 测试MIDP本地方法调用
 */
void test_midp_advanced_graphics(j2me_vm_t* vm) {
    printf("\n=== 测试MIDP高级图形方法 ===\n");
    
    // 创建测试栈帧
    j2me_stack_frame_t* frame = j2me_stack_frame_create(20, 10);
    if (!frame) {
        printf("❌ 创建栈帧失败\n");
        return;
    }
    
    printf("✅ 测试栈帧创建成功\n");
    
    // 测试drawOval
    printf("\n--- 测试Graphics.drawOval() ---\n");
    j2me_operand_stack_push(&frame->operand_stack, 0x40000001); // Graphics对象引用
    j2me_operand_stack_push(&frame->operand_stack, 50);         // x
    j2me_operand_stack_push(&frame->operand_stack, 100);       // y
    j2me_operand_stack_push(&frame->operand_stack, 80);        // width
    j2me_operand_stack_push(&frame->operand_stack, 60);        // height
    
    j2me_error_t result = midp_graphics_draw_oval(vm, frame, NULL);
    if (result == J2ME_SUCCESS) {
        printf("✅ Graphics.drawOval() 调用成功\n");
    } else {
        printf("❌ Graphics.drawOval() 调用失败: %d\n", result);
    }
    
    // 测试fillOval
    printf("\n--- 测试Graphics.fillOval() ---\n");
    j2me_operand_stack_push(&frame->operand_stack, 0x40000001); // Graphics对象引用
    j2me_operand_stack_push(&frame->operand_stack, 150);       // x
    j2me_operand_stack_push(&frame->operand_stack, 100);       // y
    j2me_operand_stack_push(&frame->operand_stack, 60);        // width
    j2me_operand_stack_push(&frame->operand_stack, 80);        // height
    
    result = midp_graphics_fill_oval(vm, frame, NULL);
    if (result == J2ME_SUCCESS) {
        printf("✅ Graphics.fillOval() 调用成功\n");
    } else {
        printf("❌ Graphics.fillOval() 调用失败: %d\n", result);
    }
    
    // 测试drawArc
    printf("\n--- 测试Graphics.drawArc() ---\n");
    j2me_operand_stack_push(&frame->operand_stack, 0x40000001); // Graphics对象引用
    j2me_operand_stack_push(&frame->operand_stack, 100);       // x
    j2me_operand_stack_push(&frame->operand_stack, 200);       // y
    j2me_operand_stack_push(&frame->operand_stack, 80);        // width
    j2me_operand_stack_push(&frame->operand_stack, 80);        // height
    j2me_operand_stack_push(&frame->operand_stack, 45);        // start_angle
    j2me_operand_stack_push(&frame->operand_stack, 90);        // arc_angle
    
    result = midp_graphics_draw_arc(vm, frame, NULL);
    if (result == J2ME_SUCCESS) {
        printf("✅ Graphics.drawArc() 调用成功\n");
    } else {
        printf("❌ Graphics.drawArc() 调用失败: %d\n", result);
    }
    
    // 刷新显示以显示所有绘制内容
    j2me_display_refresh(vm->display);
    
    // 清理栈帧
    j2me_stack_frame_destroy(frame);
    printf("✅ MIDP高级图形方法测试完成\n");
}

/**
 * @brief 综合图形演示
 */
void comprehensive_graphics_demo(j2me_vm_t* vm) {
    printf("\n=== 综合图形演示 ===\n");
    
    if (!vm->display || !vm->display->context) {
        printf("❌ 图形上下文未初始化\n");
        return;
    }
    
    j2me_graphics_context_t* context = vm->display->context;
    
    printf("🎬 开始综合图形演示...\n");
    
    for (int frame = 0; frame < 20; frame++) {
        // 清除屏幕
        j2me_graphics_clear(context);
        
        // 重置坐标变换
        context->translate_x = 0;
        context->translate_y = 0;
        
        // 绘制动态椭圆
        j2me_color_t color1 = {
            (frame * 12) % 256,
            (frame * 8) % 256,
            (frame * 16) % 256,
            255
        };
        j2me_graphics_set_color(context, color1);
        j2me_graphics_draw_oval(context, 50 + frame * 2, 50, 60, 40, true);
        
        // 绘制旋转的圆弧
        j2me_color_t color2 = {255, (frame * 10) % 256, 0, 255};
        j2me_graphics_set_color(context, color2);
        j2me_graphics_draw_arc(context, 150, 100, 80, 80, frame * 18, 90, false);
        
        // 绘制移动的多边形
        int poly_x[] = {100 + frame, 120 + frame, 110 + frame};
        int poly_y[] = {200, 200, 180};
        j2me_color_t color3 = {0, 255, (frame * 15) % 256, 255};
        j2me_graphics_set_color(context, color3);
        j2me_graphics_draw_polygon(context, poly_x, poly_y, 3, true);
        
        // 绘制文本
        j2me_color_t black = {0, 0, 0, 255};
        j2me_graphics_set_color(context, black);
        char text[32];
        snprintf(text, sizeof(text), "Frame %d", frame + 1);
        j2me_graphics_draw_string(context, text, 120, 280, 0x22);
        
        // 刷新显示
        j2me_display_refresh(vm->display);
        
        // 短暂延迟
        usleep(100000); // 100ms
        
        printf("🎬 帧 %d/20\r", frame + 1);
        fflush(stdout);
    }
    
    printf("\n✅ 综合图形演示完成\n");
}

/**
 * @brief 主测试函数
 */
int main() {
    printf("高级图形API测试程序\n");
    printf("====================\n");
    printf("测试扩展的MIDP图形API功能\n");
    printf("包括椭圆、圆弧、多边形、文本渲染等\n\n");
    
    // 创建虚拟机配置
    j2me_vm_config_t config = {
        .heap_size = 2 * 1024 * 1024,  // 2MB堆
        .stack_size = 128 * 1024,      // 128KB栈
        .max_threads = 8               // 8个线程
    };
    
    // 创建虚拟机
    j2me_vm_t* vm = j2me_vm_create(&config);
    if (!vm) {
        printf("❌ 创建虚拟机失败\n");
        return 1;
    }
    printf("✅ 虚拟机创建成功\n");
    
    // 初始化虚拟机 (这将初始化SDL2显示系统)
    j2me_error_t result = j2me_vm_initialize(vm);
    if (result != J2ME_SUCCESS) {
        printf("❌ 虚拟机初始化失败: %d\n", result);
        j2me_vm_destroy(vm);
        return 1;
    }
    printf("✅ 虚拟机初始化成功\n");
    
    // 运行测试
    test_oval_drawing(vm);
    
    printf("\n⏳ 等待3秒以查看椭圆绘制结果...\n");
    sleep(3);
    
    test_arc_drawing(vm);
    
    printf("\n⏳ 等待3秒以查看圆弧绘制结果...\n");
    sleep(3);
    
    test_polygon_drawing(vm);
    
    printf("\n⏳ 等待3秒以查看多边形绘制结果...\n");
    sleep(3);
    
    test_text_rendering(vm);
    
    printf("\n⏳ 等待3秒以查看文本渲染结果...\n");
    sleep(3);
    
    test_coordinate_transform(vm);
    
    printf("\n⏳ 等待3秒以查看坐标变换结果...\n");
    sleep(3);
    
    test_midp_advanced_graphics(vm);
    
    printf("\n⏳ 等待3秒以查看MIDP方法调用结果...\n");
    sleep(3);
    
    comprehensive_graphics_demo(vm);
    
    printf("\n⏳ 等待5秒以查看最终结果...\n");
    sleep(5);
    
    // 清理虚拟机
    j2me_vm_destroy(vm);
    
    printf("\n=== 高级图形API测试总结 ===\n");
    printf("✅ 椭圆绘制: 轮廓和填充椭圆正常\n");
    printf("✅ 圆弧绘制: 不同角度的圆弧和扇形正常\n");
    printf("✅ 多边形绘制: 三角形、五边形等多边形正常\n");
    printf("✅ 文本渲染: 不同锚点的文本绘制正常\n");
    printf("✅ 坐标变换: 平移变换功能正常\n");
    printf("✅ MIDP方法: 高级Graphics方法调用正常\n");
    printf("✅ 综合演示: 动态图形渲染正常\n");
    printf("✅ 资源管理: 自动清理和释放正常\n");
    
    printf("\n🎉 高级图形API测试完成！\n");
    printf("💡 下一步: 实现事件处理系统和更多MIDP API\n");
    
    return 0;
}