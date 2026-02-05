#ifndef J2ME_EXCEPTION_H
#define J2ME_EXCEPTION_H

#include "j2me_types.h"
#include <stdbool.h>

/**
 * @file j2me_exception.h
 * @brief J2ME异常处理系统头文件
 * 
 * 完整的Java异常处理机制，包括异常抛出、捕获和栈跟踪
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 最大栈跟踪深度
 */
#define J2ME_MAX_STACK_TRACE_DEPTH 64

/**
 * @brief 栈跟踪元素
 */
typedef struct {
    char* class_name;       /**< 类名 */
    char* method_name;      /**< 方法名 */
    char* file_name;        /**< 文件名 */
    int line_number;        /**< 行号 */
    int pc;                 /**< 程序计数器 */
} j2me_stack_trace_element_t;

/**
 * @brief 异常对象
 */
typedef struct {
    char* exception_class;                      /**< 异常类名 */
    char* message;                              /**< 异常消息 */
    j2me_stack_trace_element_t* stack_trace;    /**< 栈跟踪 */
    int stack_trace_count;                      /**< 栈跟踪元素数量 */
} j2me_exception_t;

/**
 * @brief 创建异常对象
 * @param exception_class 异常类名
 * @param message 异常消息
 * @return 异常对象
 */
j2me_exception_t* j2me_exception_create(const char* exception_class, const char* message);

/**
 * @brief 销毁异常对象
 * @param exception 异常对象
 */
void j2me_exception_destroy(j2me_exception_t* exception);

/**
 * @brief 生成栈跟踪
 * @param vm 虚拟机实例
 * @param exception 异常对象
 * @return 错误码
 */
j2me_error_t j2me_exception_generate_stack_trace(j2me_vm_t* vm, j2me_exception_t* exception);

/**
 * @brief 打印栈跟踪
 * @param exception 异常对象
 */
void j2me_exception_print_stack_trace(j2me_exception_t* exception);

/**
 * @brief 抛出异常
 * @param vm 虚拟机实例
 * @param exception_class 异常类名
 * @param message 异常消息
 * @return 错误码
 */
j2me_error_t j2me_throw_exception(j2me_vm_t* vm, const char* exception_class, const char* message);

/**
 * @brief 处理异常
 * @param vm 虚拟机实例
 * @param exception 异常对象
 * @return 错误码
 */
j2me_error_t j2me_handle_exception(j2me_vm_t* vm, j2me_exception_t* exception);

/**
 * @brief 清除当前异常
 * @param vm 虚拟机实例
 */
void j2me_clear_exception(j2me_vm_t* vm);

/**
 * @brief 检查是否有未处理的异常
 * @param vm 虚拟机实例
 * @return 是否有异常
 */
bool j2me_has_pending_exception(j2me_vm_t* vm);

/**
 * @brief 获取当前异常
 * @param vm 虚拟机实例
 * @return 当前异常对象
 */
j2me_exception_t* j2me_get_current_exception(j2me_vm_t* vm);

/**
 * @brief 抛出空指针异常
 * @param vm 虚拟机实例
 * @return 错误码
 */
j2me_error_t j2me_throw_null_pointer_exception(j2me_vm_t* vm);

/**
 * @brief 抛出数组越界异常
 * @param vm 虚拟机实例
 * @param index 越界索引
 * @param length 数组长度
 * @return 错误码
 */
j2me_error_t j2me_throw_array_index_out_of_bounds_exception(j2me_vm_t* vm, int index, int length);

/**
 * @brief 抛出算术异常
 * @param vm 虚拟机实例
 * @param message 异常消息
 * @return 错误码
 */
j2me_error_t j2me_throw_arithmetic_exception(j2me_vm_t* vm, const char* message);

/**
 * @brief 抛出类转换异常
 * @param vm 虚拟机实例
 * @param from_class 源类名
 * @param to_class 目标类名
 * @return 错误码
 */
j2me_error_t j2me_throw_class_cast_exception(j2me_vm_t* vm, const char* from_class, const char* to_class);

/**
 * @brief 抛出类未找到异常
 * @param vm 虚拟机实例
 * @param class_name 类名
 * @return 错误码
 */
j2me_error_t j2me_throw_class_not_found_exception(j2me_vm_t* vm, const char* class_name);

/**
 * @brief 抛出方法未找到异常
 * @param vm 虚拟机实例
 * @param class_name 类名
 * @param method_name 方法名
 * @return 错误码
 */
j2me_error_t j2me_throw_no_such_method_exception(j2me_vm_t* vm, const char* class_name, const char* method_name);

/**
 * @brief 抛出内存不足异常
 * @param vm 虚拟机实例
 * @return 错误码
 */
j2me_error_t j2me_throw_out_of_memory_exception(j2me_vm_t* vm);

/**
 * @brief 抛出栈溢出异常
 * @param vm 虚拟机实例
 * @return 错误码
 */
j2me_error_t j2me_throw_stack_overflow_exception(j2me_vm_t* vm);

#ifdef __cplusplus
}
#endif

#endif // J2ME_EXCEPTION_H