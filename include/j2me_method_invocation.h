#ifndef J2ME_METHOD_INVOCATION_H
#define J2ME_METHOD_INVOCATION_H

#include "j2me_types.h"
#include "j2me_vm.h"
#include "j2me_class.h"
#include "j2me_interpreter.h"
#include "j2me_exception.h"
#include "j2me_field_access.h"

/**
 * @file j2me_method_invocation.h
 * @brief J2ME方法调用系统头文件
 * 
 * 完整的方法调用机制，支持虚方法、静态方法、特殊方法和接口方法调用
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 方法调用上下文
 */
typedef struct {
    j2me_vm_t* vm;                          /**< 虚拟机实例 */
    j2me_stack_frame_t* caller_frame;       /**< 调用者栈帧 */
    j2me_method_t* method;                  /**< 被调用方法 */
    j2me_value_t* args;                     /**< 方法参数 */
    int arg_count;                          /**< 参数数量 */
    j2me_value_t return_value;              /**< 返回值 */
    j2me_exception_t* exception;            /**< 异常信息 */
} j2me_method_invocation_context_t;

/**
 * @brief 创建方法调用上下文
 * @param vm 虚拟机实例
 * @param caller_frame 调用者栈帧
 * @param method 被调用方法
 * @param args 方法参数
 * @param arg_count 参数数量
 * @return 方法调用上下文
 */
j2me_method_invocation_context_t* j2me_method_invocation_create_context(
    j2me_vm_t* vm,
    j2me_stack_frame_t* caller_frame,
    j2me_method_t* method,
    j2me_value_t* args,
    int arg_count);

/**
 * @brief 销毁方法调用上下文
 * @param context 方法调用上下文
 */
void j2me_method_invocation_destroy_context(j2me_method_invocation_context_t* context);

/**
 * @brief 解析方法引用
 * @param vm 虚拟机实例
 * @param class_info 当前类
 * @param method_ref_index 方法引用索引
 * @param resolved_method 输出解析的方法
 * @return 错误码
 */
j2me_error_t j2me_method_invocation_resolve_method_ref(
    j2me_vm_t* vm,
    j2me_class_t* class_info,
    uint16_t method_ref_index,
    j2me_method_t** resolved_method);

/**
 * @brief 准备方法参数
 * @param context 方法调用上下文
 * @param caller_stack 调用者操作数栈
 * @return 错误码
 */
j2me_error_t j2me_method_invocation_prepare_args(
    j2me_method_invocation_context_t* context,
    j2me_operand_stack_t* caller_stack);

/**
 * @brief 创建新的栈帧用于方法调用
 * @param context 方法调用上下文
 * @return 新的栈帧
 */
j2me_stack_frame_t* j2me_method_invocation_create_frame(j2me_method_invocation_context_t* context);

/**
 * @brief 执行方法调用
 * @param context 方法调用上下文
 * @return 错误码
 */
j2me_error_t j2me_method_invocation_execute(j2me_method_invocation_context_t* context);

/**
 * @brief 调用虚方法
 * @param vm 虚拟机实例
 * @param caller_frame 调用者栈帧
 * @param method_ref_index 方法引用索引
 * @return 错误码
 */
j2me_error_t j2me_method_invocation_invoke_virtual(
    j2me_vm_t* vm,
    j2me_stack_frame_t* caller_frame,
    uint16_t method_ref_index);

/**
 * @brief 调用静态方法
 * @param vm 虚拟机实例
 * @param caller_frame 调用者栈帧
 * @param method_ref_index 方法引用索引
 * @return 错误码
 */
j2me_error_t j2me_method_invocation_invoke_static(
    j2me_vm_t* vm,
    j2me_stack_frame_t* caller_frame,
    uint16_t method_ref_index);

/**
 * @brief 调用特殊方法
 * @param vm 虚拟机实例
 * @param caller_frame 调用者栈帧
 * @param method_ref_index 方法引用索引
 * @return 错误码
 */
j2me_error_t j2me_method_invocation_invoke_special(
    j2me_vm_t* vm,
    j2me_stack_frame_t* caller_frame,
    uint16_t method_ref_index);

/**
 * @brief 调用接口方法
 * @param vm 虚拟机实例
 * @param caller_frame 调用者栈帧
 * @param method_ref_index 方法引用索引
 * @param count 参数数量
 * @return 错误码
 */
j2me_error_t j2me_method_invocation_invoke_interface(
    j2me_vm_t* vm,
    j2me_stack_frame_t* caller_frame,
    uint16_t method_ref_index,
    uint8_t count);

#ifdef __cplusplus
}
#endif

#endif // J2ME_METHOD_INVOCATION_H