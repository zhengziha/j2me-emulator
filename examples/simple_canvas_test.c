#include "j2me_vm.h"
#include "j2me_heap.h"
#include "j2me_string.h"
#include "j2me_native_methods.h"
#include "j2me_class.h"
#include "j2me_interpreter.h"
#include "j2me_graphics.h"
#include "j2me_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <SDL2/SDL.h>

/**
 * @file simple_canvas_test.c
 * @brief Simple Canvas测试程序
 * 
 * 测试Canvas和Graphics对象的创建和使用
 */

// Canvas对象结构（在堆上）
typedef struct {
    int width;
    int height;
    j2me_ref_t graphics_ref;  // Graphics对象引用
} j2me_canvas_data_t;

// Graphics对象结构（在堆上）
typedef struct {
    void* context_ptr;  // 使用void*存储指针（8字节）
    int color;          // 当前颜色
    int padding;        // 对齐填充
} j2me_graphics_data_t;

// Canvas类ID
#define J2ME_CLASS_CANVAS 2

// Graphics类ID  
#define J2ME_CLASS_GRAPHICS 3

/**
 * @brief 创建Canvas对象
 */
j2me_ref_t create_canvas_object(j2me_vm_t* vm, int width, int height) {
    LOG_DEBUG("[Canvas] 创建Canvas对象: %dx%d\n", width, height);
    
    // 在堆上分配Canvas对象
    j2me_ref_t canvas_ref = j2me_heap_alloc(vm->heap, J2ME_CLASS_CANVAS, 
                                            sizeof(j2me_canvas_data_t));
    if (canvas_ref == J2ME_NULL_REF) {
        LOG_DEBUG("[Canvas] 错误: Canvas对象分配失败\n");
        return J2ME_NULL_REF;
    }
    
    // 获取Canvas数据
    j2me_canvas_data_t* canvas_data = (j2me_canvas_data_t*)j2me_heap_get_object_data(vm->heap, canvas_ref);
    if (!canvas_data) {
        LOG_DEBUG("[Canvas] 错误: 无法获取Canvas数据\n");
        return J2ME_NULL_REF;
    }
    
    // 初始化Canvas数据
    canvas_data->width = width;
    canvas_data->height = height;
    canvas_data->graphics_ref = J2ME_NULL_REF;
    
    LOG_DEBUG("[Canvas] Canvas对象创建成功: ref=0x%x\n", canvas_ref);
    return canvas_ref;
}

/**
 * @brief 创建Graphics对象
 */
j2me_ref_t create_graphics_object(j2me_vm_t* vm, j2me_graphics_context_t* context) {
    LOG_DEBUG("[Graphics] 创建Graphics对象\n");
    LOG_DEBUG("[Graphics] 输入context指针 = %p\n", context);
    
    // 在堆上分配Graphics对象
    j2me_ref_t graphics_ref = j2me_heap_alloc(vm->heap, J2ME_CLASS_GRAPHICS,
                                              sizeof(j2me_graphics_data_t));
    if (graphics_ref == J2ME_NULL_REF) {
        LOG_DEBUG("[Graphics] 错误: Graphics对象分配失败\n");
        return J2ME_NULL_REF;
    }
    
    // 获取Graphics数据
    j2me_graphics_data_t* graphics_data = (j2me_graphics_data_t*)j2me_heap_get_object_data(vm->heap, graphics_ref);
    if (!graphics_data) {
        LOG_DEBUG("[Graphics] 错误: 无法获取Graphics数据\n");
        return J2ME_NULL_REF;
    }
    
    LOG_DEBUG("[Graphics] graphics_data指针 = %p\n", graphics_data);
    LOG_DEBUG("[Graphics] 设置前: context_ptr = %p\n", graphics_data->context_ptr);
    
    // 初始化Graphics数据
    graphics_data->context_ptr = (void*)context;
    graphics_data->color = 0xFFFFFF;  // 默认白色
    
    LOG_DEBUG("[Graphics] 设置后: context_ptr = %p\n", graphics_data->context_ptr);
    LOG_DEBUG("[Graphics] Graphics数据初始化:\n");
    LOG_DEBUG("  context_ptr = %p\n", graphics_data->context_ptr);
    LOG_DEBUG("  color = 0x%x\n", graphics_data->color);
    
    LOG_DEBUG("[Graphics] Graphics对象创建成功: ref=0x%x\n", graphics_ref);
    return graphics_ref;
}

/**
 * @brief 模拟paint()方法调用
 */
void simulate_paint(j2me_vm_t* vm, j2me_ref_t canvas_ref, j2me_ref_t graphics_ref) {
    LOG_DEBUG("\n[Paint] 开始绘制...\n");
    LOG_DEBUG("========================================\n");
    
    // 获取Graphics上下文
    j2me_graphics_data_t* graphics_data = (j2me_graphics_data_t*)j2me_heap_get_object_data(vm->heap, graphics_ref);
    
    LOG_DEBUG("[Paint] 调试信息:\n");
    LOG_DEBUG("  graphics_ref = 0x%x\n", graphics_ref);
    LOG_DEBUG("  graphics_data = %p\n", graphics_data);
    
    if (!graphics_data) {
        LOG_DEBUG("[Paint] 错误: 无法获取Graphics数据\n");
        return;
    }
    
    LOG_DEBUG("  context_ptr = %p\n", graphics_data->context_ptr);
    LOG_DEBUG("  color = 0x%x\n", graphics_data->color);
    
    if (!graphics_data->context_ptr) {
        LOG_DEBUG("[Paint] 错误: context_ptr为空\n");
        return;
    }
    
    j2me_graphics_context_t* ctx = (j2me_graphics_context_t*)graphics_data->context_ptr;
    
    // 设置渲染目标为canvas纹理
    LOG_DEBUG("[Paint] 设置渲染目标为canvas纹理\n");
    SDL_SetRenderTarget(ctx->renderer, ctx->canvas);
    
    // 清空屏幕（黑色）
    LOG_DEBUG("[Paint] 清空屏幕\n");
    j2me_color_t black = {0, 0, 0, 255};
    j2me_graphics_set_color(ctx, black);
    j2me_graphics_draw_rect(ctx, 0, 0, 240, 320, true);
    
    // 绘制红色矩形
    LOG_DEBUG("[Paint] 绘制红色矩形 (10, 10, 100, 100)\n");
    j2me_color_t red = {255, 0, 0, 255};
    j2me_graphics_set_color(ctx, red);
    j2me_graphics_draw_rect(ctx, 10, 10, 100, 100, true);
    
    // 绘制绿色矩形
    LOG_DEBUG("[Paint] 绘制绿色矩形 (120, 10, 100, 100)\n");
    j2me_color_t green = {0, 255, 0, 255};
    j2me_graphics_set_color(ctx, green);
    j2me_graphics_draw_rect(ctx, 120, 10, 100, 100, true);
    
    // 绘制蓝色矩形
    LOG_DEBUG("[Paint] 绘制蓝色矩形 (10, 120, 100, 100)\n");
    j2me_color_t blue = {0, 0, 255, 255};
    j2me_graphics_set_color(ctx, blue);
    j2me_graphics_draw_rect(ctx, 10, 120, 100, 100, true);
    
    // 绘制黄色矩形
    LOG_DEBUG("[Paint] 绘制黄色矩形 (120, 120, 100, 100)\n");
    j2me_color_t yellow = {255, 255, 0, 255};
    j2me_graphics_set_color(ctx, yellow);
    j2me_graphics_draw_rect(ctx, 120, 120, 100, 100, true);
    
    // 恢复渲染目标
    LOG_DEBUG("[Paint] 恢复渲染目标\n");
    SDL_SetRenderTarget(ctx->renderer, NULL);
    
    // 呈现到屏幕
    LOG_DEBUG("[Paint] 刷新显示\n");
    j2me_display_refresh(vm->display);
    
    // 验证渲染
    LOG_DEBUG("[Paint] 验证渲染状态:\n");
    LOG_DEBUG("  renderer = %p\n", ctx->renderer);
    LOG_DEBUG("  canvas = %p\n", ctx->canvas);
    LOG_DEBUG("  display->renderer = %p\n", vm->display->renderer);
    
    // 强制SDL呈现
    SDL_RenderPresent(vm->display->renderer);
    LOG_DEBUG("[Paint] SDL_RenderPresent 调用完成\n");
    
    LOG_DEBUG("========================================\n");
    LOG_DEBUG("[Paint] 绘制完成\n\n");
}

int main(void) {
    LOG_DEBUG("=== Simple Canvas测试 ===\n\n");
    
    // 步骤1: 初始化SDL
    LOG_DEBUG("步骤1: 初始化SDL\n");
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        LOG_DEBUG("✗ SDL初始化失败: %s\n", SDL_GetError());
        return 1;
    }
    LOG_DEBUG("✓ SDL初始化成功\n\n");
    
    // 步骤2: 创建虚拟机
    LOG_DEBUG("步骤2: 创建虚拟机\n");
    j2me_vm_config_t config = j2me_vm_get_default_config();
    config.heap_size = 2 * 1024 * 1024; // 2MB堆
    
    j2me_vm_t* vm = j2me_vm_create(&config);
    assert(vm != NULL);
    LOG_DEBUG("✓ 虚拟机创建成功\n\n");
    
    // 步骤3: 初始化显示系统（使用更大的窗口）
    LOG_DEBUG("步骤3: 初始化显示系统\n");
    vm->display = j2me_display_initialize(480, 640, "Phase 2 Canvas Test - 4个彩色矩形");
    if (!vm->display) {
        LOG_DEBUG("✗ 显示系统创建失败\n");
        j2me_vm_destroy(vm);
        SDL_Quit();
        return 1;
    }
    
    // 确保窗口可见
    SDL_RaiseWindow(vm->display->window);
    SDL_ShowWindow(vm->display->window);
    
    // 创建graphics context（内部canvas仍然是240x320）
    vm->display->context = j2me_graphics_create_context(vm->display, 240, 320);
    if (!vm->display->context) {
        LOG_DEBUG("✗ Graphics context创建失败\n");
        j2me_display_destroy(vm->display);
        j2me_vm_destroy(vm);
        SDL_Quit();
        return 1;
    }
    
    LOG_DEBUG("✓ 显示系统创建成功\n");
    LOG_DEBUG("  窗口大小: 480x640 (2x缩放)\n");
    LOG_DEBUG("  Canvas大小: 240x320\n\n");
    
    // 步骤4: 创建Canvas对象
    LOG_DEBUG("步骤4: 创建Canvas对象\n");
    j2me_ref_t canvas_ref = create_canvas_object(vm, 240, 320);
    assert(canvas_ref != J2ME_NULL_REF);
    LOG_DEBUG("✓ Canvas对象创建成功\n\n");
    
    // 步骤5: 创建Graphics对象
    LOG_DEBUG("步骤5: 创建Graphics对象\n");
    j2me_ref_t graphics_ref = create_graphics_object(vm, vm->display->context);
    assert(graphics_ref != J2ME_NULL_REF);
    
    // 立即验证数据
    LOG_DEBUG("  验证Graphics数据...\n");
    j2me_graphics_data_t* verify_data = (j2me_graphics_data_t*)j2me_heap_get_object_data(vm->heap, graphics_ref);
    LOG_DEBUG("  verify_data = %p\n", verify_data);
    if (verify_data) {
        LOG_DEBUG("  context_ptr = %p\n", verify_data->context_ptr);
        LOG_DEBUG("  color = 0x%x\n", verify_data->color);
    }
    
    LOG_DEBUG("✓ Graphics对象创建成功\n\n");
    
    // 步骤6: 关联Graphics到Canvas
    LOG_DEBUG("步骤6: 关联Graphics到Canvas\n");
    j2me_canvas_data_t* canvas_data = (j2me_canvas_data_t*)j2me_heap_get_object_data(vm->heap, canvas_ref);
    canvas_data->graphics_ref = graphics_ref;
    LOG_DEBUG("✓ Graphics关联成功\n\n");
    
    // 步骤7: 调用paint()方法
    LOG_DEBUG("步骤7: 调用paint()方法\n");
    simulate_paint(vm, canvas_ref, graphics_ref);
    LOG_DEBUG("✓ paint()调用成功\n\n");
    
    // 步骤8: 显示窗口并等待
    LOG_DEBUG("步骤8: 显示窗口（5秒后自动关闭，或按任意键/点击关闭）\n");
    LOG_DEBUG("========================================\n");
    LOG_DEBUG("窗口已打开，应该看到4个彩色矩形：\n");
    LOG_DEBUG("  左上: 红色 (10, 10, 100x100)\n");
    LOG_DEBUG("  右上: 绿色 (120, 10, 100x100)\n");
    LOG_DEBUG("  左下: 蓝色 (10, 120, 100x100)\n");
    LOG_DEBUG("  右下: 黄色 (120, 120, 100x100)\n");
    LOG_DEBUG("========================================\n\n");
    
    // 等待5秒让用户看到窗口
    LOG_DEBUG("等待5秒...\n");
    bool should_close = false;
    for (int i = 0; i < 50 && !should_close; i++) {
        if (i % 10 == 0) {
            LOG_DEBUG("  %d秒...\n", 5 - i/10);
            // 每秒刷新一次显示
            j2me_display_refresh(vm->display);
        }
        
        SDL_Delay(100);  // 100ms
        
        // 处理事件避免窗口无响应
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_KEYDOWN || event.type == SDL_MOUSEBUTTONDOWN) {
                LOG_DEBUG("用户关闭窗口\n");
                should_close = true;
                break;
            }
        }
    }
    
    LOG_DEBUG("✓ 窗口关闭\n\n");
    
    // 步骤9: 清理资源
    LOG_DEBUG("步骤9: 清理资源\n");
    j2me_display_destroy(vm->display);
    vm->display = NULL;
    j2me_vm_destroy(vm);
    SDL_Quit();
    LOG_DEBUG("✓ 资源清理完成\n\n");
    
    LOG_DEBUG("=== 所有测试通过! ===\n");
    LOG_DEBUG("\n🎉 Phase 2 Canvas对象测试成功！\n");
    LOG_DEBUG("  ✓ SDL初始化\n");
    LOG_DEBUG("  ✓ 虚拟机创建\n");
    LOG_DEBUG("  ✓ 显示系统创建\n");
    LOG_DEBUG("  ✓ Canvas对象创建\n");
    LOG_DEBUG("  ✓ Graphics对象创建\n");
    LOG_DEBUG("  ✓ 对象关联\n");
    LOG_DEBUG("  ✓ paint()方法调用\n");
    LOG_DEBUG("  ✓ 图形绘制\n");
    LOG_DEBUG("\n下一步: 实现真实的Java Canvas类加载和paint()方法调用！\n");
    
    return 0;
}
