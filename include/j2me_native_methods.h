#ifndef J2ME_NATIVE_METHODS_H
#define J2ME_NATIVE_METHODS_H

#include "j2me_types.h"
#include "j2me_vm.h"
#include "j2me_interpreter.h"

/**
 * @file j2me_native_methods.h
 * @brief J2ME本地方法注册和调用系统
 * 
 * 提供MIDP API本地方法的注册和调用机制，
 * 使字节码解释器能够调用C实现的MIDP API
 */

// 本地方法函数指针类型
typedef j2me_error_t (*j2me_native_method_func_t)(j2me_vm_t* vm, 
                                                   j2me_stack_frame_t* frame,
                                                   void* args);

// 本地方法注册项
typedef struct {
    const char* class_name;     // 类名 (如 "javax/microedition/lcdui/Display")
    const char* method_name;    // 方法名 (如 "getDisplay")
    const char* signature;      // 方法签名 (如 "()Ljavax/microedition/lcdui/Display;")
    j2me_native_method_func_t func; // 本地方法实现
} j2me_native_method_entry_t;

// 本地方法注册表
typedef struct j2me_native_method_registry {
    j2me_native_method_entry_t* entries;  // 注册项数组
    size_t count;                         // 注册项数量
    size_t capacity;                      // 容量
} j2me_native_method_registry_t;

/**
 * @brief 创建本地方法注册表
 * @return 注册表指针
 */
j2me_native_method_registry_t* j2me_native_method_registry_create(void);

/**
 * @brief 销毁本地方法注册表
 * @param registry 注册表
 */
void j2me_native_method_registry_destroy(j2me_native_method_registry_t* registry);

/**
 * @brief 注册本地方法
 * @param registry 注册表
 * @param class_name 类名
 * @param method_name 方法名
 * @param signature 方法签名
 * @param func 本地方法实现
 * @return 错误码
 */
j2me_error_t j2me_native_method_register(j2me_native_method_registry_t* registry,
                                         const char* class_name,
                                         const char* method_name,
                                         const char* signature,
                                         j2me_native_method_func_t func);

/**
 * @brief 查找本地方法
 * @param registry 注册表
 * @param class_name 类名
 * @param method_name 方法名
 * @param signature 方法签名
 * @return 本地方法函数指针，未找到返回NULL
 */
j2me_native_method_func_t j2me_native_method_find(j2me_native_method_registry_t* registry,
                                                  const char* class_name,
                                                  const char* method_name,
                                                  const char* signature);

/**
 * @brief 调用本地方法
 * @param vm 虚拟机实例
 * @param frame 当前栈帧
 * @param class_name 类名
 * @param method_name 方法名
 * @param signature 方法签名
 * @param args 参数
 * @return 错误码
 */
j2me_error_t j2me_native_method_invoke(j2me_vm_t* vm,
                                       j2me_stack_frame_t* frame,
                                       const char* class_name,
                                       const char* method_name,
                                       const char* signature,
                                       void* args);

/**
 * @brief 初始化MIDP本地方法
 * @param vm 虚拟机实例
 * @return 错误码
 */
j2me_error_t j2me_midp_native_methods_init(j2me_vm_t* vm);

// MIDP Display类本地方法
j2me_error_t midp_display_get_display(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_display_set_current(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_display_get_current(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);

// MIDP Canvas类本地方法
j2me_error_t midp_canvas_repaint(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_canvas_service_repaints(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_canvas_get_width(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_canvas_get_height(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);

// MIDP Graphics类本地方法
j2me_error_t midp_graphics_set_color_rgb(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_graphics_set_color(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_graphics_get_color(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_graphics_draw_line(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_graphics_draw_rect(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_graphics_fill_rect(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_graphics_draw_string(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_graphics_draw_oval(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_graphics_fill_oval(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_graphics_draw_arc(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);

// MIDP Canvas事件处理本地方法
j2me_error_t midp_canvas_key_pressed(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_canvas_key_released(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_canvas_pointer_pressed(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_canvas_pointer_released(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_canvas_pointer_dragged(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);

#endif // J2ME_NATIVE_METHODS_H