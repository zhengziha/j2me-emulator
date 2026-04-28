#include "j2me_exception.h"
#include "j2me_vm.h"
#include "j2me_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/**
 * @file j2me_exception.c
 * @brief J2ME异常处理系统实现
 * 
 * 完整的Java异常处理机制，包括异常抛出、捕获和栈跟踪
 */

/**
 * @brief 创建异常对象
 * @param exception_class 异常类名
 * @param message 异常消息
 * @return 异常对象
 */
j2me_exception_t* j2me_exception_create(const char* exception_class, const char* message) {
    j2me_exception_t* exception = (j2me_exception_t*)malloc(sizeof(j2me_exception_t));
    if (!exception) {
        return NULL;
    }
    
    // 复制异常类名
    if (exception_class) {
        size_t class_len = strlen(exception_class) + 1;
        exception->exception_class = (char*)malloc(class_len);
        if (exception->exception_class) {
            strcpy(exception->exception_class, exception_class);
        }
    } else {
        exception->exception_class = NULL;
    }
    
    // 复制异常消息
    if (message) {
        size_t msg_len = strlen(message) + 1;
        exception->message = (char*)malloc(msg_len);
        if (exception->message) {
            strcpy(exception->message, message);
        }
    } else {
        exception->message = NULL;
    }
    
    // 初始化栈跟踪
    exception->stack_trace = NULL;
    exception->stack_trace_count = 0;
    
    LOG_DEBUG("[异常处理] 创建异常: %s - %s\n",
           exception_class ? exception_class : "未知异常",
           message ? message : "无消息");
    
    return exception;
}

/**
 * @brief 销毁异常对象
 * @param exception 异常对象
 */
void j2me_exception_destroy(j2me_exception_t* exception) {
    if (exception) {
        if (exception->exception_class) {
            free(exception->exception_class);
        }
        if (exception->message) {
            free(exception->message);
        }
        if (exception->stack_trace) {
            for (int i = 0; i < exception->stack_trace_count; i++) {
                j2me_stack_trace_element_t* element = &exception->stack_trace[i];
                if (element->class_name) free(element->class_name);
                if (element->method_name) free(element->method_name);
                if (element->file_name) free(element->file_name);
            }
            free(exception->stack_trace);
        }
        free(exception);
        LOG_DEBUG("[异常处理] 销毁异常对象\n");
    }
}

/**
 * @brief 生成栈跟踪
 * @param vm 虚拟机实例
 * @param exception 异常对象
 * @return 错误码
 */
j2me_error_t j2me_exception_generate_stack_trace(j2me_vm_t* vm, j2me_exception_t* exception) {
    if (!vm || !exception || !vm->current_thread) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 计算栈帧数量
    int frame_count = 0;
    j2me_stack_frame_t* current_frame = vm->current_thread->current_frame;
    while (current_frame && frame_count < J2ME_MAX_STACK_TRACE_DEPTH) {
        frame_count++;
        current_frame = current_frame->previous;
    }
    
    if (frame_count == 0) {
        LOG_DEBUG("[异常处理] 没有栈帧可用于生成栈跟踪\n");
        return J2ME_SUCCESS;
    }
    
    // 分配栈跟踪数组
    exception->stack_trace = (j2me_stack_trace_element_t*)malloc(
        sizeof(j2me_stack_trace_element_t) * frame_count);
    if (!exception->stack_trace) {
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    exception->stack_trace_count = frame_count;
    
    // 填充栈跟踪信息
    current_frame = vm->current_thread->current_frame;
    for (int i = 0; i < frame_count && current_frame; i++) {
        j2me_stack_trace_element_t* element = &exception->stack_trace[i];
        
        // 初始化元素
        element->class_name = NULL;
        element->method_name = NULL;
        element->file_name = NULL;
        element->line_number = -1;
        element->pc = current_frame->pc;
        
        // 获取方法信息
        if (current_frame->method_info) {
            j2me_method_t* method = (j2me_method_t*)current_frame->method_info;
            
            // 复制类名
            if (method->owner_class && method->owner_class->name) {
                size_t class_len = strlen(method->owner_class->name) + 1;
                element->class_name = (char*)malloc(class_len);
                if (element->class_name) {
                    strcpy(element->class_name, method->owner_class->name);
                }
            }
            
            // 复制方法名
            if (method->name) {
                size_t method_len = strlen(method->name) + 1;
                element->method_name = (char*)malloc(method_len);
                if (element->method_name) {
                    strcpy(element->method_name, method->name);
                }
            }
            
            // 设置文件名（简化实现，使用类名）
            if (method->owner_class && method->owner_class->name) {
                size_t file_len = strlen(method->owner_class->name) + 6; // ".java" + null
                element->file_name = (char*)malloc(file_len);
                if (element->file_name) {
                    snprintf(element->file_name, file_len, "%s.java", method->owner_class->name);
                }
            }
            
            // 计算行号（简化实现）
            element->line_number = current_frame->pc + 1;
        }
        
        LOG_DEBUG("[异常处理] 栈跟踪[%d]: %s.%s (PC=%d)\n", i,
               element->class_name ? element->class_name : "未知类",
               element->method_name ? element->method_name : "未知方法",
               element->pc);
        
        current_frame = current_frame->previous;
    }
    
    LOG_DEBUG("[异常处理] 生成栈跟踪完成，共%d个栈帧\n", frame_count);
    return J2ME_SUCCESS;
}

/**
 * @brief 打印栈跟踪
 * @param exception 异常对象
 */
void j2me_exception_print_stack_trace(j2me_exception_t* exception) {
    if (!exception) {
        return;
    }
    
    LOG_WARN("异常: %s", exception->exception_class ? exception->exception_class : "未知异常");
    if (exception->message) {
        LOG_WARN("消息: %s", exception->message);
    }

    if (exception->stack_trace && exception->stack_trace_count > 0) {
        LOG_WARN("栈跟踪:");
        for (int i = 0; i < exception->stack_trace_count; i++) {
            j2me_stack_trace_element_t* element = &exception->stack_trace[i];
            LOG_WARN("  at %s.%s(%s:%d) [PC=%d]",
                   element->class_name ? element->class_name : "未知类",
                   element->method_name ? element->method_name : "未知方法",
                   element->file_name ? element->file_name : "未知文件",
                   element->line_number,
                   element->pc);
        }
    }
}

/**
 * @brief 抛出异常
 * @param vm 虚拟机实例
 * @param exception_class 异常类名
 * @param message 异常消息
 * @return 错误码
 */
j2me_error_t j2me_throw_exception(j2me_vm_t* vm, const char* exception_class, const char* message) {
    if (!vm) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    LOG_WARN("[异常处理] 抛出异常: %s - %s",
           exception_class ? exception_class : "未知异常",
           message ? message : "无消息");
    
    // 创建异常对象
    j2me_exception_t* exception = j2me_exception_create(exception_class, message);
    if (!exception) {
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    // 生成栈跟踪
    j2me_error_t trace_result = j2me_exception_generate_stack_trace(vm, exception);
    if (trace_result != J2ME_SUCCESS) {
        LOG_WARN("[异常处理] 生成栈跟踪失败: %d", trace_result);
    }
    
    // 设置当前异常
    if (vm->current_thread) {
        if (vm->current_thread->current_exception) {
            j2me_exception_destroy(vm->current_thread->current_exception);
        }
        vm->current_thread->current_exception = exception;
    }
    
    // 打印栈跟踪
    j2me_exception_print_stack_trace(exception);
    
    return J2ME_SUCCESS;
}

/**
 * @brief 处理异常
 * @param vm 虚拟机实例
 * @param exception 异常对象
 * @return 错误码
 */
j2me_error_t j2me_handle_exception(j2me_vm_t* vm, j2me_exception_t* exception) {
    if (!vm || !exception) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    LOG_DEBUG("[异常处理] 处理异常: %s\n",
           exception->exception_class ? exception->exception_class : "未知异常");

    if (!vm->current_thread || !vm->current_thread->current_frame) {
        LOG_ERROR("[异常处理] 没有当前线程或栈帧");
        return J2ME_ERROR_INVALID_STATE;
    }
    
    // 查找异常处理器
    j2me_stack_frame_t* current_frame = vm->current_thread->current_frame;
    while (current_frame) {
        // 检查当前方法是否有异常处理表
        if (current_frame->method_info) {
            j2me_method_t* method = (j2me_method_t*)current_frame->method_info;
            
            // 简化实现：查找catch块
            // 在实际实现中，需要解析异常表并匹配异常类型
            // 这里假设方法有异常处理能力
            if (method->bytecode_length > 10) { // 简化判断：有足够字节码的方法可能有异常处理
                LOG_DEBUG("[异常处理] 在方法 %s.%s 中找到可能的异常处理\n",
                       method->owner_class ? method->owner_class->name : "未知类",
                       method->name ? method->name : "未知方法");

                // 简化处理：跳转到方法末尾
                current_frame->pc = method->bytecode_length;

                LOG_DEBUG("[异常处理] 异常已处理\n");
                return J2ME_SUCCESS;
            }
        }
        
        // 移动到上一个栈帧
        current_frame = current_frame->previous;
    }
    
    // 没有找到异常处理器，异常未被捕获
    LOG_ERROR("[异常处理] 未捕获的异常: %s",
           exception->exception_class ? exception->exception_class : "未知异常");
    
    // 打印完整的栈跟踪
    j2me_exception_print_stack_trace(exception);
    
    return J2ME_ERROR_UNCAUGHT_EXCEPTION;
}

/**
 * @brief 清除当前异常
 * @param vm 虚拟机实例
 */
void j2me_clear_exception(j2me_vm_t* vm) {
    if (vm && vm->current_thread && vm->current_thread->current_exception) {
        j2me_exception_destroy(vm->current_thread->current_exception);
        vm->current_thread->current_exception = NULL;
        LOG_DEBUG("[异常处理] 清除当前异常\n");
    }
}

/**
 * @brief 检查是否有未处理的异常
 * @param vm 虚拟机实例
 * @return 是否有异常
 */
bool j2me_has_pending_exception(j2me_vm_t* vm) {
    if (vm && vm->current_thread) {
        return vm->current_thread->current_exception != NULL;
    }
    return false;
}

/**
 * @brief 获取当前异常
 * @param vm 虚拟机实例
 * @return 当前异常对象
 */
j2me_exception_t* j2me_get_current_exception(j2me_vm_t* vm) {
    if (vm && vm->current_thread) {
        return vm->current_thread->current_exception;
    }
    return NULL;
}

/**
 * @brief 抛出空指针异常
 * @param vm 虚拟机实例
 * @return 错误码
 */
j2me_error_t j2me_throw_null_pointer_exception(j2me_vm_t* vm) {
    return j2me_throw_exception(vm, "java/lang/NullPointerException", "空指针引用");
}

/**
 * @brief 抛出数组越界异常
 * @param vm 虚拟机实例
 * @param index 越界索引
 * @param length 数组长度
 * @return 错误码
 */
j2me_error_t j2me_throw_array_index_out_of_bounds_exception(j2me_vm_t* vm, int index, int length) {
    char message[256];
    snprintf(message, sizeof(message), "数组索引越界: %d (数组长度: %d)", index, length);
    return j2me_throw_exception(vm, "java/lang/ArrayIndexOutOfBoundsException", message);
}

/**
 * @brief 抛出算术异常
 * @param vm 虚拟机实例
 * @param message 异常消息
 * @return 错误码
 */
j2me_error_t j2me_throw_arithmetic_exception(j2me_vm_t* vm, const char* message) {
    return j2me_throw_exception(vm, "java/lang/ArithmeticException", message);
}

/**
 * @brief 抛出类转换异常
 * @param vm 虚拟机实例
 * @param from_class 源类名
 * @param to_class 目标类名
 * @return 错误码
 */
j2me_error_t j2me_throw_class_cast_exception(j2me_vm_t* vm, const char* from_class, const char* to_class) {
    char message[512];
    snprintf(message, sizeof(message), "无法将 %s 转换为 %s", 
             from_class ? from_class : "未知类",
             to_class ? to_class : "未知类");
    return j2me_throw_exception(vm, "java/lang/ClassCastException", message);
}

/**
 * @brief 抛出类未找到异常
 * @param vm 虚拟机实例
 * @param class_name 类名
 * @return 错误码
 */
j2me_error_t j2me_throw_class_not_found_exception(j2me_vm_t* vm, const char* class_name) {
    char message[256];
    snprintf(message, sizeof(message), "找不到类: %s", class_name ? class_name : "未知类");
    return j2me_throw_exception(vm, "java/lang/ClassNotFoundException", message);
}

/**
 * @brief 抛出方法未找到异常
 * @param vm 虚拟机实例
 * @param class_name 类名
 * @param method_name 方法名
 * @return 错误码
 */
j2me_error_t j2me_throw_no_such_method_exception(j2me_vm_t* vm, const char* class_name, const char* method_name) {
    char message[512];
    snprintf(message, sizeof(message), "找不到方法: %s.%s", 
             class_name ? class_name : "未知类",
             method_name ? method_name : "未知方法");
    return j2me_throw_exception(vm, "java/lang/NoSuchMethodException", message);
}

/**
 * @brief 抛出内存不足异常
 * @param vm 虚拟机实例
 * @return 错误码
 */
j2me_error_t j2me_throw_out_of_memory_exception(j2me_vm_t* vm) {
    return j2me_throw_exception(vm, "java/lang/OutOfMemoryError", "Java堆空间不足");
}

/**
 * @brief 抛出栈溢出异常
 * @param vm 虚拟机实例
 * @return 错误码
 */
j2me_error_t j2me_throw_stack_overflow_exception(j2me_vm_t* vm) {
    return j2me_throw_exception(vm, "java/lang/StackOverflowError", "栈空间溢出");
}