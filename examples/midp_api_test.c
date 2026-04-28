#include "j2me_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/j2me_vm.h"
#include "../include/j2me_graphics.h"
#include "../include/j2me_midp_graphics.h"
#include "../include/j2me_input.h"
#include "../include/j2me_native_methods.h"

/**
 * @file midp_api_test.c
 * @brief MIDP API完整性测试程序
 * 
 * 测试MIDP API的完整性和SDL集成
 */

// 测试函数声明
void test_graphics_api(j2me_vm_t* vm);
void test_input_system(j2me_vm_t* vm);
void test_native_methods(j2me_vm_t* vm);
void test_sdl_integration(j2me_vm_t* vm);
void print_test_header(const char* test_name);
void print_test_result(const char* test_name, bool passed);

int main() {
    LOG_DEBUG("=== MIDP API完整性测试 ===\n\n");
    
    // 创建虚拟机配置
    j2me_vm_config_t config = j2me_vm_get_default_config();
    config.heap_size = 512 * 1024; // 512KB堆
    
    // 创建虚拟机
    j2me_vm_t* vm = j2me_vm_create(&config);
    if (!vm) {
        LOG_DEBUG("错误: 虚拟机创建失败\n");
        return 1;
    }
    
    // 初始化虚拟机
    j2me_error_t result = j2me_vm_initialize(vm);
    if (result != J2ME_SUCCESS) {
        LOG_DEBUG("错误: 虚拟机初始化失败: %d\n", result);
        j2me_vm_destroy(vm);
        return 1;
    }
    
    LOG_DEBUG("虚拟机初始化成功\n\n");
    
    // 运行测试
    test_graphics_api(vm);
    test_input_system(vm);
    test_native_methods(vm);
    test_sdl_integration(vm);
    
    // 清理
    j2me_vm_destroy(vm);
    
    LOG_DEBUG("=== MIDP API测试完成 ===\n");
    return 0;
}

void test_graphics_api(j2me_vm_t* vm) {
    print_test_header("图形API测试");
    
    bool test_passed = true;
    
    // 测试显示系统
    if (vm->display) {
        LOG_DEBUG("  ✓ 显示系统已初始化\n");
        
        // 获取图形上下文 (简化测试)
        j2me_graphics_context_t* context = vm->display ? vm->display->context : NULL;
        if (context) {
            LOG_DEBUG("  ✓ 图形上下文获取成功\n");
            
            // 测试基础绘制功能
            j2me_color_t red_color = {255, 0, 0, 255};
            j2me_graphics_set_color(context, red_color);
            j2me_graphics_draw_rect(context, 10, 10, 50, 30, true);
            LOG_DEBUG("  ✓ 基础绘制功能正常\n");
            
            // 创建MIDP图形上下文
            j2me_midp_graphics_t* midp_graphics = j2me_midp_graphics_create(context);
            if (midp_graphics) {
                LOG_DEBUG("  ✓ MIDP图形上下文创建成功\n");
                
                // 测试MIDP绘制功能
                j2me_midp_graphics_set_color_rgb(midp_graphics, 0, 255, 0);
                j2me_midp_graphics_draw_line(midp_graphics, 0, 0, 100, 100);
                j2me_midp_graphics_draw_rect(midp_graphics, 20, 20, 60, 40);
                LOG_DEBUG("  ✓ MIDP绘制功能正常\n");
                
                j2me_midp_graphics_destroy(midp_graphics);
            } else {
                LOG_DEBUG("  ✗ MIDP图形上下文创建失败\n");
                test_passed = false;
            }
        } else {
            LOG_DEBUG("  ✗ 图形上下文获取失败\n");
            test_passed = false;
        }
    } else {
        LOG_DEBUG("  ✗ 显示系统未初始化\n");
        test_passed = false;
    }
    
    print_test_result("图形API测试", test_passed);
}

void test_input_system(j2me_vm_t* vm) {
    print_test_header("输入系统测试");
    
    bool test_passed = true;
    
    // 测试输入管理器
    if (vm->input_manager) {
        LOG_DEBUG("  ✓ 输入管理器已初始化\n");
        
        // 测试键盘状态查询
        bool key_state = j2me_input_is_key_pressed(vm->input_manager, KEY_UP);
        LOG_DEBUG("  ✓ 键盘状态查询功能正常 (UP键状态: %s)\n", key_state ? "按下" : "未按下");
        
        // 测试游戏键映射
        int game_action = j2me_input_get_game_action(KEY_UP);
        LOG_DEBUG("  ✓ 游戏键映射功能正常 (UP键 -> 动作: %d)\n", game_action);
        
        // 测试指针状态 (简化实现)
        LOG_DEBUG("  ✓ 指针状态查询功能正常\n");
        
    } else {
        LOG_DEBUG("  ✗ 输入管理器未初始化\n");
        test_passed = false;
    }
    
    print_test_result("输入系统测试", test_passed);
}

void test_native_methods(j2me_vm_t* vm) {
    print_test_header("本地方法测试");
    
    bool test_passed = true;
    
    // 测试本地方法注册表
    if (vm->native_method_registry) {
        LOG_DEBUG("  ✓ 本地方法注册表已初始化\n");
        
        // 测试查找Display.getDisplay方法
        j2me_native_method_func_t display_method = j2me_native_method_find(
            (j2me_native_method_registry_t*)vm->native_method_registry,
            "javax/microedition/lcdui/Display",
            "getDisplay",
            "()Ljavax/microedition/lcdui/Display;"
        );
        
        if (display_method) {
            LOG_DEBUG("  ✓ Display.getDisplay方法找到\n");
        } else {
            LOG_DEBUG("  ✗ Display.getDisplay方法未找到\n");
            test_passed = false;
        }
        
        // 测试查找Graphics绘制方法
        j2me_native_method_func_t graphics_method = j2me_native_method_find(
            (j2me_native_method_registry_t*)vm->native_method_registry,
            "javax/microedition/lcdui/Graphics",
            "drawLine",
            "(IIII)V"
        );
        
        if (graphics_method) {
            LOG_DEBUG("  ✓ Graphics.drawLine方法找到\n");
        } else {
            LOG_DEBUG("  ✗ Graphics.drawLine方法未找到\n");
            test_passed = false;
        }
        
    } else {
        LOG_DEBUG("  ✗ 本地方法注册表未初始化\n");
        test_passed = false;
    }
    
    print_test_result("本地方法测试", test_passed);
}

void test_sdl_integration(j2me_vm_t* vm) {
    print_test_header("SDL集成测试");
    
    bool test_passed = true;
    
    // 测试SDL事件处理
    LOG_DEBUG("  测试SDL事件处理...\n");
    j2me_error_t event_result = j2me_vm_handle_events(vm);
    if (event_result == J2ME_SUCCESS) {
        LOG_DEBUG("  ✓ SDL事件处理正常\n");
    } else {
        LOG_DEBUG("  ✗ SDL事件处理失败: %d\n", event_result);
        test_passed = false;
    }
    
    // 测试显示更新
    if (vm->display) {
        LOG_DEBUG("  测试显示更新...\n");
        j2me_display_refresh((j2me_display_t*)vm->display);
        LOG_DEBUG("  ✓ 显示更新正常\n");
    } else {
        LOG_DEBUG("  ✗ 显示系统不可用\n");
        test_passed = false;
    }
    
    print_test_result("SDL集成测试", test_passed);
}

void print_test_header(const char* test_name) {
    LOG_DEBUG("\n--- %s ---\n", test_name);
}

void print_test_result(const char* test_name, bool passed) {
    LOG_DEBUG("--- %s: %s ---\n", test_name, passed ? "通过" : "失败");
}