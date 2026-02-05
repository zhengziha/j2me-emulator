#ifndef J2ME_TYPES_H
#define J2ME_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @file j2me_types.h
 * @brief J2ME基础类型定义
 * 
 * 定义J2ME虚拟机中使用的基础数据类型和常量
 */

// 基础类型定义
typedef int8_t   j2me_byte;
typedef int16_t  j2me_short;
typedef int32_t  j2me_int;
typedef int64_t  j2me_long;
typedef float    j2me_float;
typedef double   j2me_double;
typedef bool     j2me_boolean;
typedef uint16_t j2me_char;

// 引用类型
typedef void*    j2me_reference;
typedef uint32_t j2me_address;

// 虚拟机状态
typedef enum {
    J2ME_VM_UNINITIALIZED = 0,
    J2ME_VM_INITIALIZING,
    J2ME_VM_RUNNING,
    J2ME_VM_SUSPENDED,
    J2ME_VM_TERMINATED,
    J2ME_VM_ERROR
} j2me_vm_state_t;

// 错误码
typedef enum {
    J2ME_SUCCESS = 0,
    J2ME_ERROR_OUT_OF_MEMORY,
    J2ME_ERROR_INVALID_PARAMETER,
    J2ME_ERROR_CLASS_NOT_FOUND,
    J2ME_ERROR_METHOD_NOT_FOUND,
    J2ME_ERROR_STACK_OVERFLOW,
    J2ME_ERROR_ILLEGAL_ACCESS,
    J2ME_ERROR_RUNTIME_EXCEPTION,
    J2ME_ERROR_IO_EXCEPTION,
    J2ME_ERROR_NOT_IMPLEMENTED,
    J2ME_ERROR_NETWORK_EXCEPTION,
    J2ME_ERROR_SECURITY_EXCEPTION,
    J2ME_ERROR_INITIALIZATION_FAILED
} j2me_error_t;

// 常量定义
#define J2ME_MAX_STACK_SIZE     1024
#define J2ME_MAX_LOCALS         256
#define J2ME_MAX_CLASS_NAME     256
#define J2ME_MAX_METHOD_NAME    128

// 字节码指令类型
typedef uint8_t j2me_opcode_t;

// 栈帧结构前向声明
typedef struct j2me_stack_frame j2me_stack_frame_t;
typedef struct j2me_thread j2me_thread_t;
typedef struct j2me_vm j2me_vm_t;

#endif // J2ME_TYPES_H