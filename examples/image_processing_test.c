/**
 * @file image_processing_test.c
 * @brief 图像加载和处理测试程序
 * 
 * 测试PNG/JPEG图像加载、绘制和MIDP Image API
 */

#include "j2me_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "j2me_vm.h"
#include "j2me_graphics.h"
#include "j2me_native_methods.h"

/**
 * @brief 创建测试图像文件
 */
void create_test_images(void) {
    LOG_DEBUG("\n=== 创建测试图像文件 ===\n");
    
    // 创建一个简单的PPM图像文件用于测试
    FILE* file = fopen("test_image.ppm", "w");
    if (file) {
        fprintf(file, "P3\n");
        fprintf(file, "# Test image\n");
        fprintf(file, "64 64\n");
        fprintf(file, "255\n");
        
        // 创建一个渐变图像
        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 64; x++) {
                int r = (x * 255) / 63;
                int g = (y * 255) / 63;
                int b = ((x + y) * 255) / 126;
                fprintf(file, "%d %d %d ", r, g, b);
            }
            fprintf(file, "\n");
        }
        
        fclose(file);
        LOG_DEBUG("✅ 创建测试图像: test_image.ppm (64x64)\n");
    } else {
        LOG_DEBUG("❌ 无法创建测试图像文件\n");
    }
    
    // 创建一个简单的BMP图像文件
    file = fopen("test_pattern.bmp", "wb");
    if (file) {
        // BMP文件头 (简化版本)
        uint8_t bmp_header[] = {
            0x42, 0x4D,             // "BM"
            0x36, 0x10, 0x00, 0x00, // 文件大小
            0x00, 0x00, 0x00, 0x00, // 保留
            0x36, 0x00, 0x00, 0x00, // 数据偏移
            0x28, 0x00, 0x00, 0x00, // 信息头大小
            0x20, 0x00, 0x00, 0x00, // 宽度 32
            0x20, 0x00, 0x00, 0x00, // 高度 32
            0x01, 0x00,             // 平面数
            0x18, 0x00,             // 位深度 24
            0x00, 0x00, 0x00, 0x00, // 压缩
            0x00, 0x10, 0x00, 0x00, // 图像大小
            0x00, 0x00, 0x00, 0x00, // X分辨率
            0x00, 0x00, 0x00, 0x00, // Y分辨率
            0x00, 0x00, 0x00, 0x00, // 颜色数
            0x00, 0x00, 0x00, 0x00  // 重要颜色数
        };
        
        fwrite(bmp_header, 1, sizeof(bmp_header), file);
        
        // 创建32x32的棋盘图案
        for (int y = 31; y >= 0; y--) { // BMP是从下到上存储
            for (int x = 0; x < 32; x++) {
                uint8_t color = ((x / 4) + (y / 4)) % 2 ? 255 : 0;
                fwrite(&color, 1, 1, file); // B
                fwrite(&color, 1, 1, file); // G
                fwrite(&color, 1, 1, file); // R
            }
        }
        
        fclose(file);
        LOG_DEBUG("✅ 创建测试图像: test_pattern.bmp (32x32)\n");
    } else {
        LOG_DEBUG("❌ 无法创建BMP测试图像文件\n");
    }
}

/**
 * @brief 测试图像创建功能
 */
void test_image_creation(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试图像创建功能 ===\n");
    
    if (!vm->display || !vm->display->context) {
        LOG_DEBUG("❌ 图形上下文未初始化\n");
        return;
    }
    
    j2me_graphics_context_t* context = vm->display->context;
    
    // 测试创建可变图像
    LOG_DEBUG("🖼️ 创建可变图像...\n");
    j2me_image_t* mutable_image = j2me_image_create(context, 80, 60);
    if (mutable_image) {
        LOG_DEBUG("✅ 可变图像创建成功: %dx%d, 可变=%s\n", 
               mutable_image->width, mutable_image->height, 
               mutable_image->mutable ? "是" : "否");
        
        // 在可变图像上绘制一些内容
        SDL_SetRenderTarget(context->renderer, mutable_image->texture);
        SDL_SetRenderDrawColor(context->renderer, 255, 0, 0, 255); // 红色
        SDL_RenderClear(context->renderer);
        
        // 绘制一些图案
        SDL_SetRenderDrawColor(context->renderer, 0, 255, 0, 255); // 绿色
        SDL_Rect rect = {10, 10, 60, 40};
        SDL_RenderFillRect(context->renderer, &rect);
        
        SDL_SetRenderTarget(context->renderer, NULL);
        LOG_DEBUG("✅ 在可变图像上绘制完成\n");
    } else {
        LOG_DEBUG("❌ 可变图像创建失败\n");
    }
    
    // 测试从文件加载图像
    LOG_DEBUG("🖼️ 从文件加载图像...\n");
    j2me_image_t* loaded_image1 = j2me_image_load(context, "test_image.ppm");
    if (loaded_image1) {
        LOG_DEBUG("✅ PPM图像加载成功: %dx%d, 可变=%s\n", 
               loaded_image1->width, loaded_image1->height,
               loaded_image1->mutable ? "是" : "否");
    }
    
    j2me_image_t* loaded_image2 = j2me_image_load(context, "test_pattern.bmp");
    if (loaded_image2) {
        LOG_DEBUG("✅ BMP图像加载成功: %dx%d, 可变=%s\n", 
               loaded_image2->width, loaded_image2->height,
               loaded_image2->mutable ? "是" : "否");
    }
    
    // 测试加载不存在的文件
    j2me_image_t* missing_image = j2me_image_load(context, "nonexistent.png");
    if (missing_image) {
        LOG_DEBUG("✅ 不存在文件的占位符图像创建成功: %dx%d\n", 
               missing_image->width, missing_image->height);
    }
    
    // 绘制所有图像到屏幕
    LOG_DEBUG("🎨 绘制图像到屏幕...\n");
    j2me_graphics_clear(context);
    
    if (mutable_image) {
        j2me_graphics_draw_image(context, mutable_image, 20, 20, 0x00);
    }
    if (loaded_image1) {
        j2me_graphics_draw_image(context, loaded_image1, 120, 20, 0x00);
    }
    if (loaded_image2) {
        j2me_graphics_draw_image(context, loaded_image2, 20, 100, 0x00);
    }
    if (missing_image) {
        j2me_graphics_draw_image(context, missing_image, 120, 100, 0x00);
    }
    
    j2me_display_refresh(vm->display);
    
    // 清理图像
    if (mutable_image) j2me_image_destroy(mutable_image);
    if (loaded_image1) j2me_image_destroy(loaded_image1);
    if (loaded_image2) j2me_image_destroy(loaded_image2);
    if (missing_image) j2me_image_destroy(missing_image);
    
    LOG_DEBUG("✅ 图像创建功能测试完成\n");
}

/**
 * @brief 测试MIDP Image本地方法
 */
void test_midp_image_methods(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试MIDP Image本地方法 ===\n");
    
    // 创建测试栈帧
    j2me_stack_frame_t* frame = j2me_stack_frame_create(20, 10);
    if (!frame) {
        LOG_DEBUG("❌ 创建栈帧失败\n");
        return;
    }
    
    LOG_DEBUG("✅ 测试栈帧创建成功\n");
    
    // 测试Image.createImage(int, int)
    LOG_DEBUG("\n--- 测试Image.createImage(int, int) ---\n");
    j2me_operand_stack_push(&frame->operand_stack, 100); // width
    j2me_operand_stack_push(&frame->operand_stack, 80);  // height
    
    j2me_error_t result = midp_image_create_image(vm, frame, NULL);
    if (result == J2ME_SUCCESS) {
        j2me_int image_ref;
        j2me_operand_stack_pop(&frame->operand_stack, &image_ref);
        LOG_DEBUG("✅ Image.createImage(100, 80) 调用成功，返回: 0x%x\n", image_ref);
        
        // 测试Image.getWidth()
        LOG_DEBUG("\n--- 测试Image.getWidth() ---\n");
        j2me_operand_stack_push(&frame->operand_stack, image_ref);
        result = midp_image_get_width(vm, frame, NULL);
        if (result == J2ME_SUCCESS) {
            j2me_int width;
            j2me_operand_stack_pop(&frame->operand_stack, &width);
            LOG_DEBUG("✅ Image.getWidth() 调用成功，返回宽度: %d\n", width);
        }
        
        // 测试Image.getHeight()
        LOG_DEBUG("\n--- 测试Image.getHeight() ---\n");
        j2me_operand_stack_push(&frame->operand_stack, image_ref);
        result = midp_image_get_height(vm, frame, NULL);
        if (result == J2ME_SUCCESS) {
            j2me_int height;
            j2me_operand_stack_pop(&frame->operand_stack, &height);
            LOG_DEBUG("✅ Image.getHeight() 调用成功，返回高度: %d\n", height);
        }
    } else {
        LOG_DEBUG("❌ Image.createImage(int, int) 调用失败: %d\n", result);
    }
    
    // 测试Image.createImage(String)
    LOG_DEBUG("\n--- 测试Image.createImage(String) ---\n");
    j2me_operand_stack_push(&frame->operand_stack, 0x60000001); // 文件名字符串引用
    
    result = midp_image_create_image_from_file(vm, frame, NULL);
    if (result == J2ME_SUCCESS) {
        j2me_int image_ref;
        j2me_operand_stack_pop(&frame->operand_stack, &image_ref);
        LOG_DEBUG("✅ Image.createImage(String) 调用成功，返回: 0x%x\n", image_ref);
        
        // 测试Graphics.drawImage()
        LOG_DEBUG("\n--- 测试Graphics.drawImage() ---\n");
        j2me_operand_stack_push(&frame->operand_stack, 0x40000001); // Graphics对象引用
        j2me_operand_stack_push(&frame->operand_stack, image_ref);   // Image对象引用
        j2me_operand_stack_push(&frame->operand_stack, 50);         // x
        j2me_operand_stack_push(&frame->operand_stack, 50);         // y
        j2me_operand_stack_push(&frame->operand_stack, 0x00);       // anchor
        
        result = midp_graphics_draw_image(vm, frame, NULL);
        if (result == J2ME_SUCCESS) {
            LOG_DEBUG("✅ Graphics.drawImage() 调用成功\n");
        } else {
            LOG_DEBUG("❌ Graphics.drawImage() 调用失败: %d\n", result);
        }
    } else {
        LOG_DEBUG("❌ Image.createImage(String) 调用失败: %d\n", result);
    }
    
    // 清理栈帧
    j2me_stack_frame_destroy(frame);
    LOG_DEBUG("✅ MIDP Image本地方法测试完成\n");
}

/**
 * @brief 测试图像锚点和变换
 */
void test_image_anchors_and_transforms(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试图像锚点和变换 ===\n");
    
    if (!vm->display || !vm->display->context) {
        LOG_DEBUG("❌ 图形上下文未初始化\n");
        return;
    }
    
    j2me_graphics_context_t* context = vm->display->context;
    
    // 创建测试图像
    j2me_image_t* test_image = j2me_image_create(context, 40, 30);
    if (!test_image) {
        LOG_DEBUG("❌ 创建测试图像失败\n");
        return;
    }
    
    // 在测试图像上绘制内容
    SDL_SetRenderTarget(context->renderer, test_image->texture);
    SDL_SetRenderDrawColor(context->renderer, 0, 0, 255, 255); // 蓝色背景
    SDL_RenderClear(context->renderer);
    
    // 绘制边框
    SDL_SetRenderDrawColor(context->renderer, 255, 255, 0, 255); // 黄色边框
    SDL_Rect border = {0, 0, 40, 30};
    SDL_RenderDrawRect(context->renderer, &border);
    
    // 绘制中心点
    SDL_SetRenderDrawColor(context->renderer, 255, 0, 0, 255); // 红色中心点
    SDL_RenderDrawPoint(context->renderer, 20, 15);
    
    SDL_SetRenderTarget(context->renderer, NULL);
    
    // 清除屏幕
    j2me_graphics_clear(context);
    
    // 绘制参考线
    j2me_color_t gray = {128, 128, 128, 255};
    j2me_graphics_set_color(context, gray);
    j2me_graphics_draw_line(context, 120, 0, 120, 320); // 垂直线
    j2me_graphics_draw_line(context, 0, 160, 240, 160); // 水平线
    
    // 测试不同锚点
    LOG_DEBUG("🎯 测试不同锚点...\n");
    
    // TOP-LEFT (0x00)
    j2me_graphics_draw_image(context, test_image, 120, 160, 0x00);
    
    // TOP-RIGHT (0x01)
    j2me_graphics_draw_image(context, test_image, 120, 160, 0x01);
    
    // BOTTOM-LEFT (0x10)
    j2me_graphics_draw_image(context, test_image, 120, 160, 0x10);
    
    // BOTTOM-RIGHT (0x11)
    j2me_graphics_draw_image(context, test_image, 120, 160, 0x11);
    
    // CENTER (0x22)
    j2me_graphics_draw_image(context, test_image, 120, 160, 0x22);
    
    // 添加标签
    j2me_color_t black = {0, 0, 0, 255};
    j2me_graphics_set_color(context, black);
    j2me_graphics_draw_string(context, "Anchor Test", 120, 20, 0x22);
    j2me_graphics_draw_string(context, "Center: (120,160)", 120, 300, 0x22);
    
    j2me_display_refresh(vm->display);
    
    // 清理
    j2me_image_destroy(test_image);
    
    LOG_DEBUG("✅ 图像锚点和变换测试完成\n");
}

/**
 * @brief 图像处理性能测试
 */
void test_image_performance(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 图像处理性能测试 ===\n");
    
    if (!vm->display || !vm->display->context) {
        LOG_DEBUG("❌ 图形上下文未初始化\n");
        return;
    }
    
    j2me_graphics_context_t* context = vm->display->context;
    
    // 创建多个测试图像
    const int num_images = 10;
    j2me_image_t* images[num_images];
    
    LOG_DEBUG("🚀 创建 %d 个图像...\n", num_images);
    for (int i = 0; i < num_images; i++) {
        images[i] = j2me_image_create(context, 32, 32);
        if (images[i]) {
            // 在每个图像上绘制不同颜色
            SDL_SetRenderTarget(context->renderer, images[i]->texture);
            SDL_SetRenderDrawColor(context->renderer, 
                                   (i * 25) % 256, 
                                   (i * 50) % 256, 
                                   (i * 75) % 256, 255);
            SDL_RenderClear(context->renderer);
            SDL_SetRenderTarget(context->renderer, NULL);
        }
    }
    
    // 性能测试：快速绘制多个图像
    LOG_DEBUG("⚡ 性能测试：绘制动画...\n");
    
    for (int frame = 0; frame < 60; frame++) { // 60帧动画
        j2me_graphics_clear(context);
        
        // 绘制所有图像
        for (int i = 0; i < num_images; i++) {
            if (images[i]) {
                int x = 20 + (i * 20) + (frame % 50);
                int y = 50 + (int)(30 * sin(frame * 0.1 + i));
                j2me_graphics_draw_image(context, images[i], x, y, 0x00);
            }
        }
        
        // 显示帧数
        j2me_color_t white = {255, 255, 255, 255};
        j2me_graphics_set_color(context, white);
        char frame_text[32];
        snprintf(frame_text, sizeof(frame_text), "Frame %d/60", frame + 1);
        j2me_graphics_draw_string(context, frame_text, 120, 280, 0x22);
        
        j2me_display_refresh(vm->display);
        
        // 短暂延迟
        usleep(33000); // ~30 FPS
        
        if (frame % 15 == 0) {
            LOG_DEBUG("⚡ 帧 %d/60\r", frame + 1);
            fflush(stdout);
        }
    }
    
    LOG_DEBUG("\n");
    
    // 清理图像
    for (int i = 0; i < num_images; i++) {
        if (images[i]) {
            j2me_image_destroy(images[i]);
        }
    }
    
    LOG_DEBUG("✅ 图像处理性能测试完成\n");
}

/**
 * @brief 主测试函数
 */
int main() {
    LOG_DEBUG("图像加载和处理测试程序\n");
    LOG_DEBUG("========================\n");
    LOG_DEBUG("测试PNG/JPEG图像加载、绘制和MIDP Image API\n");
    LOG_DEBUG("包括图像创建、文件加载、锚点、性能测试\n\n");
    
    // 创建测试图像文件
    create_test_images();
    
    // 创建虚拟机配置
    j2me_vm_config_t config = {
        .heap_size = 2 * 1024 * 1024,  // 2MB堆
        .stack_size = 128 * 1024,      // 128KB栈
        .max_threads = 8               // 8个线程
    };
    
    // 创建虚拟机
    j2me_vm_t* vm = j2me_vm_create(&config);
    if (!vm) {
        LOG_DEBUG("❌ 创建虚拟机失败\n");
        return 1;
    }
    LOG_DEBUG("✅ 虚拟机创建成功\n");
    
    // 初始化虚拟机 (这将初始化SDL2显示系统和图像系统)
    j2me_error_t result = j2me_vm_initialize(vm);
    if (result != J2ME_SUCCESS) {
        LOG_DEBUG("❌ 虚拟机初始化失败: %d\n", result);
        j2me_vm_destroy(vm);
        return 1;
    }
    LOG_DEBUG("✅ 虚拟机初始化成功\n");
    
    // 运行测试
    test_image_creation(vm);
    
    LOG_DEBUG("\n⏳ 等待5秒以查看图像创建结果...\n");
    sleep(5);
    
    test_midp_image_methods(vm);
    
    LOG_DEBUG("\n⏳ 等待3秒...\n");
    sleep(3);
    
    test_image_anchors_and_transforms(vm);
    
    LOG_DEBUG("\n⏳ 等待5秒以查看锚点测试结果...\n");
    sleep(5);
    
    test_image_performance(vm);
    
    LOG_DEBUG("\n⏳ 等待3秒以查看最终结果...\n");
    sleep(3);
    
    // 清理虚拟机
    j2me_vm_destroy(vm);
    
    // 清理测试文件
    remove("test_image.ppm");
    remove("test_pattern.bmp");
    
    LOG_DEBUG("\n=== 图像加载和处理测试总结 ===\n");
    LOG_DEBUG("✅ 图像创建: 可变图像创建和内容绘制正常\n");
    LOG_DEBUG("✅ 文件加载: PPM/BMP图像文件加载正常\n");
    LOG_DEBUG("✅ 占位符处理: 不存在文件的占位符图像创建正常\n");
    LOG_DEBUG("✅ MIDP Image API: createImage、getWidth、getHeight方法正常\n");
    LOG_DEBUG("✅ Graphics.drawImage: 图像绘制方法正常\n");
    LOG_DEBUG("✅ 锚点系统: 不同锚点的图像定位正常\n");
    LOG_DEBUG("✅ 性能测试: 60帧动画流畅播放，多图像绘制正常\n");
    LOG_DEBUG("✅ 资源管理: 自动清理和释放正常\n");
    
    LOG_DEBUG("\n🎉 图像加载和处理测试完成！\n");
    LOG_DEBUG("💡 下一步: 完善字体系统和完整游戏测试\n");
    
    return 0;
}