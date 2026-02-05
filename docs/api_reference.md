# J2ME模拟器API参考

## 核心API

### 虚拟机管理

#### j2me_vm_create
```c
j2me_vm_t* j2me_vm_create(const j2me_vm_config_t* config);
```
创建虚拟机实例。

**参数:**
- `config`: 虚拟机配置结构指针

**返回值:**
- 成功: 虚拟机实例指针
- 失败: NULL

**示例:**
```c
j2me_vm_config_t config = j2me_vm_get_default_config();
config.heap_size = 2 * 1024 * 1024; // 2MB堆
j2me_vm_t* vm = j2me_vm_create(&config);
```

#### j2me_vm_initialize
```c
j2me_error_t j2me_vm_initialize(j2me_vm_t* vm);
```
初始化虚拟机。

**参数:**
- `vm`: 虚拟机实例

**返回值:**
- `J2ME_SUCCESS`: 初始化成功
- 其他: 错误码

#### j2me_vm_start
```c
j2me_error_t j2me_vm_start(j2me_vm_t* vm, const char* main_class);
```
启动虚拟机并执行主类。

**参数:**
- `vm`: 虚拟机实例
- `main_class`: 主类名称

**返回值:**
- `J2ME_SUCCESS`: 启动成功
- 其他: 错误码

### 解释器API

#### j2me_operand_stack_push
```c
j2me_error_t j2me_operand_stack_push(j2me_operand_stack_t* stack, j2me_int value);
```
将值压入操作数栈。

**参数:**
- `stack`: 操作数栈指针
- `value`: 要压入的值

**返回值:**
- `J2ME_SUCCESS`: 操作成功
- `J2ME_ERROR_STACK_OVERFLOW`: 栈溢出

#### j2me_operand_stack_pop
```c
j2me_error_t j2me_operand_stack_pop(j2me_operand_stack_t* stack, j2me_int* value);
```
从操作数栈弹出值。

**参数:**
- `stack`: 操作数栈指针
- `value`: 输出值指针

**返回值:**
- `J2ME_SUCCESS`: 操作成功
- `J2ME_ERROR_INVALID_PARAMETER`: 栈为空或参数无效

### 图形API

#### j2me_display_initialize
```c
j2me_display_t* j2me_display_initialize(int width, int height, const char* title);
```
初始化显示系统。

**参数:**
- `width`: 窗口宽度
- `height`: 窗口高度
- `title`: 窗口标题

**返回值:**
- 成功: 显示系统指针
- 失败: NULL

#### j2me_graphics_set_color
```c
void j2me_graphics_set_color(j2me_graphics_context_t* context, j2me_color_t color);
```
设置绘制颜色。

**参数:**
- `context`: 图形上下文
- `color`: RGBA颜色值

**示例:**
```c
j2me_color_t red = {255, 0, 0, 255};
j2me_graphics_set_color(context, red);
```

#### j2me_graphics_draw_rect
```c
void j2me_graphics_draw_rect(j2me_graphics_context_t* context, int x, int y, int width, int height, bool filled);
```
绘制矩形。

**参数:**
- `context`: 图形上下文
- `x`, `y`: 矩形位置
- `width`, `height`: 矩形尺寸
- `filled`: 是否填充

## 数据结构

### j2me_vm_config_t
虚拟机配置结构。

```c
typedef struct {
    size_t heap_size;           // 堆大小 (字节)
    size_t stack_size;          // 栈大小 (字节)
    size_t max_threads;         // 最大线程数
    bool enable_gc;             // 是否启用垃圾回收
    bool enable_jit;            // 是否启用JIT编译
} j2me_vm_config_t;
```

### j2me_color_t
颜色结构。

```c
typedef struct {
    uint8_t r, g, b, a;         // RGBA分量 (0-255)
} j2me_color_t;
```

### j2me_operand_stack_t
操作数栈结构。

```c
typedef struct {
    j2me_int* data;             // 栈数据
    size_t size;                // 栈大小
    size_t top;                 // 栈顶位置
} j2me_operand_stack_t;
```

## 错误码

### j2me_error_t
```c
typedef enum {
    J2ME_SUCCESS = 0,                   // 成功
    J2ME_ERROR_OUT_OF_MEMORY,          // 内存不足
    J2ME_ERROR_INVALID_PARAMETER,      // 无效参数
    J2ME_ERROR_CLASS_NOT_FOUND,        // 类未找到
    J2ME_ERROR_METHOD_NOT_FOUND,       // 方法未找到
    J2ME_ERROR_STACK_OVERFLOW,         // 栈溢出
    J2ME_ERROR_ILLEGAL_ACCESS,         // 非法访问
    J2ME_ERROR_RUNTIME_EXCEPTION       // 运行时异常
} j2me_error_t;
```

## 使用示例

### 基本虚拟机使用
```c
#include "j2me_vm.h"
#include "j2me_graphics.h"

int main() {
    // 创建虚拟机
    j2me_vm_config_t config = j2me_vm_get_default_config();
    j2me_vm_t* vm = j2me_vm_create(&config);
    
    // 初始化显示
    j2me_display_t* display = j2me_display_initialize(240, 320, "J2ME App");
    
    // 启动虚拟机
    j2me_vm_initialize(vm);
    j2me_vm_start(vm, "com.example.MainMIDlet");
    
    // 主循环
    bool running = true;
    while (running) {
        j2me_vm_execute_time_slice(vm, 16); // 60 FPS
        // 处理事件...
    }
    
    // 清理
    j2me_vm_destroy(vm);
    j2me_display_destroy(display);
    return 0;
}
```

### 图形绘制示例
```c
void draw_scene(j2me_graphics_context_t* ctx) {
    // 清除画布
    j2me_graphics_clear(ctx);
    
    // 绘制红色矩形
    j2me_color_t red = {255, 0, 0, 255};
    j2me_graphics_set_color(ctx, red);
    j2me_graphics_draw_rect(ctx, 10, 10, 100, 50, true);
    
    // 绘制蓝色边框
    j2me_color_t blue = {0, 0, 255, 255};
    j2me_graphics_set_color(ctx, blue);
    j2me_graphics_draw_rect(ctx, 5, 5, 110, 60, false);
}
```

## 性能注意事项

### 内存管理
- 及时释放不需要的资源
- 避免频繁的内存分配
- 使用对象池减少GC压力

### 图形优化
- 批量绘制操作
- 避免不必要的重绘
- 使用裁剪区域优化

### 解释器优化
- 减少栈操作
- 优化热点代码路径
- 考虑使用JIT编译