#ifndef J2ME_INTERPRETER_H
#define J2ME_INTERPRETER_H

#include "j2me_types.h"
#include "j2me_class.h"
#include <stddef.h>

/**
 * @file j2me_interpreter.h
 * @brief J2ME字节码解释器
 * 
 * 高性能字节码解释器实现，支持优化的指令分发
 */

// 操作数栈
typedef struct {
    j2me_int* data;             // 栈数据
    size_t size;                // 栈大小
    size_t top;                 // 栈顶位置
} j2me_operand_stack_t;

// 局部变量表
typedef struct {
    j2me_int* variables;        // 变量数组
    size_t size;                // 变量表大小
} j2me_local_vars_t;

// 栈帧
struct j2me_stack_frame {
    j2me_operand_stack_t operand_stack; // 操作数栈
    j2me_local_vars_t local_vars;       // 局部变量表
    uint8_t* bytecode;                  // 字节码指针
    uint32_t pc;                        // 程序计数器
    j2me_stack_frame_t* previous;       // 上一个栈帧
    void* method_info;                  // 方法信息
};

// 线程结构
struct j2me_thread {
    j2me_stack_frame_t* current_frame;  // 当前栈帧
    j2me_stack_frame_t* frame_stack;    // 栈帧栈
    size_t frame_count;                 // 栈帧数量
    j2me_thread_t* next;                // 下一个线程
    uint32_t thread_id;                 // 线程ID
    bool is_running;                    // 是否运行中
};

/**
 * @brief 创建操作数栈
 * @param size 栈大小
 * @return 操作数栈指针
 */
j2me_operand_stack_t* j2me_operand_stack_create(size_t size);

/**
 * @brief 销毁操作数栈
 * @param stack 操作数栈
 */
void j2me_operand_stack_destroy(j2me_operand_stack_t* stack);

/**
 * @brief 压栈操作
 * @param stack 操作数栈
 * @param value 值
 * @return 错误码
 */
j2me_error_t j2me_operand_stack_push(j2me_operand_stack_t* stack, j2me_int value);

/**
 * @brief 出栈操作
 * @param stack 操作数栈
 * @param value 输出值指针
 * @return 错误码
 */
j2me_error_t j2me_operand_stack_pop(j2me_operand_stack_t* stack, j2me_int* value);

/**
 * @brief 创建栈帧
 * @param max_stack 最大栈深度
 * @param max_locals 最大局部变量数
 * @return 栈帧指针
 */
j2me_stack_frame_t* j2me_stack_frame_create(size_t max_stack, size_t max_locals);

/**
 * @brief 销毁栈帧
 * @param frame 栈帧
 */
void j2me_stack_frame_destroy(j2me_stack_frame_t* frame);

/**
 * @brief 执行字节码指令
 * @param vm 虚拟机实例
 * @param thread 线程
 * @return 错误码
 */
j2me_error_t j2me_interpreter_execute_instruction(j2me_vm_t* vm, j2me_thread_t* thread);

/**
 * @brief 执行多条指令
 * @param vm 虚拟机实例
 * @param thread 线程
 * @param max_instructions 最大执行指令数
 * @return 错误码
 */
j2me_error_t j2me_interpreter_execute_batch(j2me_vm_t* vm, j2me_thread_t* thread, uint32_t max_instructions);

/**
 * @brief 执行方法
 * @param vm 虚拟机实例
 * @param method 方法
 * @param object 对象实例 (对于实例方法)
 * @param args 方法参数
 * @return 错误码
 */
j2me_error_t j2me_interpreter_execute_method(j2me_vm_t* vm, j2me_method_t* method, void* object, void* args);

#endif // J2ME_INTERPRETER_H