#include "../include/j2me_vm.h"
#include "../include/j2me_object.h"
#include "../include/j2me_midp_graphics.h"
#include "../include/j2me_input.h"
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

/**
 * @file midp_test.c
 * @brief MIDP API测试程序
 * 
 * 测试第三阶段新增的功能：对象系统、MIDP图形API、输入系统
 */

// 全局变量
static bool running = true;
static j2me_vm_t* vm = NULL;
static j2me_display_t* display = NULL;
static j2me_midp_graphics_t* midp_graphics = NULL;
static j2me_input_manager_t* input_manager = NULL;

// 测试对象
static j2me_object_t* test_object = NULL;
static j2me_array_t* test_array = NULL;
static j2me_string_t* test_string = NULL;
static j2me_midp_image_t* test_image = NULL;
static j2me_midp_font_t* test_font = NULL;

/**
 * @brief 键盘事件回调
 */
void on_key_event(j2me_key_event_t* event, void* user_data) {
    printf("[MIDP测试] 键盘事件: 类型=%d, 键码=%d (%s), 字符='%c', 游戏键=%s\n",
           event->type, event->key_code, j2me_input_get_key_name(event->key_code),
           event->key_char ? event->key_char : '?',
           event->is_game_key ? "是" : "否");
    
    // ESC键退出
    if (event->key_code == KEY_END && event->type == INPUT_EVENT_KEY_PRESSED) {
        running = false;
    }
}

/**
 * @brief 指针事件回调
 */
void on_pointer_event(j2me_pointer_event_t* event, void* user_data) {
    printf("[MIDP测试] 指针事件: 类型=%d, 位置=(%d,%d)\n",
           event->type, event->x, event->y);
}

/**
 * @brief 测试对象系统
 */
void test_object_system(void) {
    printf("\n=== 测试对象系统 ===\n");
    
    // 加载一个测试类
    j2me_class_loader_t* loader = (j2me_class_loader_t*)vm->class_loader;
    j2me_class_t* hello_class = j2me_class_loader_load_class(loader, "Hello");
    
    if (hello_class) {
        // 创建对象
        test_object = j2me_object_create(vm, hello_class);
        if (test_object) {
            printf("✓ 对象创建成功\n");
            
            // 测试类型检查
            bool is_instance = j2me_object_instanceof(test_object, hello_class);
            printf("✓ instanceof检查: %s\n", is_instance ? "通过" : "失败");
            
            // 测试类型转换
            bool can_cast = j2me_object_checkcast(test_object, hello_class);
            printf("✓ checkcast检查: %s\n", can_cast ? "通过" : "失败");
        } else {
            printf("⚠ 对象创建失败\n");
        }
    }
    
    // 创建数组
    test_array = j2me_array_create(vm, ARRAY_TYPE_INT, 10);
    if (test_array) {
        printf("✓ 数组创建成功，长度: %d\n", j2me_array_get_length(test_array));
        
        // 测试数组操作
        j2me_array_set_int(test_array, 0, 42);
        j2me_array_set_int(test_array, 1, 100);
        
        int value0 = j2me_array_get_int(test_array, 0);
        int value1 = j2me_array_get_int(test_array, 1);
        
        printf("✓ 数组元素: [0]=%d, [1]=%d\n", value0, value1);
        
        if (value0 == 42 && value1 == 100) {
            printf("✓ 数组操作正确\n");
        } else {
            printf("⚠ 数组操作错误\n");
        }
    } else {
        printf("⚠ 数组创建失败\n");
    }
    
    // 创建字符串
    test_string = j2me_string_create_from_cstr(vm, "Hello MIDP!");
    if (test_string) {
        printf("✓ 字符串创建成功，长度: %d\n", j2me_string_get_length(test_string));
        
        // 创建另一个字符串进行比较
        j2me_string_t* str2 = j2me_string_create_from_cstr(vm, "Hello MIDP!");
        if (str2) {
            int cmp_result = j2me_string_compare(test_string, str2);
            printf("✓ 字符串比较结果: %d (0表示相等)\n", cmp_result);
        }
    } else {
        printf("⚠ 字符串创建失败\n");
    }
}

/**
 * @brief 测试MIDP图形API
 */
void test_midp_graphics(void) {
    printf("\n=== 测试MIDP图形API ===\n");
    
    if (!midp_graphics) {
        printf("⚠ MIDP图形上下文未初始化\n");
        return;
    }
    
    // 清除画布
    j2me_graphics_clear(display->context);
    
    // 测试颜色设置
    j2me_midp_graphics_set_color_rgb(midp_graphics, 255, 0, 0); // 红色
    printf("✓ 设置颜色为红色\n");
    
    // 测试基本绘制
    j2me_midp_graphics_draw_rect(midp_graphics, 10, 10, 50, 30);
    printf("✓ 绘制矩形\n");
    
    j2me_midp_graphics_set_color(midp_graphics, 0x00FF00); // 绿色
    j2me_midp_graphics_fill_rect(midp_graphics, 70, 10, 50, 30);
    printf("✓ 填充矩形\n");
    
    // 测试线条绘制
    j2me_midp_graphics_set_color_rgb(midp_graphics, 0, 0, 255); // 蓝色
    j2me_midp_graphics_draw_line(midp_graphics, 0, 50, 240, 50);
    printf("✓ 绘制直线\n");
    
    // 测试圆角矩形
    j2me_midp_graphics_set_color_rgb(midp_graphics, 255, 255, 0); // 黄色
    j2me_midp_graphics_draw_round_rect(midp_graphics, 10, 60, 80, 40, 10, 10);
    printf("✓ 绘制圆角矩形\n");
    
    // 测试弧形
    j2me_midp_graphics_set_color_rgb(midp_graphics, 255, 0, 255); // 紫色
    j2me_midp_graphics_draw_arc(midp_graphics, 100, 60, 60, 60, 0, 90);
    printf("✓ 绘制弧形\n");
    
    // 测试坐标变换
    j2me_midp_graphics_translate(midp_graphics, 20, 20);
    j2me_midp_graphics_set_color_rgb(midp_graphics, 0, 255, 255); // 青色
    j2me_midp_graphics_fill_rect(midp_graphics, 0, 100, 30, 20);
    printf("✓ 坐标变换和绘制\n");
    
    // 重置变换
    j2me_midp_graphics_translate(midp_graphics, -20, -20);
    
    // 测试文本绘制
    j2me_midp_graphics_set_color_rgb(midp_graphics, 0, 0, 0); // 黑色
    j2me_midp_graphics_draw_string(midp_graphics, "Hello MIDP!", 10, 140, ANCHOR_LEFT | ANCHOR_TOP);
    printf("✓ 绘制文本\n");
    
    // 测试不同锚点的文本
    j2me_midp_graphics_draw_string(midp_graphics, "Center", 120, 160, ANCHOR_HCENTER | ANCHOR_VCENTER);
    j2me_midp_graphics_draw_string(midp_graphics, "Right", 230, 180, ANCHOR_RIGHT | ANCHOR_TOP);
    printf("✓ 不同锚点文本绘制\n");
    
    // 测试字符绘制
    j2me_midp_graphics_draw_char(midp_graphics, 'A', 10, 200, ANCHOR_LEFT | ANCHOR_TOP);
    printf("✓ 绘制字符\n");
    
    // 测试子字符串绘制
    j2me_midp_graphics_draw_substring(midp_graphics, "Substring Test", 3, 6, 50, 200, ANCHOR_LEFT | ANCHOR_TOP);
    printf("✓ 绘制子字符串\n");
    
    // 测试裁剪
    j2me_midp_graphics_set_clip(midp_graphics, 10, 220, 100, 50);
    j2me_midp_graphics_set_color_rgb(midp_graphics, 128, 128, 128); // 灰色
    j2me_midp_graphics_fill_rect(midp_graphics, 0, 210, 200, 70); // 部分会被裁剪
    printf("✓ 裁剪区域测试\n");
    
    // 刷新显示
    j2me_display_refresh(display);
}

/**
 * @brief 测试字体系统
 */
void test_font_system(void) {
    printf("\n=== 测试字体系统 ===\n");
    
    // 创建不同的字体
    j2me_midp_font_t* small_font = j2me_midp_font_create(vm, FONT_FACE_SYSTEM, FONT_STYLE_PLAIN, FONT_SIZE_SMALL);
    j2me_midp_font_t* large_font = j2me_midp_font_create(vm, FONT_FACE_SYSTEM, FONT_STYLE_BOLD, FONT_SIZE_LARGE);
    
    if (small_font && large_font) {
        printf("✓ 字体创建成功\n");
        printf("✓ 小字体高度: %d\n", j2me_midp_font_get_height(small_font));
        printf("✓ 大字体高度: %d\n", j2me_midp_font_get_height(large_font));
        
        // 测试字符串宽度计算
        const char* test_text = "Test";
        int small_width = j2me_midp_font_string_width(small_font, test_text);
        int large_width = j2me_midp_font_string_width(large_font, test_text);
        
        printf("✓ 小字体文本宽度: %d\n", small_width);
        printf("✓ 大字体文本宽度: %d\n", large_width);
        
        // 测试字符宽度
        int char_width = j2me_midp_font_char_width(small_font, 'A');
        printf("✓ 字符'A'宽度: %d\n", char_width);
        
        test_font = small_font; // 保存用于后续测试
    } else {
        printf("⚠ 字体创建失败\n");
    }
}

/**
 * @brief 测试图像系统
 */
void test_image_system(void) {
    printf("\n=== 测试图像系统 ===\n");
    
    // 创建可变图像
    test_image = j2me_midp_image_create(vm, 64, 48);
    if (test_image) {
        printf("✓ 图像创建成功: %dx%d\n", 
               j2me_midp_image_get_width(test_image),
               j2me_midp_image_get_height(test_image));
        
        printf("✓ 图像可变性: %s\n", 
               j2me_midp_image_is_mutable(test_image) ? "可变" : "不可变");
        
        // 尝试获取图像的图形上下文
        j2me_midp_graphics_t* img_graphics = j2me_midp_image_get_graphics(test_image);
        if (img_graphics) {
            printf("✓ 获取图像图形上下文成功\n");
        } else {
            printf("⚠ 获取图像图形上下文失败 (功能未完全实现)\n");
        }
    } else {
        printf("⚠ 图像创建失败\n");
    }
    
    // 尝试从文件创建图像
    j2me_midp_image_t* file_image = j2me_midp_image_create_from_file(vm, "test.png");
    if (file_image) {
        printf("✓ 从文件创建图像成功 (简化实现)\n");
    }
}

/**
 * @brief 测试输入系统
 */
void test_input_system(void) {
    printf("\n=== 测试输入系统 ===\n");
    
    if (!input_manager) {
        printf("⚠ 输入管理器未初始化\n");
        return;
    }
    
    // 测试键名称获取
    printf("✓ 键名称测试:\n");
    printf("  KEY_UP: %s\n", j2me_input_get_key_name(KEY_UP));
    printf("  KEY_FIRE: %s\n", j2me_input_get_key_name(KEY_FIRE));
    printf("  KEY_NUM5: %s\n", j2me_input_get_key_name(KEY_NUM5));
    
    // 测试游戏动作映射
    int up_action = j2me_input_get_game_action(KEY_UP);
    int fire_key = j2me_input_get_key_code(KEY_FIRE);
    printf("✓ 游戏动作映射: KEY_UP -> %d, KEY_FIRE <- %d\n", up_action, fire_key);
    
    // 测试当前输入状态
    int key_states = j2me_input_get_key_states(input_manager);
    printf("✓ 当前游戏键状态: 0x%x\n", key_states);
    
    // 测试指针状态
    int pointer_x, pointer_y;
    j2me_input_get_pointer_position(input_manager, &pointer_x, &pointer_y);
    bool pointer_pressed = j2me_input_is_pointer_pressed(input_manager);
    printf("✓ 指针状态: 位置=(%d,%d), 按下=%s\n", 
           pointer_x, pointer_y, pointer_pressed ? "是" : "否");
}

/**
 * @brief 绘制交互式演示
 */
void draw_interactive_demo(void) {
    if (!midp_graphics) {
        return;
    }
    
    // 清除画布
    j2me_graphics_clear(display->context);
    
    // 绘制标题
    j2me_midp_graphics_set_color_rgb(midp_graphics, 0, 0, 0);
    j2me_midp_graphics_draw_string(midp_graphics, "MIDP API Demo", 120, 10, ANCHOR_HCENTER | ANCHOR_TOP);
    
    // 绘制说明
    j2me_midp_graphics_draw_string(midp_graphics, "Use arrow keys to move", 10, 30, ANCHOR_LEFT | ANCHOR_TOP);
    j2me_midp_graphics_draw_string(midp_graphics, "Press SPACE for action", 10, 45, ANCHOR_LEFT | ANCHOR_TOP);
    j2me_midp_graphics_draw_string(midp_graphics, "ESC to quit", 10, 60, ANCHOR_LEFT | ANCHOR_TOP);
    
    // 绘制游戏键状态指示器
    int y_pos = 80;
    j2me_midp_graphics_draw_string(midp_graphics, "Game Keys:", 10, y_pos, ANCHOR_LEFT | ANCHOR_TOP);
    
    // 检查各个游戏键状态并绘制指示器
    struct {
        int key;
        const char* name;
        int x, y;
    } keys[] = {
        {KEY_UP, "UP", 50, 100},
        {KEY_DOWN, "DOWN", 50, 130},
        {KEY_LEFT, "LEFT", 20, 115},
        {KEY_RIGHT, "RIGHT", 80, 115},
        {KEY_FIRE, "FIRE", 50, 115}
    };
    
    for (int i = 0; i < 5; i++) {
        bool pressed = j2me_input_is_key_pressed(input_manager, keys[i].key);
        
        // 设置颜色 (按下时为红色，否则为灰色)
        if (pressed) {
            j2me_midp_graphics_set_color_rgb(midp_graphics, 255, 0, 0);
        } else {
            j2me_midp_graphics_set_color_rgb(midp_graphics, 128, 128, 128);
        }
        
        // 绘制按键指示器
        j2me_midp_graphics_fill_rect(midp_graphics, keys[i].x, keys[i].y, 20, 15);
        
        // 绘制按键名称
        j2me_midp_graphics_set_color_rgb(midp_graphics, 0, 0, 0);
        j2me_midp_graphics_draw_string(midp_graphics, keys[i].name, 
                                      keys[i].x + 10, keys[i].y + 20, 
                                      ANCHOR_HCENTER | ANCHOR_TOP);
    }
    
    // 绘制指针位置
    int pointer_x, pointer_y;
    j2me_input_get_pointer_position(input_manager, &pointer_x, &pointer_y);
    bool pointer_pressed = j2me_input_is_pointer_pressed(input_manager);
    
    j2me_midp_graphics_set_color_rgb(midp_graphics, 0, 0, 255);
    j2me_midp_graphics_draw_string(midp_graphics, "Mouse:", 10, 160, ANCHOR_LEFT | ANCHOR_TOP);
    
    char pos_text[64];
    snprintf(pos_text, sizeof(pos_text), "(%d,%d) %s", 
             pointer_x, pointer_y, pointer_pressed ? "PRESSED" : "");
    j2me_midp_graphics_draw_string(midp_graphics, pos_text, 60, 160, ANCHOR_LEFT | ANCHOR_TOP);
    
    // 在指针位置绘制十字标记
    if (pointer_x > 0 && pointer_y > 0) {
        j2me_midp_graphics_set_color_rgb(midp_graphics, pointer_pressed ? 255 : 0, 0, 0);
        j2me_midp_graphics_draw_line(midp_graphics, pointer_x - 5, pointer_y, pointer_x + 5, pointer_y);
        j2me_midp_graphics_draw_line(midp_graphics, pointer_x, pointer_y - 5, pointer_x, pointer_y + 5);
    }
    
    // 绘制一些装饰图形
    j2me_midp_graphics_set_color_rgb(midp_graphics, 0, 255, 0);
    j2me_midp_graphics_draw_round_rect(midp_graphics, 150, 100, 80, 60, 10, 10);
    
    j2me_midp_graphics_set_color_rgb(midp_graphics, 255, 255, 0);
    j2me_midp_graphics_fill_arc(midp_graphics, 160, 110, 60, 40, 0, 180);
    
    // 刷新显示
    j2me_display_refresh(display);
}

/**
 * @brief 主循环
 */
void main_loop(void) {
    SDL_Event event;
    uint32_t last_time = SDL_GetTicks();
    const uint32_t frame_time = 1000 / 30; // 30 FPS
    
    while (running) {
        uint32_t current_time = SDL_GetTicks();
        
        // 处理SDL事件
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                break;
            }
            
            // 让输入管理器处理事件
            j2me_input_handle_sdl_event(input_manager, &event);
        }
        
        // 更新输入状态
        j2me_input_update(input_manager);
        
        // 渲染 (限制帧率)
        if (current_time - last_time >= frame_time) {
            draw_interactive_demo();
            last_time = current_time;
        }
        
        // 避免CPU占用过高
        SDL_Delay(1);
    }
}

int main(void) {
    printf("J2ME MIDP API测试程序 (第三阶段)\n");
    printf("================================\n");
    
    // 初始化虚拟机
    j2me_vm_config_t vm_config = j2me_vm_get_default_config();
    vm = j2me_vm_create(&vm_config);
    if (!vm) {
        printf("错误: 虚拟机创建失败\n");
        return 1;
    }
    
    j2me_vm_initialize(vm);
    
    // 初始化显示系统
    display = j2me_display_initialize(240, 320, "J2ME MIDP Test");
    if (!display) {
        printf("错误: 显示系统初始化失败\n");
        j2me_vm_destroy(vm);
        return 1;
    }
    
    // 创建图形上下文
    j2me_graphics_context_t* graphics = j2me_graphics_create_context(display, 240, 320);
    if (!graphics) {
        printf("错误: 图形上下文创建失败\n");
        j2me_display_destroy(display);
        j2me_vm_destroy(vm);
        return 1;
    }
    
    // 创建MIDP图形上下文
    midp_graphics = j2me_midp_graphics_create(graphics);
    if (!midp_graphics) {
        printf("错误: MIDP图形上下文创建失败\n");
        j2me_display_destroy(display);
        j2me_vm_destroy(vm);
        return 1;
    }
    
    // 创建输入管理器
    input_manager = j2me_input_manager_create();
    if (!input_manager) {
        printf("错误: 输入管理器创建失败\n");
        j2me_midp_graphics_destroy(midp_graphics);
        j2me_display_destroy(display);
        j2me_vm_destroy(vm);
        return 1;
    }
    
    // 设置输入回调
    j2me_input_set_key_callback(input_manager, on_key_event, NULL);
    j2me_input_set_pointer_callback(input_manager, on_pointer_event, NULL);
    
    printf("所有子系统初始化完成\n");
    
    // 运行测试
    test_object_system();
    test_font_system();
    test_image_system();
    test_input_system();
    test_midp_graphics();
    
    printf("\n开始交互式演示...\n");
    printf("使用方向键移动，空格键动作，ESC退出\n");
    
    // 运行主循环
    main_loop();
    
    printf("\n=== MIDP测试完成 ===\n");
    
    // 清理资源
    j2me_input_manager_destroy(input_manager);
    j2me_midp_graphics_destroy(midp_graphics);
    j2me_display_destroy(display);
    j2me_vm_destroy(vm);
    
    return 0;
}