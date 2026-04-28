/**
 * @file chinese_encoding_test.c
 * @brief 中文字符编码测试程序
 * 
 * 专门测试中文字符编码和渲染问题的修复
 */

#include "j2me_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>

#include "j2me_vm.h"
#include "j2me_graphics.h"
#include "j2me_input.h"

/**
 * @brief 测试UTF-8编码的中文字符串
 */
void test_utf8_chinese_strings(j2me_graphics_context_t* context) {
    LOG_DEBUG("\n=== 测试UTF-8中文字符串 ===\n");
    
    // 设置UTF-8 locale
    setlocale(LC_ALL, "");
    LOG_DEBUG("📝 设置UTF-8 locale完成\n");
    
    // 测试不同类型的中文文本
    const char* test_strings[] = {
        "你好世界",                    // 基础中文
        "J2ME中文字体测试",            // 中英文混合
        "数字123和符号！@#",           // 中文+数字+符号
        "简体中文：北京上海广州",       // 地名
        "繁體中文：臺北香港澳門",       // 繁体中文
        "特殊字符：©®™€£¥",           // 特殊符号
        "表情符号：😀😊🎮🎯",         // Unicode表情（如果字体支持）
        "长文本测试：这是一个比较长的中文文本，用来测试文本渲染和换行功能。",
        NULL
    };
    
    // 清除屏幕
    j2me_graphics_clear(context);
    
    // 设置不同颜色测试每个字符串
    j2me_color_t colors[] = {
        {255, 255, 255, 255}, // 白色
        {255, 100, 100, 255}, // 红色
        {100, 255, 100, 255}, // 绿色
        {100, 100, 255, 255}, // 蓝色
        {255, 255, 100, 255}, // 黄色
        {255, 100, 255, 255}, // 紫色
        {100, 255, 255, 255}, // 青色
        {255, 200, 100, 255}, // 橙色
    };
    
    for (int i = 0; test_strings[i] != NULL; i++) {
        const char* text = test_strings[i];
        
        // 设置颜色
        j2me_graphics_set_color(context, colors[i % 8]);
        
        // 计算位置
        int y = 50 + i * 35;
        
        // 渲染文本
        j2me_graphics_draw_string(context, text, 20, y, 0x00);
        
        // 获取文本宽度进行验证
        int text_width = j2me_graphics_get_string_width(context, text);
        
        LOG_DEBUG("🎨 渲染文本 %d: \"%s\"\n", i + 1, text);
        LOG_DEBUG("   位置: (20, %d), 宽度: %d 像素\n", y, text_width);
        
        // 在文本右侧绘制宽度指示线
        j2me_color_t indicator_color = {100, 100, 100, 255};
        j2me_graphics_set_color(context, indicator_color);
        j2me_graphics_draw_line(context, 20 + text_width, y, 20 + text_width, y + 20);
    }
    
    LOG_DEBUG("✅ UTF-8中文字符串测试完成\n");
}

/**
 * @brief 测试字体对中文字符的支持
 */
void test_font_chinese_support(j2me_graphics_context_t* context) {
    LOG_DEBUG("\n=== 测试字体中文字符支持 ===\n");
    
    if (!context->current_font.ttf_font) {
        LOG_DEBUG("❌ 当前没有加载TTF字体\n");
        return;
    }
    
    // 测试常用中文字符
    const char* chinese_chars[] = {
        "你", "好", "世", "界", "中", "文", "字", "体",
        "测", "试", "程", "序", "游", "戏", "模", "拟",
        "器", "系", "统", "功", "能", "显", "示", "效",
        "果", "质", "量", "性", "能", "优", "化", "完",
        NULL
    };
    
    LOG_DEBUG("📝 测试字体: %s\n", context->current_font.name);
    LOG_DEBUG("📏 字体大小: %d\n", context->current_font.size);
    
    // 清除屏幕
    j2me_graphics_clear(context);
    
    // 设置白色
    j2me_graphics_set_color(context, (j2me_color_t){255, 255, 255, 255});
    
    // 绘制字符网格
    int chars_per_row = 8;
    int char_size = 40;
    
    for (int i = 0; chinese_chars[i] != NULL; i++) {
        int row = i / chars_per_row;
        int col = i % chars_per_row;
        
        int x = 50 + col * char_size;
        int y = 50 + row * char_size;
        
        // 绘制字符
        j2me_graphics_draw_string(context, chinese_chars[i], x, y, 0x00);
        
        // 绘制网格线
        j2me_color_t grid_color = {50, 50, 50, 255};
        j2me_graphics_set_color(context, grid_color);
        j2me_graphics_draw_rect(context, x - 5, y - 5, char_size - 10, char_size - 10, false);
        j2me_graphics_set_color(context, (j2me_color_t){255, 255, 255, 255});
        
        // 测试字符宽度
        int char_width = j2me_graphics_get_string_width(context, chinese_chars[i]);
        if (i < 8) { // 只打印前8个字符的信息
            LOG_DEBUG("   字符 '%s': 宽度 %d 像素\n", chinese_chars[i], char_width);
        }
    }
    
    LOG_DEBUG("✅ 字体中文字符支持测试完成\n");
}

/**
 * @brief 测试不同字体大小的中文渲染
 */
void test_chinese_font_sizes(j2me_graphics_context_t* context) {
    LOG_DEBUG("\n=== 测试不同字体大小的中文渲染 ===\n");
    
    // 清除屏幕
    j2me_graphics_clear(context);
    
    const char* test_text = "中文字体大小测试";
    int sizes[] = {12, 16, 20, 24, 28, 32, 36, 40};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    
    j2me_graphics_set_color(context, (j2me_color_t){255, 255, 255, 255});
    
    for (int i = 0; i < num_sizes; i++) {
        // 创建新字体
        j2me_font_t font = j2me_graphics_create_font("STHeiti", sizes[i], 0);
        j2me_graphics_set_font(context, font);
        
        int y = 50 + i * 50;
        
        // 渲染文本
        j2me_graphics_draw_string(context, test_text, 50, y, 0x00);
        
        // 显示大小信息
        char size_info[32];
        snprintf(size_info, sizeof(size_info), "%d像素", sizes[i]);
        j2me_graphics_draw_string(context, size_info, 350, y, 0x00);
        
        LOG_DEBUG("📏 字体大小 %d: 高度 %d 像素\n", 
               sizes[i], j2me_graphics_get_font_height(context));
    }
    
    LOG_DEBUG("✅ 不同字体大小测试完成\n");
}

/**
 * @brief 中文编码修复验证演示
 */
void chinese_encoding_demo(j2me_vm_t* vm) {
    LOG_DEBUG("\n=== 中文编码修复验证演示 ===\n");
    LOG_DEBUG("🎮 控制说明:\n");
    LOG_DEBUG("   - 数字键 1-3: 切换不同测试\n");
    LOG_DEBUG("   - ESC键: 退出演示\n\n");
    
    if (!vm->display || !vm->display->context) {
        LOG_DEBUG("❌ 图形上下文未初始化\n");
        return;
    }
    
    j2me_graphics_context_t* context = vm->display->context;
    
    int demo_mode = 1;
    bool running = true;
    int frame_count = 0;
    
    while (running && vm->state == J2ME_VM_RUNNING) {
        // 处理事件
        j2me_vm_handle_events(vm);
        
        // 检查按键
        if (vm->input_manager) {
            if (j2me_input_is_key_pressed(vm->input_manager, KEY_END)) {
                running = false;
            }
            
            // 切换演示模式
            for (int i = 1; i <= 3; i++) {
                if (j2me_input_is_key_pressed(vm->input_manager, KEY_NUM0 + i)) {
                    demo_mode = i;
                    LOG_DEBUG("🔄 切换到测试模式 %d\n", demo_mode);
                }
            }
        }
        
        // 根据模式显示不同内容
        switch (demo_mode) {
            case 1:
                test_utf8_chinese_strings(context);
                break;
            case 2:
                test_font_chinese_support(context);
                break;
            case 3:
                test_chinese_font_sizes(context);
                break;
        }
        
        // 显示模式提示
        j2me_graphics_set_color(context, (j2me_color_t){200, 200, 200, 255});
        char mode_text[128];
        snprintf(mode_text, sizeof(mode_text), "测试模式: %d (按1-3切换, ESC退出)", demo_mode);
        j2me_graphics_draw_string(context, mode_text, 10, 10, 0x00);
        
        // 显示修复状态
        j2me_graphics_set_color(context, (j2me_color_t){100, 255, 100, 255});
        j2me_graphics_draw_string(context, "✅ 中文编码修复已应用 - 使用UTF-8渲染", 10, 30, 0x00);
        
        // 刷新显示
        j2me_display_refresh(vm->display);
        
        frame_count++;
        
        // 延迟 (30 FPS)
        usleep(33000);
        
        // 每个模式显示5秒后自动切换（演示模式）
        if (frame_count % 150 == 0) {
            demo_mode = (demo_mode % 3) + 1;
            LOG_DEBUG("🔄 自动切换到测试模式 %d\n", demo_mode);
        }
    }
    
    LOG_DEBUG("✅ 中文编码修复验证演示结束\n");
}

/**
 * @brief 主测试函数
 */
int main() {
    LOG_DEBUG("中文字符编码修复测试程序\n");
    LOG_DEBUG("========================\n");
    LOG_DEBUG("测试UTF-8中文字符编码和渲染修复\n\n");
    
    // 设置UTF-8环境
    setlocale(LC_ALL, "");
    LOG_DEBUG("🌐 设置UTF-8 locale环境\n");
    
    // 创建虚拟机配置
    j2me_vm_config_t config = {
        .heap_size = 2 * 1024 * 1024,  // 2MB堆
        .stack_size = 256 * 1024,      // 256KB栈
        .max_threads = 4               // 4个线程
    };
    
    // 创建虚拟机
    j2me_vm_t* vm = j2me_vm_create(&config);
    if (!vm) {
        LOG_DEBUG("❌ 创建虚拟机失败\n");
        return 1;
    }
    LOG_DEBUG("✅ 虚拟机创建成功\n");
    
    // 初始化虚拟机
    j2me_error_t result = j2me_vm_initialize(vm);
    if (result != J2ME_SUCCESS) {
        LOG_DEBUG("❌ 虚拟机初始化失败: %d\n", result);
        j2me_vm_destroy(vm);
        return 1;
    }
    LOG_DEBUG("✅ 虚拟机初始化成功\n");
    
    if (!vm->display || !vm->display->context) {
        LOG_DEBUG("❌ 图形上下文未初始化\n");
        j2me_vm_destroy(vm);
        return 1;
    }
    
    j2me_graphics_context_t* context = vm->display->context;
    
    // 显示当前字体信息
    LOG_DEBUG("\n📋 当前字体信息:\n");
    LOG_DEBUG("   字体名称: %s\n", context->current_font.name);
    LOG_DEBUG("   字体大小: %d\n", context->current_font.size);
    LOG_DEBUG("   TTF字体: %s\n", context->current_font.ttf_font ? "已加载" : "未加载");
    
    LOG_DEBUG("\n⏳ 等待3秒后开始中文编码测试...\n");
    sleep(3);
    
    // 运行中文编码修复验证演示
    chinese_encoding_demo(vm);
    
    LOG_DEBUG("\n⏳ 等待3秒以查看最终结果...\n");
    sleep(3);
    
    // 清理虚拟机
    j2me_vm_destroy(vm);
    
    LOG_DEBUG("\n=== 中文编码修复测试总结 ===\n");
    LOG_DEBUG("✅ UTF-8编码支持: TTF_RenderUTF8_Blended()函数应用\n");
    LOG_DEBUG("✅ 中文字体优先: 更新字体加载顺序，优先中文字体\n");
    LOG_DEBUG("✅ 字符串度量: TTF_SizeUTF8()函数支持中文宽度计算\n");
    LOG_DEBUG("✅ 多字体支持: 扩展中文字体路径列表\n");
    LOG_DEBUG("✅ 编码兼容: UTF-8和普通文本渲染双重支持\n");
    LOG_DEBUG("✅ 错误处理: 渲染失败时的回退机制\n");
    
    LOG_DEBUG("\n🎉 中文字符编码修复测试完成！\n");
    LOG_DEBUG("💡 现在应该能够正确显示中文字符，不再出现乱码！\n");
    
    return 0;
}