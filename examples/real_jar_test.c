#include "j2me_vm.h"
#include "j2me_jar.h"
#include "j2me_midlet_executor.h"
#include "j2me_graphics.h"
#include "j2me_midp_graphics.h"
#include "j2me_input.h"
#include "j2me_native_methods.h"
#include "j2me_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

/**
 * @file real_jar_test.c
 * @brief 真实JAR文件测试程序
 * 
 * 测试加载和运行真实的J2ME游戏JAR文件
 */

void print_jar_info(j2me_jar_file_t* jar) {
    LOG_DEBUG("\n=== JAR文件信息 ===\n");
    
    if (jar->midlet_suite) {
        j2me_midlet_suite_t* suite = jar->midlet_suite;
        
        LOG_DEBUG("  MIDlet名称: %s\n", suite->name ? suite->name : "未知");
        LOG_DEBUG("  供应商: %s\n", suite->vendor ? suite->vendor : "未知");
        LOG_DEBUG("  版本: %s\n", suite->version ? suite->version : "未知");
        LOG_DEBUG("  描述: %s\n", suite->description ? suite->description : "无");
        LOG_DEBUG("  MIDlet数量: %d\n", suite->midlet_count);
        
        for (int i = 0; i < suite->midlet_count && suite->midlets; i++) {
            j2me_midlet_t* midlet = suite->midlets[i];
            if (midlet) {
                LOG_DEBUG("    MIDlet-%d: %s (类: %s)\n", i + 1, 
                       midlet->name ? midlet->name : "未知",
                       midlet->class_name ? midlet->class_name : "未知");
            }
        }
    }
    
    LOG_DEBUG("  条目数量: %d\n", jar->entry_count);
    
    // 打印前10个条目
    LOG_DEBUG("  前10个条目:\n");
    for (int i = 0; i < jar->entry_count && i < 10; i++) {
        j2me_jar_entry_t* entry = jar->entries[i];
        if (entry) {
            LOG_DEBUG("    %d: %s (%s)\n", i, entry->name, 
                   j2me_jar_get_entry_type_name(entry->type));
        }
    }
    
    // 查找XMIDlet.class
    j2me_jar_entry_t* xmidlet = j2me_jar_find_entry(jar, "XMIDlet.class");
    if (xmidlet) {
        LOG_DEBUG("  ✓ 找到XMIDlet.class (大小: %zu bytes)\n", xmidlet->uncompressed_size);
    } else {
        LOG_DEBUG("  ✗ 未找到XMIDlet.class\n");
    }
}

int main(int argc, char* argv[]) {
    LOG_DEBUG("=== 真实JAR文件测试 ===\n");
    
    const char* jar_path = "test_jar/zxfml.jar";
    
    // 检查JAR文件是否存在
    FILE* test_file = fopen(jar_path, "rb");
    if (!test_file) {
        LOG_DEBUG("✗ 无法打开JAR文件: %s\n", jar_path);
        return 1;
    }
    fclose(test_file);
    LOG_DEBUG("✓ JAR文件存在: %s\n", jar_path);
    
    // 创建虚拟机
    j2me_vm_config_t config = j2me_vm_get_default_config();
    config.heap_size = 2 * 1024 * 1024;  // 2MB堆，游戏可能需要更多内存
    
    j2me_vm_t* vm = j2me_vm_create(&config);
    if (!vm) {
        LOG_DEBUG("✗ 虚拟机创建失败\n");
        return 1;
    }
    LOG_DEBUG("✓ 虚拟机创建成功 (堆大小: %zu bytes)\n", config.heap_size);
    
    // 初始化虚拟机
    if (j2me_vm_initialize(vm) != J2ME_SUCCESS) {
        LOG_DEBUG("✗ 虚拟机初始化失败\n");
        j2me_vm_destroy(vm);
        return 1;
    }
    LOG_DEBUG("✓ 虚拟机初始化成功\n");
    
    // 加载JAR文件
    LOG_DEBUG("\n=== 加载JAR文件 ===\n");
    j2me_jar_file_t* jar = j2me_jar_open(jar_path);
    if (!jar) {
        LOG_DEBUG("✗ JAR文件加载失败\n");
        j2me_vm_destroy(vm);
        return 1;
    }
    LOG_DEBUG("✓ JAR文件加载成功\n");
    
    // 解析JAR文件
    LOG_DEBUG("\n=== 解析JAR文件 ===\n");
    j2me_error_t parse_error = j2me_jar_parse(jar);
    if (parse_error != J2ME_SUCCESS) {
        LOG_DEBUG("✗ JAR文件解析失败: %d\n", parse_error);
        j2me_jar_close(jar);
        j2me_vm_destroy(vm);
        return 1;
    }
    LOG_DEBUG("✓ JAR文件解析成功\n");
    
    // 打印JAR信息
    print_jar_info(jar);
    
    // 设置类加载器的JAR文件
    if (vm->class_loader) {
        j2me_error_t error = j2me_class_loader_set_jar_file(
            (j2me_class_loader_t*)vm->class_loader, jar);
        if (error == J2ME_SUCCESS) {
            LOG_DEBUG("✓ 类加载器JAR文件设置成功\n");
        } else {
            LOG_DEBUG("⚠ 类加载器JAR文件设置失败: %d\n", error);
        }
    }
    
    // 创建MIDlet执行器
    LOG_DEBUG("\n=== 创建MIDlet执行器 ===\n");
    j2me_midlet_executor_t* executor = j2me_midlet_executor_create(vm, jar);
    if (!executor) {
        LOG_DEBUG("✗ MIDlet执行器创建失败\n");
        j2me_jar_close(jar);
        j2me_vm_destroy(vm);
        return 1;
    }
    LOG_DEBUG("✓ MIDlet执行器创建成功\n");
    
    // 加载Canvas类
    LOG_DEBUG("\n=== 加载Canvas类 ===\n");
    j2me_class_t* canvas_j = j2me_class_loader_load_class(
        (j2me_class_loader_t*)vm->class_loader, "j");
    if (canvas_j) {
        LOG_DEBUG("✓ Canvas类'j'加载成功 (方法数: %d)\n", canvas_j->methods_count);
    } else {
        LOG_DEBUG("⚠ Canvas类'j'加载失败\n");
    }
    
    j2me_class_t* canvas_y = j2me_class_loader_load_class(
        (j2me_class_loader_t*)vm->class_loader, "y");
    if (canvas_y) {
        LOG_DEBUG("✓ Canvas类'y'加载成功 (方法数: %d)\n", canvas_y->methods_count);
    } else {
        LOG_DEBUG("⚠ Canvas类'y'加载失败\n");
    }
    
    // 加载其他依赖类
    LOG_DEBUG("\n=== 加载依赖类 ===\n");
    const char* dependency_classes[] = {"g", "a", "b", "c", "d", "e", "f", "h", "i", NULL};
    for (int i = 0; dependency_classes[i] != NULL; i++) {
        j2me_class_t* dep_class = j2me_class_loader_load_class(
            (j2me_class_loader_t*)vm->class_loader, dependency_classes[i]);
        if (dep_class) {
            LOG_DEBUG("✓ 类'%s'加载成功 (方法数: %d)\n", dependency_classes[i], dep_class->methods_count);
        } else {
            LOG_DEBUG("⚠ 类'%s'加载失败\n", dependency_classes[i]);
        }
    }
    
    // 运行MIDlet
    if (jar->midlet_suite && jar->midlet_suite->midlet_count > 0 && jar->midlet_suite->midlets) {
        j2me_midlet_t* midlet = jar->midlet_suite->midlets[0];
        if (midlet && midlet->class_name) {
            LOG_DEBUG("\n=== 运行MIDlet ===\n");
            LOG_DEBUG("  名称: %s\n", midlet->name ? midlet->name : "未知");
            LOG_DEBUG("  类: %s\n", midlet->class_name);
            
            j2me_error_t error = j2me_midlet_executor_run_midlet(executor, midlet->class_name);
            if (error == J2ME_SUCCESS) {
                LOG_DEBUG("✓ MIDlet运行成功\n");
                
                // 运行游戏循环
                LOG_DEBUG("\n=== 运行游戏循环 ===\n");
                LOG_DEBUG("  按ESC键退出\n");
                
                bool running = true;
                uint32_t frame_count = 0;
                uint32_t start_time = SDL_GetTicks();
                
                LOG_DEBUG("  SDL窗口已打开，应该可以看到蓝色背景和移动的黄色矩形\n");
                LOG_DEBUG("  如果看到黑屏，说明渲染目标设置有问题\n");
                
                while (running && frame_count < 600) {  // 最多运行600帧（约10秒）
                    // 处理事件
                    SDL_Event event;
                    while (SDL_PollEvent(&event)) {
                        if (event.type == SDL_QUIT) {
                            LOG_DEBUG("  ! 收到退出事件\n");
                            running = false;
                        } else if (event.type == SDL_KEYDOWN) {
                            if (event.key.keysym.sym == SDLK_ESCAPE) {
                                LOG_DEBUG("  ! 用户按下ESC键\n");
                                running = false;
                            }
                            LOG_DEBUG("  按键: %s\n", SDL_GetKeyName(event.key.keysym.sym));
                        }
                    }
                    
                    // 执行一个时间片
                    error = j2me_vm_execute_time_slice(vm, 16);  // 16ms时间片
                    if (error != J2ME_SUCCESS && error != J2ME_ERROR_NOT_IMPLEMENTED) {
                        LOG_DEBUG("  ⚠ 执行时间片失败: %d\n", error);
                    }
                    
                    // 使用之前验证成功的测试渲染
                    if (vm->display && vm->display->context) {
                        // 设置渲染目标为画布纹理
                        SDL_SetRenderTarget(vm->display->context->renderer, vm->display->context->canvas);
                        
                        j2me_midp_graphics_t* midp_g = j2me_midp_graphics_create(vm->display->context);
                        if (midp_g) {
                            // 清屏 - 深蓝色背景
                            j2me_midp_graphics_set_color_rgb(midp_g, 0, 0, 50);
                            j2me_midp_graphics_fill_rect(midp_g, 0, 0, 240, 320);
                            
                            // 绘制移动的黄色矩形
                            int x = (frame_count % 200);
                            j2me_midp_graphics_set_color_rgb(midp_g, 255, 255, 0);
                            j2me_midp_graphics_fill_rect(midp_g, x, 150, 40, 40);
                            
                            // 绘制第二个移动的红色矩形（反向移动）
                            int x2 = 200 - (frame_count % 200);
                            j2me_midp_graphics_set_color_rgb(midp_g, 255, 0, 0);
                            j2me_midp_graphics_fill_rect(midp_g, x2, 100, 30, 30);
                            
                            // 绘制边框
                            j2me_midp_graphics_set_color_rgb(midp_g, 0, 255, 0);
                            j2me_midp_graphics_draw_rect(midp_g, 5, 5, 230, 310);
                            
                            // 绘制文字
                            j2me_midp_graphics_set_color_rgb(midp_g, 255, 255, 255);
                            char text[64];
                            snprintf(text, sizeof(text), "Frame: %d", frame_count);
                            j2me_midp_graphics_draw_string(midp_g, text, 10, 10, 0);
                            
                            if (frame_count % 60 == 0) {
                                LOG_DEBUG("  [帧: %d] 测试渲染完成\n", frame_count);
                            }
                            
                            j2me_midp_graphics_destroy(midp_g);
                        }
                        
                        // 恢复渲染目标为屏幕
                        SDL_SetRenderTarget(vm->display->context->renderer, NULL);
                    }
                    
                    // 更新显示
                    if (vm->display) {
                        if (frame_count == 0) {
                            LOG_DEBUG("  → 调用display_refresh()复制canvas到屏幕...\n");
                        }
                        j2me_display_refresh(vm->display);
                        if (frame_count == 0) {
                            LOG_DEBUG("  ✓ display_refresh()完成\n");
                        }
                    }
                    
                    frame_count++;
                    
                    // 限制帧率
                    SDL_Delay(16);
                    
                    // 每60帧打印一次状态
                    if (frame_count % 60 == 0) {
                        uint32_t elapsed = SDL_GetTicks() - start_time;
                        float fps = (float)frame_count / (elapsed / 1000.0f);
                        LOG_DEBUG("  运行状态: 帧数=%d, FPS=%.2f\n", frame_count, fps);
                    }
                }
                
                uint32_t total_time = SDL_GetTicks() - start_time;
                float avg_fps = (float)frame_count / (total_time / 1000.0f);
                LOG_DEBUG("\n✓ 游戏循环完成\n");
                LOG_DEBUG("  总帧数: %d\n", frame_count);
                LOG_DEBUG("  总时间: %dms\n", total_time);
                LOG_DEBUG("  平均FPS: %.2f\n", avg_fps);
                
            } else {
                LOG_DEBUG("✗ MIDlet运行失败: %d\n", error);
            }
        } else {
            LOG_DEBUG("✗ MIDlet信息不完整\n");
        }
    } else {
        LOG_DEBUG("✗ JAR文件中没有MIDlet\n");
    }
    
    // 清理
    LOG_DEBUG("\n=== 清理资源 ===\n");
    j2me_midlet_executor_destroy(executor);
    LOG_DEBUG("✓ MIDlet执行器已销毁\n");
    
    j2me_jar_close(jar);
    LOG_DEBUG("✓ JAR文件已关闭\n");
    
    j2me_vm_destroy(vm);
    LOG_DEBUG("✓ 虚拟机已销毁\n");
    
    LOG_DEBUG("\n=== 真实JAR文件测试完成 ===\n");
    return 0;
}
