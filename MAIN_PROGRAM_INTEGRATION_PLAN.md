# 主程序集成计划

## 日期: 2026-02-06

## 🎯 目标

将Phase 1的堆对象系统集成到`src/main.c`，使其能够运行真实的J2ME游戏。

## 📋 当前问题分析

### 问题1: 使用假对象引用
**位置**: `src/main.c` 主循环中
```c
// 错误：使用假的对象引用
vm->current_canvas_ref = 0x20000001;
```

**影响**: 
- Canvas对象不是真实的堆对象
- 无法正确调用Java的paint()方法
- 游戏逻辑无法正常执行

### 问题2: 渲染系统已经工作
**好消息**: 
- SDL窗口创建成功 ✓
- 图形上下文创建成功 ✓
- `call_canvas_paint_method()` 已经实现 ✓
- 能够调用真实的Java paint()方法 ✓

**问题**: 
- Canvas对象引用是假的，导致无法找到真实的Canvas类
- Graphics对象引用也是假的

## 🔧 解决方案

### 方案概述
不需要创建新的测试程序！只需要修改`src/main.c`，让它：
1. 使用Phase 1的堆系统创建真实的Canvas对象
2. 使用Phase 1的堆系统创建真实的Graphics对象
3. 让游戏的Canvas.paint()方法能够正确执行

### 关键修改点

#### 修改1: 在Display.setCurrent中创建真实Canvas对象

**当前代码** (`src/core/j2me_native_methods.c`):
```c
// 保存当前Canvas到VM
vm->current_canvas_ref = canvas_ref;  // canvas_ref可能是假引用
```

**需要做的**:
- 检查`canvas_ref`是否是真实的堆对象
- 如果不是，需要在游戏启动时创建真实的Canvas对象

#### 修改2: 创建真实的Graphics对象

**当前代码** (`src/core/j2me_native_methods.c`):
```c
// 创建Graphics对象引用
j2me_int graphics_ref = 0x40000001; // 假引用！
```

**需要改为**:
```c
// 在堆上创建真实的Graphics对象
j2me_ref_t graphics_ref = j2me_heap_alloc_object(
    vm->heap,
    GRAPHICS_CLASS_ID,
    sizeof(j2me_graphics_object_t)
);

// 初始化Graphics对象
j2me_graphics_object_t* graphics_obj = 
    (j2me_graphics_object_t*)j2me_heap_get_object(vm->heap, graphics_ref);
graphics_obj->context = vm->display->context;
graphics_obj->color = 0x000000; // 黑色
```

#### 修改3: 简化主循环

**当前代码** (`src/main.c`):
```c
// 复杂的手动调用逻辑
if (vm->state == J2ME_VM_RUNNING && vm->current_canvas_ref != 0) {
    // 手动创建栈帧
    // 手动调用repaint
    // ...
}
```

**应该改为**:
```c
// 简单的游戏循环
if (vm->state == J2ME_VM_RUNNING) {
    // 执行游戏线程
    j2me_vm_execute_time_slice(vm, delta_time);
    
    // 处理事件（包括Canvas重绘）
    j2me_vm_handle_events(vm);
    
    // 刷新显示
    if (vm->display) {
        j2me_display_refresh(vm->display);
    }
}
```

## 📝 实施步骤

### 步骤1: 定义Canvas和Graphics对象结构

在`include/j2me_heap.h`中添加：
```c
// Canvas对象结构（在堆上分配）
typedef struct {
    int width;
    int height;
    j2me_ref_t graphics_ref;  // Graphics对象引用
} j2me_canvas_object_t;

// Graphics对象结构（在堆上分配）
typedef struct {
    j2me_graphics_context_t* context;  // SDL图形上下文
    int color;                         // 当前颜色
} j2me_graphics_object_t;

// 类ID定义
#define CANVAS_CLASS_ID   0x0001
#define GRAPHICS_CLASS_ID 0x0002
```

### 步骤2: 实现Canvas对象创建函数

在`src/core/j2me_heap.c`中添加：
```c
/**
 * @brief 创建Canvas对象
 */
j2me_ref_t j2me_heap_create_canvas(j2me_heap_t* heap, int width, int height) {
    // 分配Canvas对象
    j2me_ref_t canvas_ref = j2me_heap_alloc_object(
        heap,
        CANVAS_CLASS_ID,
        sizeof(j2me_canvas_object_t)
    );
    
    if (canvas_ref == 0) {
        return 0;
    }
    
    // 初始化Canvas对象
    j2me_canvas_object_t* canvas = 
        (j2me_canvas_object_t*)j2me_heap_get_object_data(heap, canvas_ref);
    
    canvas->width = width;
    canvas->height = height;
    canvas->graphics_ref = 0;  // 稍后创建
    
    LOG_DEBUG("[堆] 创建Canvas对象: ref=0x%x, 大小=%dx%d\n", 
           canvas_ref, width, height);
    
    return canvas_ref;
}

/**
 * @brief 创建Graphics对象
 */
j2me_ref_t j2me_heap_create_graphics(j2me_heap_t* heap, 
                                     j2me_graphics_context_t* context) {
    // 分配Graphics对象
    j2me_ref_t graphics_ref = j2me_heap_alloc_object(
        heap,
        GRAPHICS_CLASS_ID,
        sizeof(j2me_graphics_object_t)
    );
    
    if (graphics_ref == 0) {
        return 0;
    }
    
    // 初始化Graphics对象
    j2me_graphics_object_t* graphics = 
        (j2me_graphics_object_t*)j2me_heap_get_object_data(heap, graphics_ref);
    
    graphics->context = context;
    graphics->color = 0x000000;  // 黑色
    
    LOG_DEBUG("[堆] 创建Graphics对象: ref=0x%x\n", graphics_ref);
    
    return graphics_ref;
}
```

### 步骤3: 修改midp_canvas_call_paint_method

在`src/core/j2me_native_methods.c`中修改：
```c
static j2me_error_t midp_canvas_call_paint_method(j2me_vm_t* vm, j2me_int canvas_ref) {
    if (!vm || !vm->heap) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    LOG_DEBUG("[MIDP Canvas] 调用Canvas.paint()，Canvas引用: 0x%x\n", canvas_ref);
    
    // 创建真实的Graphics对象
    j2me_ref_t graphics_ref = j2me_heap_create_graphics(
        vm->heap,
        vm->display->context
    );
    
    if (graphics_ref == 0) {
        LOG_DEBUG("[MIDP Canvas] 错误: 无法创建Graphics对象\n");
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    // 调用真实的Canvas.paint方法
    j2me_error_t result = call_canvas_paint_method(vm, canvas_ref, graphics_ref);
    
    // 释放Graphics对象
    j2me_heap_release_ref(vm->heap, graphics_ref);
    
    return result;
}
```

### 步骤4: 修改Display.setCurrent

在`src/core/j2me_native_methods.c`中修改：
```c
// 在Display.setCurrent中
if (strcmp(method_name, "setCurrent") == 0) {
    // ... 弹出参数 ...
    
    // 检查是否是真实的堆对象
    if (vm->heap && j2me_heap_is_valid_ref(vm->heap, displayable_ref)) {
        // 是真实的堆对象，直接使用
        vm->current_canvas_ref = displayable_ref;
        LOG_DEBUG("[MIDP Display] 设置Canvas: 0x%x (真实堆对象)\n", displayable_ref);
    } else {
        // 不是真实的堆对象，创建一个
        j2me_ref_t canvas_ref = j2me_heap_create_canvas(
            vm->heap,
            240,  // 默认宽度
            320   // 默认高度
        );
        
        if (canvas_ref != 0) {
            vm->current_canvas_ref = canvas_ref;
            LOG_DEBUG("[MIDP Display] 创建新Canvas: 0x%x\n", canvas_ref);
        } else {
            LOG_DEBUG("[MIDP Display] 错误: 无法创建Canvas对象\n");
        }
    }
    
    // 触发初始绘制
    if (vm->current_canvas_ref != 0) {
        j2me_stack_frame_t* frame = j2me_stack_frame_create(10, 5);
        if (frame) {
            j2me_operand_stack_push(&frame->operand_stack, vm->current_canvas_ref);
            midp_canvas_repaint(vm, frame, NULL);
            j2me_stack_frame_destroy(frame);
        }
    }
    
    return J2ME_SUCCESS;
}
```

### 步骤5: 简化主循环

在`src/main.c`中修改：
```c
// 主循环
while (running) {
    uint32_t current_time = SDL_GetTicks();
    uint32_t delta_time = current_time - last_time;
    
    // 处理事件
    handle_events(&running, input_manager);
    
    // 执行虚拟机时间片
    if (delta_time >= frame_time) {
        // 执行游戏逻辑
        j2me_vm_execute_time_slice(vm, delta_time);
        
        // 处理虚拟机事件（包括Canvas重绘）
        j2me_vm_handle_events(vm);
        
        // 如果有Canvas，触发重绘
        if (vm->state == J2ME_VM_RUNNING && vm->current_canvas_ref != 0) {
            // 每5帧触发一次重绘
            static int repaint_counter = 0;
            repaint_counter++;
            
            if (repaint_counter % 5 == 0) {
                j2me_stack_frame_t* frame = j2me_stack_frame_create(10, 5);
                if (frame) {
                    j2me_operand_stack_push(&frame->operand_stack, vm->current_canvas_ref);
                    midp_canvas_repaint(vm, frame, NULL);
                    j2me_stack_frame_destroy(frame);
                }
            }
        }
        
        last_time = current_time;
    }
    
    // 避免CPU占用过高
    SDL_Delay(1);
}
```

## ✅ 预期结果

完成这些修改后：

1. **游戏启动时**:
   - 加载JAR文件 ✓
   - 解析class文件 ✓
   - 启动MIDlet ✓
   - 调用Display.setCurrent() ✓
   - 创建真实的Canvas对象 ✓

2. **游戏运行时**:
   - 主循环执行游戏逻辑 ✓
   - 定期调用Canvas.repaint() ✓
   - repaint()调用真实的Java paint()方法 ✓
   - paint()方法绘制游戏图形 ✓
   - 图形显示在窗口中 ✓

3. **用户看到**:
   - 窗口打开 ✓
   - 显示游戏图形 ✓
   - 图形随游戏逻辑更新 ✓

## 🎯 关键点

1. **不要创建新的测试程序** - 修复主程序即可
2. **使用Phase 1的堆系统** - 创建真实的对象
3. **简化主循环** - 让VM自己管理游戏逻辑
4. **信任现有代码** - 渲染系统已经工作，只需要真实的对象

## 📊 工作量估算

- 定义对象结构: 30分钟
- 实现创建函数: 1小时
- 修改native方法: 1小时
- 修改主循环: 30分钟
- 测试和调试: 1-2小时

**总计**: 4-5小时

## 🚀 下一步

1. 实施上述修改
2. 编译并运行
3. 测试真实的JAR文件
4. 如果看到游戏图形，Phase 2完成！

---

**创建日期**: 2026年2月6日  
**状态**: 待实施  
**优先级**: 最高
