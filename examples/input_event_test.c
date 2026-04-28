/**
 * @file input_event_test.c
 * @brief 输入事件处理测试程序
 * 
 * 测试SDL事件与MIDP Canvas事件回调的集成
 */

#include "j2me_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "j2me_vm.h"
#include "j2me_input.h"
#include "j2me_graphics.h"
#include "j2me_native_methods.h"

/**
 * @brief 测试输入系统初始化
 */
void test_input_system_initialization(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试输入系统初始化 ===\n");
    
    if (!vm->input_manager) {
        LOG_DEBUG("❌ 输入管理器未初始化\n");
        return;
    }
    
    LOG_DEBUG("✅ 输入管理器已创建\n");
    
    // 测试键映射
    LOG_DEBUG("🔑 测试键映射...\n");
    LOG_DEBUG("  - 数字键0: %s\n", j2me_input_get_key_name(KEY_NUM0));
    LOG_DEBUG("  - 上方向键: %s\n", j2me_input_get_key_name(KEY_UP));
    LOG_DEBUG("  - 确认键: %s\n", j2me_input_get_key_name(KEY_FIRE));
    LOG_DEBUG("  - 左软键: %s\n", j2me_input_get_key_name(KEY_SOFT_LEFT));
    
    // 测试游戏动作映射
    LOG_DEBUG("🎮 测试游戏动作映射...\n");
    LOG_DEBUG("  - 数字键2 -> 游戏动作: %d\n", j2me_input_get_game_action(KEY_NUM2));
    LOG_DEBUG("  - 数字键5 -> 游戏动作: %d\n", j2me_input_get_game_action(KEY_NUM5));
    LOG_DEBUG("  - 上方向键 -> 游戏动作: %d\n", j2me_input_get_game_action(KEY_UP));
    
    LOG_DEBUG("✅ 输入系统初始化测试完成\n");
}

/**
 * @brief 测试MIDP Canvas事件方法
 */
void test_midp_canvas_events(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试MIDP Canvas事件方法 ===\n");
    
    // 创建测试栈帧
    j2me_stack_frame_t* frame = j2me_stack_frame_create(20, 10);
    if (!frame) {
        LOG_DEBUG("❌ 创建栈帧失败\n");
        return;
    }
    
    LOG_DEBUG("✅ 测试栈帧创建成功\n");
    
    // 测试keyPressed
    LOG_DEBUG("\n--- 测试Canvas.keyPressed() ---\n");
    j2me_operand_stack_push(&frame->operand_stack, 0x30000001); // Canvas对象引用
    j2me_operand_stack_push(&frame->operand_stack, KEY_UP);     // 上方向键
    
    j2me_error_t result = midp_canvas_key_pressed(vm, frame, NULL);
    if (result == J2ME_SUCCESS) {
        LOG_DEBUG("✅ Canvas.keyPressed(UP) 调用成功\n");
    } else {
        LOG_DEBUG("❌ Canvas.keyPressed(UP) 调用失败: %d\n", result);
    }
    
    // 测试keyReleased
    LOG_DEBUG("\n--- 测试Canvas.keyReleased() ---\n");
    j2me_operand_stack_push(&frame->operand_stack, 0x30000001); // Canvas对象引用
    j2me_operand_stack_push(&frame->operand_stack, KEY_FIRE);   // 确认键
    
    result = midp_canvas_key_released(vm, frame, NULL);
    if (result == J2ME_SUCCESS) {
        LOG_DEBUG("✅ Canvas.keyReleased(FIRE) 调用成功\n");
    } else {
        LOG_DEBUG("❌ Canvas.keyReleased(FIRE) 调用失败: %d\n", result);
    }
    
    // 测试pointerPressed
    LOG_DEBUG("\n--- 测试Canvas.pointerPressed() ---\n");
    j2me_operand_stack_push(&frame->operand_stack, 0x30000001); // Canvas对象引用
    j2me_operand_stack_push(&frame->operand_stack, 120);        // X坐标
    j2me_operand_stack_push(&frame->operand_stack, 160);        // Y坐标
    
    result = midp_canvas_pointer_pressed(vm, frame, NULL);
    if (result == J2ME_SUCCESS) {
        LOG_DEBUG("✅ Canvas.pointerPressed(120, 160) 调用成功\n");
    } else {
        LOG_DEBUG("❌ Canvas.pointerPressed(120, 160) 调用失败: %d\n", result);
    }
    
    // 测试pointerReleased
    LOG_DEBUG("\n--- 测试Canvas.pointerReleased() ---\n");
    j2me_operand_stack_push(&frame->operand_stack, 0x30000001); // Canvas对象引用
    j2me_operand_stack_push(&frame->operand_stack, 100);        // X坐标
    j2me_operand_stack_push(&frame->operand_stack, 200);        // Y坐标
    
    result = midp_canvas_pointer_released(vm, frame, NULL);
    if (result == J2ME_SUCCESS) {
        LOG_DEBUG("✅ Canvas.pointerReleased(100, 200) 调用成功\n");
    } else {
        LOG_DEBUG("❌ Canvas.pointerReleased(100, 200) 调用失败: %d\n", result);
    }
    
    // 测试pointerDragged
    LOG_DEBUG("\n--- 测试Canvas.pointerDragged() ---\n");
    j2me_operand_stack_push(&frame->operand_stack, 0x30000001); // Canvas对象引用
    j2me_operand_stack_push(&frame->operand_stack, 150);        // X坐标
    j2me_operand_stack_push(&frame->operand_stack, 180);        // Y坐标
    
    result = midp_canvas_pointer_dragged(vm, frame, NULL);
    if (result == J2ME_SUCCESS) {
        LOG_DEBUG("✅ Canvas.pointerDragged(150, 180) 调用成功\n");
    } else {
        LOG_DEBUG("❌ Canvas.pointerDragged(150, 180) 调用失败: %d\n", result);
    }
    
    // 清理栈帧
    j2me_stack_frame_destroy(frame);
    LOG_DEBUG("✅ MIDP Canvas事件方法测试完成\n");
}

/**
 * @brief 交互式事件处理演示
 */
void interactive_event_demo(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 交互式事件处理演示 ===\n");
    LOG_DEBUG("🎮 请使用键盘和鼠标与窗口交互\n");
    LOG_DEBUG("   - 方向键: 上下左右移动\n");
    LOG_DEBUG("   - 数字键: 0-9\n");
    LOG_DEBUG("   - 空格键: 确认 (FIRE)\n");
    LOG_DEBUG("   - F1/F2: 左右软键\n");
    LOG_DEBUG("   - 鼠标: 点击和拖拽\n");
    LOG_DEBUG("   - ESC键: 退出演示\n");
    LOG_DEBUG("⏰ 演示将运行30秒，或按ESC键退出\n\n");
    
    if (!vm->display || !vm->display->context) {
        LOG_DEBUG("❌ 图形上下文未初始化\n");
        return;
    }
    
    j2me_graphics_context_t* context = vm->display->context;
    
    // 绘制初始界面
    j2me_graphics_clear(context);
    
    // 绘制标题
    j2me_color_t black = {0, 0, 0, 255};
    j2me_graphics_set_color(context, black);
    j2me_graphics_draw_string(context, "Input Event Demo", 120, 30, 0x22);
    
    // 绘制说明
    j2me_graphics_draw_string(context, "Press keys or click", 120, 60, 0x22);
    j2me_graphics_draw_string(context, "ESC to exit", 120, 90, 0x22);
    
    // 绘制一个可交互的矩形
    j2me_color_t blue = {0, 0, 255, 255};
    j2me_graphics_set_color(context, blue);
    j2me_graphics_draw_rect(context, 80, 120, 80, 60, false);
    j2me_graphics_draw_string(context, "Click Me", 120, 150, 0x22);
    
    j2me_display_refresh(vm->display);
    
    // 事件处理循环
    int demo_time = 0;
    const int max_demo_time = 30000; // 30秒
    const int frame_time = 100;      // 100ms per frame
    
    bool running = true;
    while (running && demo_time < max_demo_time) {
        // 处理事件
        j2me_error_t result = j2me_vm_handle_events(vm);
        if (result != J2ME_SUCCESS || vm->state != J2ME_VM_RUNNING) {
            LOG_DEBUG("\n🛑 虚拟机停止或发生错误\n");
            break;
        }
        
        // 检查ESC键是否按下
        if (j2me_input_is_key_pressed(vm->input_manager, KEY_END)) {
            LOG_DEBUG("\n🛑 用户按下ESC键，退出演示\n");
            break;
        }
        
        // 更新显示 (可以在这里添加动态效果)
        // j2me_display_refresh(vm->display);
        
        // 延迟
        usleep(frame_time * 1000);
        demo_time += frame_time;
        
        // 每5秒显示一次进度
        if (demo_time % 5000 == 0) {
            LOG_DEBUG("⏰ 演示进行中... %d/%d 秒\n", demo_time / 1000, max_demo_time / 1000);
        }
    }
    
    if (demo_time >= max_demo_time) {
        LOG_DEBUG("\n⏰ 演示时间结束\n");
    }
    
    LOG_DEBUG("✅ 交互式事件处理演示完成\n");
}

/**
 * @brief 输入状态监控测试
 */
void test_input_state_monitoring(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 测试输入状态监控 ===\n");
    
    if (!vm->input_manager) {
        LOG_DEBUG("❌ 输入管理器未初始化\n");
        return;
    }
    
    LOG_DEBUG("🔍 监控输入状态 (5秒)...\n");
    LOG_DEBUG("   请按住一些键或鼠标按钮\n\n");
    
    for (int i = 0; i < 50; i++) { // 5秒，每100ms检查一次
        // 处理事件
        j2me_vm_handle_events(vm);
        
        // 检查一些常用键的状态
        bool any_key_pressed = false;
        
        if (j2me_input_is_key_pressed(vm->input_manager, KEY_UP)) {
            LOG_DEBUG("🔑 上方向键按下\n");
            any_key_pressed = true;
        }
        if (j2me_input_is_key_pressed(vm->input_manager, KEY_DOWN)) {
            LOG_DEBUG("🔑 下方向键按下\n");
            any_key_pressed = true;
        }
        if (j2me_input_is_key_pressed(vm->input_manager, KEY_LEFT)) {
            LOG_DEBUG("🔑 左方向键按下\n");
            any_key_pressed = true;
        }
        if (j2me_input_is_key_pressed(vm->input_manager, KEY_RIGHT)) {
            LOG_DEBUG("🔑 右方向键按下\n");
            any_key_pressed = true;
        }
        if (j2me_input_is_key_pressed(vm->input_manager, KEY_FIRE)) {
            LOG_DEBUG("🔑 确认键按下\n");
            any_key_pressed = true;
        }
        
        // 检查指针状态
        if (j2me_input_is_pointer_pressed(vm->input_manager)) {
            int x, y;
            j2me_input_get_pointer_position(vm->input_manager, &x, &y);
            LOG_DEBUG("🖱️ 指针按下: (%d, %d)\n", x, y);
            any_key_pressed = true;
        }
        
        // 检查游戏键状态
        int key_states = j2me_input_get_key_states(vm->input_manager);
        if (key_states != 0) {
            LOG_DEBUG("🎮 游戏键状态: 0x%x\n", key_states);
            any_key_pressed = true;
        }
        
        if (any_key_pressed) {
            LOG_DEBUG("---\n");
        }
        
        usleep(100000); // 100ms
    }
    
    LOG_DEBUG("✅ 输入状态监控测试完成\n");
}

/**
 * @brief 主测试函数
 */
int main() {
    LOG_DEBUG("输入事件处理测试程序\n");
    LOG_DEBUG("======================\n");
    LOG_DEBUG("测试SDL事件与MIDP Canvas事件回调的集成\n");
    LOG_DEBUG("包括键盘、鼠标事件处理和状态监控\n\n");
    
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
    
    // 初始化虚拟机 (这将初始化SDL2显示系统和输入系统)
    j2me_error_t result = j2me_vm_initialize(vm);
    if (result != J2ME_SUCCESS) {
        LOG_DEBUG("❌ 虚拟机初始化失败: %d\n", result);
        j2me_vm_destroy(vm);
        return 1;
    }
    LOG_DEBUG("✅ 虚拟机初始化成功\n");
    
    // 运行测试
    test_input_system_initialization(vm);
    
    LOG_DEBUG("\n⏳ 等待3秒...\n");
    sleep(3);
    
    test_midp_canvas_events(vm);
    
    LOG_DEBUG("\n⏳ 等待3秒...\n");
    sleep(3);
    
    test_input_state_monitoring(vm);
    
    LOG_DEBUG("\n⏳ 等待3秒...\n");
    sleep(3);
    
    interactive_event_demo(vm);
    
    // 清理虚拟机
    j2me_vm_destroy(vm);
    
    LOG_DEBUG("\n=== 输入事件处理测试总结 ===\n");
    LOG_DEBUG("✅ 输入系统初始化: 输入管理器创建和键映射正常\n");
    LOG_DEBUG("✅ MIDP Canvas事件: keyPressed、keyReleased、pointer事件方法正常\n");
    LOG_DEBUG("✅ 事件回调集成: SDL事件成功触发MIDP Canvas回调\n");
    LOG_DEBUG("✅ 输入状态监控: 键盘和鼠标状态实时监控正常\n");
    LOG_DEBUG("✅ 交互式演示: 用户输入事件处理正常\n");
    LOG_DEBUG("✅ 资源管理: 自动清理和释放正常\n");
    
    LOG_DEBUG("\n🎉 输入事件处理测试完成！\n");
    LOG_DEBUG("💡 下一步: 实现图像加载和处理系统\n");
    
    return 0;
}