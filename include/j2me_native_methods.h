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

/**
 * @brief 调用Canvas对象的paint方法
 * @param vm 虚拟机实例
 * @param canvas_ref Canvas对象引用
 * @param graphics_ref Graphics对象引用
 * @return 错误码
 */
j2me_error_t call_canvas_paint_method(j2me_vm_t* vm, j2me_int canvas_ref, j2me_int graphics_ref);

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
j2me_error_t midp_graphics_draw_round_rect(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_graphics_fill_round_rect(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_graphics_fill_arc(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_graphics_fill_triangle(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_graphics_draw_rgb(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_graphics_get_display_color(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);

// MIDP Font类本地方法
j2me_error_t midp_font_char_width(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_font_chars_width(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_font_string_width(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_font_substring_width(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_font_init(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);

// MIDP Canvas事件处理本地方法
j2me_error_t midp_canvas_key_pressed(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_canvas_key_released(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_canvas_pointer_pressed(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_canvas_pointer_released(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_canvas_pointer_dragged(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);

// MIDP Image类本地方法
j2me_error_t midp_image_create_image(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_image_create_image_from_file(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_image_get_width(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_image_get_height(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_graphics_draw_image(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);

// MIDP MIDlet类本地方法
j2me_error_t midp_midlet_platform_request(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_midlet_destroy_app(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t midp_midlet_notify_destroyed(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);

// Java String类本地方法 (基础)
j2me_error_t java_string_length(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_string_char_at(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_string_substring(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);

// Java Thread类本地方法
j2me_error_t java_thread_start(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_thread_sleep(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_thread_yield(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_thread_current_thread(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);

// Java Object类本地方法
j2me_error_t java_object_get_class(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_object_hash_code(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_object_notify(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_object_notify_all(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_object_wait(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);

// Java System类本地方法
j2me_error_t java_system_out_println(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_system_out_print(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_system_current_time_millis(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_system_arraycopy(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_system_identity_hash_code(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_system_get_property0(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);

// Java String类本地方法
j2me_error_t java_string_equals(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_string_hash_code(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_string_index_of(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_string_index_of_from(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_string_last_index_of(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_string_last_index_of_from(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_string_intern(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);

// Java Math类本地方法
j2me_error_t java_math_sin(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_math_cos(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_math_tan(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_math_sqrt(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_math_ceil(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_math_floor(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);

// Java Runtime类本地方法
j2me_error_t java_runtime_gc(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_runtime_free_memory(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_runtime_total_memory(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_runtime_exit_internal(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);

// Java Throwable类本地方法
j2me_error_t java_throwable_print_stack_trace(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);
j2me_error_t java_throwable_fill_in_stack_trace(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args);

#endif // J2ME_NATIVE_METHODS_H
