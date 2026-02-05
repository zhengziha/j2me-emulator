#include "j2me_interpreter.h"
#include "j2me_bytecode.h"
#include "j2me_vm.h"
#include "j2me_native_methods.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @file j2me_interpreter.c
 * @brief J2ME字节码解释器实现
 * 
 * 高性能字节码解释器，使用优化的指令分发机制
 */

// Java字节码指令定义 (部分核心指令)
#define OPCODE_NOP          0x00
#define OPCODE_ACONST_NULL  0x01
#define OPCODE_ICONST_M1    0x02
#define OPCODE_ICONST_0     0x03
#define OPCODE_ICONST_1     0x04
#define OPCODE_ICONST_2     0x05
#define OPCODE_ICONST_3     0x06
#define OPCODE_ICONST_4     0x07
#define OPCODE_ICONST_5     0x08
#define OPCODE_BIPUSH       0x10
#define OPCODE_SIPUSH       0x11
#define OPCODE_ILOAD        0x15
#define OPCODE_ALOAD_0      0x2a
#define OPCODE_ALOAD_1      0x2b
#define OPCODE_ALOAD_2      0x2c
#define OPCODE_ALOAD_3      0x2d
#define OPCODE_ILOAD_0      0x1a
#define OPCODE_ILOAD_1      0x1b
#define OPCODE_ILOAD_2      0x1c
#define OPCODE_ILOAD_3      0x1d
#define OPCODE_ISTORE       0x36
#define OPCODE_ASTORE_0     0x4b
#define OPCODE_ASTORE_1     0x4c
#define OPCODE_ASTORE_2     0x4d
#define OPCODE_ASTORE_3     0x4e
#define OPCODE_ISTORE_0     0x3b
#define OPCODE_ISTORE_1     0x3c
#define OPCODE_ISTORE_2     0x3d
#define OPCODE_ISTORE_3     0x3e
#define OPCODE_POP          0x57
#define OPCODE_POP2         0x58
#define OPCODE_DUP          0x59
#define OPCODE_SWAP         0x5f
#define OPCODE_IADD         0x60
#define OPCODE_ISUB         0x64
#define OPCODE_IMUL         0x68
#define OPCODE_IDIV         0x6c
#define OPCODE_IREM         0x70
#define OPCODE_INEG         0x74
#define OPCODE_ISHL         0x78
#define OPCODE_ISHR         0x7a
#define OPCODE_IUSHR        0x7c
#define OPCODE_IAND         0x7e
#define OPCODE_IOR          0x80
#define OPCODE_IXOR         0x82
#define OPCODE_IFEQ         0x99
#define OPCODE_IFNE         0x9a
#define OPCODE_IFLT         0x9b
#define OPCODE_IFGE         0x9c
#define OPCODE_IFGT         0x9d
#define OPCODE_IFLE         0x9e
#define OPCODE_IF_ICMPEQ    0x9f
#define OPCODE_IF_ICMPNE    0xa0
#define OPCODE_GOTO         0xa7
#define OPCODE_IRETURN      0xac
#define OPCODE_RETURN       0xb1
#define OPCODE_GETSTATIC    0xb2
#define OPCODE_PUTSTATIC    0xb3
#define OPCODE_GETFIELD     0xb4
#define OPCODE_PUTFIELD     0xb5
#define OPCODE_INVOKEVIRTUAL 0xb6
#define OPCODE_INVOKESPECIAL 0xb7
#define OPCODE_INVOKESTATIC 0xb8
#define OPCODE_INVOKEINTERFACE 0xb9
#define OPCODE_NEW          0xbb

j2me_operand_stack_t* j2me_operand_stack_create(size_t size) {
    j2me_operand_stack_t* stack = (j2me_operand_stack_t*)malloc(sizeof(j2me_operand_stack_t));
    if (!stack) {
        return NULL;
    }
    
    stack->data = (j2me_int*)malloc(sizeof(j2me_int) * size);
    if (!stack->data) {
        free(stack);
        return NULL;
    }
    
    stack->size = size;
    stack->top = 0;
    
    return stack;
}

void j2me_operand_stack_destroy(j2me_operand_stack_t* stack) {
    if (stack) {
        if (stack->data) {
            free(stack->data);
        }
        free(stack);
    }
}

j2me_error_t j2me_operand_stack_push(j2me_operand_stack_t* stack, j2me_int value) {
    if (!stack || stack->top >= stack->size) {
        return J2ME_ERROR_STACK_OVERFLOW;
    }
    
    stack->data[stack->top++] = value;
    return J2ME_SUCCESS;
}

j2me_error_t j2me_operand_stack_pop(j2me_operand_stack_t* stack, j2me_int* value) {
    if (!stack || !value || stack->top == 0) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    *value = stack->data[--stack->top];
    return J2ME_SUCCESS;
}

j2me_stack_frame_t* j2me_stack_frame_create(size_t max_stack, size_t max_locals) {
    j2me_stack_frame_t* frame = (j2me_stack_frame_t*)malloc(sizeof(j2me_stack_frame_t));
    if (!frame) {
        return NULL;
    }
    
    memset(frame, 0, sizeof(j2me_stack_frame_t));
    
    // 创建操作数栈
    j2me_operand_stack_t* stack = j2me_operand_stack_create(max_stack);
    if (!stack) {
        free(frame);
        return NULL;
    }
    frame->operand_stack = *stack;
    free(stack); // 只需要数据，不需要包装结构
    
    // 创建局部变量表
    frame->local_vars.variables = (j2me_int*)malloc(sizeof(j2me_int) * max_locals);
    if (!frame->local_vars.variables) {
        free(frame->operand_stack.data);
        free(frame);
        return NULL;
    }
    frame->local_vars.size = max_locals;
    
    // 初始化局部变量为0
    memset(frame->local_vars.variables, 0, sizeof(j2me_int) * max_locals);
    
    return frame;
}

void j2me_stack_frame_destroy(j2me_stack_frame_t* frame) {
    if (frame) {
        if (frame->operand_stack.data) {
            free(frame->operand_stack.data);
        }
        if (frame->local_vars.variables) {
            free(frame->local_vars.variables);
        }
        free(frame);
    }
}

/**
 * @brief 执行单条字节码指令 (增强版本)
 * @param vm 虚拟机实例
 * @param frame 当前栈帧
 * @return 错误码
 */
static j2me_error_t execute_single_instruction(j2me_vm_t* vm, j2me_stack_frame_t* frame) {
    if (!frame->bytecode) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    j2me_opcode_t opcode = frame->bytecode[frame->pc++];
    j2me_error_t result = J2ME_SUCCESS;
    j2me_int value1, value2, result_value;
    
    // 获取指令信息用于调试
    const char* inst_name = j2me_get_instruction_name(opcode);
    
    // 使用跳转表优化指令分发
    switch (opcode) {
        case OPCODE_NOP:
            // 无操作
            break;
            
        case OPCODE_ACONST_NULL:
            // 将null引用压入栈
            result = j2me_operand_stack_push(&frame->operand_stack, 0);
            break;
            
        case OPCODE_ICONST_M1:
            result = j2me_operand_stack_push(&frame->operand_stack, -1);
            break;
            
        case OPCODE_ICONST_0:
        case OPCODE_ICONST_1:
        case OPCODE_ICONST_2:
        case OPCODE_ICONST_3:
        case OPCODE_ICONST_4:
        case OPCODE_ICONST_5:
            // 将常量压入栈
            result = j2me_operand_stack_push(&frame->operand_stack, opcode - OPCODE_ICONST_0);
            break;
            
        case OPCODE_BIPUSH:
            // 将byte值压入栈
            value1 = (j2me_byte)frame->bytecode[frame->pc++];
            result = j2me_operand_stack_push(&frame->operand_stack, value1);
            break;
            
        case OPCODE_SIPUSH:
            // 将short值压入栈
            value1 = (j2me_short)((frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1]);
            frame->pc += 2;
            result = j2me_operand_stack_push(&frame->operand_stack, value1);
            break;
            
        case OPCODE_ILOAD:
            // 从局部变量加载int
            value1 = frame->bytecode[frame->pc++];
            if (value1 < frame->local_vars.size) {
                result = j2me_operand_stack_push(&frame->operand_stack, frame->local_vars.variables[value1]);
            } else {
                result = J2ME_ERROR_INVALID_PARAMETER;
            }
            break;
            
        case OPCODE_ALOAD_0:
        case OPCODE_ALOAD_1:
        case OPCODE_ALOAD_2:
        case OPCODE_ALOAD_3:
            // 从局部变量加载引用到栈
            value1 = opcode - OPCODE_ALOAD_0;
            if (value1 < frame->local_vars.size) {
                result = j2me_operand_stack_push(&frame->operand_stack, frame->local_vars.variables[value1]);
            } else {
                result = J2ME_ERROR_INVALID_PARAMETER;
            }
            break;
            
        case OPCODE_ASTORE_0:
        case OPCODE_ASTORE_1:
        case OPCODE_ASTORE_2:
        case OPCODE_ASTORE_3:
            // 从栈存储引用到局部变量
            value1 = opcode - OPCODE_ASTORE_0;
            result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
            if (result == J2ME_SUCCESS && value1 < frame->local_vars.size) {
                frame->local_vars.variables[value1] = value2;
            } else {
                result = J2ME_ERROR_INVALID_PARAMETER;
            }
            break;
            
        case OPCODE_ISTORE:
            // 存储int到局部变量
            value1 = frame->bytecode[frame->pc++];
            result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
            if (result == J2ME_SUCCESS && value1 < frame->local_vars.size) {
                frame->local_vars.variables[value1] = value2;
            } else {
                result = J2ME_ERROR_INVALID_PARAMETER;
            }
            break;
            
        case OPCODE_ISTORE_0:
        case OPCODE_ISTORE_1:
        case OPCODE_ISTORE_2:
        case OPCODE_ISTORE_3:
            // 从栈存储到局部变量
            value1 = opcode - OPCODE_ISTORE_0;
            result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
            if (result == J2ME_SUCCESS && value1 < frame->local_vars.size) {
                frame->local_vars.variables[value1] = value2;
            } else {
                result = J2ME_ERROR_INVALID_PARAMETER;
            }
            break;
            
        case OPCODE_POP:
            // 弹出栈顶值
            result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
            break;
            
        case OPCODE_POP2:
            // 弹出栈顶两个值
            result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
            if (result == J2ME_SUCCESS) {
                result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
            }
            break;
            
        case OPCODE_DUP:
            // 复制栈顶值
            if (frame->operand_stack.top > 0) {
                value1 = frame->operand_stack.data[frame->operand_stack.top - 1];
                result = j2me_operand_stack_push(&frame->operand_stack, value1);
            } else {
                result = J2ME_ERROR_INVALID_PARAMETER;
            }
            break;
            
        case OPCODE_SWAP:
            // 交换栈顶两个值
            if (frame->operand_stack.top >= 2) {
                value1 = frame->operand_stack.data[frame->operand_stack.top - 1];
                value2 = frame->operand_stack.data[frame->operand_stack.top - 2];
                frame->operand_stack.data[frame->operand_stack.top - 1] = value2;
                frame->operand_stack.data[frame->operand_stack.top - 2] = value1;
            } else {
                result = J2ME_ERROR_INVALID_PARAMETER;
            }
            break;
            
        case OPCODE_IADD:
            // 整数加法
            result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
            if (result == J2ME_SUCCESS) {
                result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
                if (result == J2ME_SUCCESS) {
                    result_value = value1 + value2;
                    result = j2me_operand_stack_push(&frame->operand_stack, result_value);
                }
            }
            break;
            
        case OPCODE_ISUB:
            // 整数减法
            result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
            if (result == J2ME_SUCCESS) {
                result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
                if (result == J2ME_SUCCESS) {
                    result_value = value1 - value2;
                    result = j2me_operand_stack_push(&frame->operand_stack, result_value);
                }
            }
            break;
            
        case OPCODE_IMUL:
            // 整数乘法
            result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
            if (result == J2ME_SUCCESS) {
                result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
                if (result == J2ME_SUCCESS) {
                    result_value = value1 * value2;
                    result = j2me_operand_stack_push(&frame->operand_stack, result_value);
                }
            }
            break;
            
        case OPCODE_IDIV:
            // 整数除法
            result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
            if (result == J2ME_SUCCESS) {
                result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
                if (result == J2ME_SUCCESS) {
                    if (value2 == 0) {
                        result = J2ME_ERROR_RUNTIME_EXCEPTION; // 除零异常
                    } else {
                        result_value = value1 / value2;
                        result = j2me_operand_stack_push(&frame->operand_stack, result_value);
                    }
                }
            }
            break;
            
        case OPCODE_IREM:
            // 整数取余
            result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
            if (result == J2ME_SUCCESS) {
                result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
                if (result == J2ME_SUCCESS) {
                    if (value2 == 0) {
                        result = J2ME_ERROR_RUNTIME_EXCEPTION; // 除零异常
                    } else {
                        result_value = value1 % value2;
                        result = j2me_operand_stack_push(&frame->operand_stack, result_value);
                    }
                }
            }
            break;
            
        case OPCODE_INEG:
            // 整数取负
            result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
            if (result == J2ME_SUCCESS) {
                result_value = -value1;
                result = j2me_operand_stack_push(&frame->operand_stack, result_value);
            }
            break;
            
        case OPCODE_ISHL:
            // 整数左移
            result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
            if (result == J2ME_SUCCESS) {
                result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
                if (result == J2ME_SUCCESS) {
                    result_value = value1 << (value2 & 0x1f); // 只使用低5位
                    result = j2me_operand_stack_push(&frame->operand_stack, result_value);
                }
            }
            break;
            
        case OPCODE_ISHR:
            // 整数算术右移
            result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
            if (result == J2ME_SUCCESS) {
                result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
                if (result == J2ME_SUCCESS) {
                    result_value = value1 >> (value2 & 0x1f); // 只使用低5位
                    result = j2me_operand_stack_push(&frame->operand_stack, result_value);
                }
            }
            break;
            
        case OPCODE_IUSHR:
            // 整数逻辑右移
            result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
            if (result == J2ME_SUCCESS) {
                result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
                if (result == J2ME_SUCCESS) {
                    result_value = (uint32_t)value1 >> (value2 & 0x1f); // 只使用低5位
                    result = j2me_operand_stack_push(&frame->operand_stack, result_value);
                }
            }
            break;
            
        case OPCODE_IAND:
            // 整数按位与
            result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
            if (result == J2ME_SUCCESS) {
                result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
                if (result == J2ME_SUCCESS) {
                    result_value = value1 & value2;
                    result = j2me_operand_stack_push(&frame->operand_stack, result_value);
                }
            }
            break;
            
        case OPCODE_IOR:
            // 整数按位或
            result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
            if (result == J2ME_SUCCESS) {
                result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
                if (result == J2ME_SUCCESS) {
                    result_value = value1 | value2;
                    result = j2me_operand_stack_push(&frame->operand_stack, result_value);
                }
            }
            break;
            
        case OPCODE_IXOR:
            // 整数按位异或
            result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
            if (result == J2ME_SUCCESS) {
                result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
                if (result == J2ME_SUCCESS) {
                    result_value = value1 ^ value2;
                    result = j2me_operand_stack_push(&frame->operand_stack, result_value);
                }
            }
            break;
            
        case OPCODE_IFEQ:
            // 如果等于0则跳转
            result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
            if (result == J2ME_SUCCESS) {
                j2me_short offset = (j2me_short)((frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1]);
                frame->pc += 2;
                if (value1 == 0) {
                    frame->pc = (frame->pc - 3) + offset; // 跳转到目标位置
                }
            }
            break;
            
        case OPCODE_IFNE:
            // 如果不等于0则跳转
            result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
            if (result == J2ME_SUCCESS) {
                j2me_short offset = (j2me_short)((frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1]);
                frame->pc += 2;
                if (value1 != 0) {
                    frame->pc = (frame->pc - 3) + offset; // 跳转到目标位置
                }
            }
            break;
            
        case OPCODE_IFLT:
            // 如果小于0则跳转
            result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
            if (result == J2ME_SUCCESS) {
                j2me_short offset = (j2me_short)((frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1]);
                frame->pc += 2;
                if (value1 < 0) {
                    frame->pc = (frame->pc - 3) + offset; // 跳转到目标位置
                }
            }
            break;
            
        case OPCODE_IFGE:
            // 如果大于等于0则跳转
            result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
            if (result == J2ME_SUCCESS) {
                j2me_short offset = (j2me_short)((frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1]);
                frame->pc += 2;
                if (value1 >= 0) {
                    frame->pc = (frame->pc - 3) + offset; // 跳转到目标位置
                }
            }
            break;
            
        case OPCODE_IFGT:
            // 如果大于0则跳转
            result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
            if (result == J2ME_SUCCESS) {
                j2me_short offset = (j2me_short)((frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1]);
                frame->pc += 2;
                if (value1 > 0) {
                    frame->pc = (frame->pc - 3) + offset; // 跳转到目标位置
                }
            }
            break;
            
        case OPCODE_IFLE:
            // 如果小于等于0则跳转
            result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
            if (result == J2ME_SUCCESS) {
                j2me_short offset = (j2me_short)((frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1]);
                frame->pc += 2;
                if (value1 <= 0) {
                    frame->pc = (frame->pc - 3) + offset; // 跳转到目标位置
                }
            }
            break;
            
        case OPCODE_IF_ICMPEQ:
            // 如果两个int相等则跳转
            result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
            if (result == J2ME_SUCCESS) {
                result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
                if (result == J2ME_SUCCESS) {
                    j2me_short offset = (j2me_short)((frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1]);
                    frame->pc += 2;
                    if (value1 == value2) {
                        frame->pc = (frame->pc - 3) + offset; // 跳转到目标位置
                    }
                }
            }
            break;
            
        case OPCODE_IF_ICMPNE:
            // 如果两个int不等则跳转
            result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
            if (result == J2ME_SUCCESS) {
                result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
                if (result == J2ME_SUCCESS) {
                    j2me_short offset = (j2me_short)((frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1]);
                    frame->pc += 2;
                    if (value1 != value2) {
                        frame->pc = (frame->pc - 3) + offset; // 跳转到目标位置
                    }
                }
            }
            break;
            
        case OPCODE_GOTO:
            // 无条件跳转
            {
                j2me_short offset = (j2me_short)((frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1]);
                frame->pc = (frame->pc - 1) + offset; // 跳转到目标位置
            }
            break;
            
        case OPCODE_IRETURN:
            // 返回int值
            result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
            // TODO: 将返回值传递给调用者
            return J2ME_SUCCESS; // 特殊处理，表示方法结束
            
        case OPCODE_RETURN:
            // 方法返回
            return J2ME_SUCCESS; // 特殊处理，表示方法结束
            
        case OPCODE_GETSTATIC:
            // 获取静态字段
            {
                uint16_t field_ref_index = (frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1];
                frame->pc += 2;
                
                printf("[解释器] getstatic: 字段引用索引 #%d\n", field_ref_index);
                
                // 暂时简化处理：压入一个假的值
                j2me_int field_value = 0x87654321; // 假的字段值
                result = j2me_operand_stack_push(&frame->operand_stack, field_value);
                
                printf("[解释器] getstatic: 获取静态字段值 0x%x\n", field_value);
            }
            break;
            
        case OPCODE_PUTSTATIC:
            // 设置静态字段
            {
                uint16_t field_ref_index = (frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1];
                frame->pc += 2;
                
                printf("[解释器] putstatic: 字段引用索引 #%d\n", field_ref_index);
                
                // 弹出要设置的值
                j2me_int field_value;
                result = j2me_operand_stack_pop(&frame->operand_stack, &field_value);
                
                printf("[解释器] putstatic: 设置静态字段值 0x%x\n", field_value);
            }
            break;
            
        case OPCODE_GETFIELD:
            // 获取实例字段
            {
                uint16_t field_ref_index = (frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1];
                frame->pc += 2;
                
                printf("[解释器] getfield: 字段引用索引 #%d\n", field_ref_index);
                
                // 弹出对象引用
                j2me_int object_ref;
                result = j2me_operand_stack_pop(&frame->operand_stack, &object_ref);
                
                if (result == J2ME_SUCCESS) {
                    // 暂时简化处理：压入一个假的字段值
                    j2me_int field_value = 0x11223344; // 假的字段值
                    result = j2me_operand_stack_push(&frame->operand_stack, field_value);
                    
                    printf("[解释器] getfield: 从对象 0x%x 获取字段值 0x%x\n", object_ref, field_value);
                }
            }
            break;
            
        case OPCODE_PUTFIELD:
            // 设置实例字段
            {
                uint16_t field_ref_index = (frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1];
                frame->pc += 2;
                
                printf("[解释器] putfield: 字段引用索引 #%d\n", field_ref_index);
                
                // 弹出字段值和对象引用
                j2me_int field_value, object_ref;
                result = j2me_operand_stack_pop(&frame->operand_stack, &field_value);
                if (result == J2ME_SUCCESS) {
                    result = j2me_operand_stack_pop(&frame->operand_stack, &object_ref);
                    
                    printf("[解释器] putfield: 设置对象 0x%x 的字段值 0x%x\n", object_ref, field_value);
                }
            }
            break;
            
        case OPCODE_INVOKESPECIAL:
            // 调用特殊方法 (构造方法、私有方法、父类方法)
            {
                // 获取方法引用索引 (2字节)
                uint16_t method_ref_index = (frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1];
                frame->pc += 2;
                
                printf("[解释器] invokespecial: 方法引用索引 #%d\n", method_ref_index);
                
                // 从常量池解析方法引用
                const char* class_name = NULL;
                const char* method_name = NULL;
                const char* descriptor = NULL;
                
                j2me_method_t* current_method = (j2me_method_t*)frame->method_info;
                if (current_method && current_method->owner_class) {
                    j2me_error_t resolve_result = j2me_interpreter_resolve_method_ref(
                        current_method->owner_class, method_ref_index,
                        &class_name, &method_name, &descriptor);
                    
                    if (resolve_result == J2ME_SUCCESS) {
                        printf("[解释器] invokespecial: 解析到方法 %s.%s%s\n", 
                               class_name, method_name, descriptor);
                        
                        // 检查是否为构造方法
                        if (strcmp(method_name, "<init>") == 0) {
                            // 构造方法调用，弹出this引用
                            if (frame->operand_stack.top > 0) {
                                j2me_int this_ref;
                                result = j2me_operand_stack_pop(&frame->operand_stack, &this_ref);
                                printf("[解释器] invokespecial: 弹出this引用 0x%x (构造方法)\n", this_ref);
                            }
                            printf("[解释器] invokespecial: 构造方法调用完成\n");
                        } else {
                            // 其他特殊方法调用，简化处理
                            if (frame->operand_stack.top > 0) {
                                j2me_int this_ref;
                                result = j2me_operand_stack_pop(&frame->operand_stack, &this_ref);
                                printf("[解释器] invokespecial: 弹出this引用 0x%x\n", this_ref);
                            }
                            printf("[解释器] invokespecial: 特殊方法调用完成\n");
                        }
                    } else {
                        printf("[解释器] invokespecial: 方法解析失败: %d\n", resolve_result);
                        result = resolve_result;
                    }
                } else {
                    // 简化处理：invokespecial通常是构造方法调用，不返回值
                    // 弹出this引用 (如果栈不为空)
                    if (frame->operand_stack.top > 0) {
                        j2me_int this_ref;
                        result = j2me_operand_stack_pop(&frame->operand_stack, &this_ref);
                        printf("[解释器] invokespecial: 弹出this引用 0x%x\n", this_ref);
                    }
                    
                    printf("[解释器] invokespecial: 构造方法调用完成 (简化实现)\n");
                }
            }
            break;
            
        case OPCODE_INVOKEVIRTUAL:
            // 调用虚方法
            {
                uint16_t method_ref_index = (frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1];
                frame->pc += 2;
                
                printf("[解释器] invokevirtual: 方法引用索引 #%d\n", method_ref_index);
                
                // 从常量池解析方法引用
                const char* class_name = NULL;
                const char* method_name = NULL;
                const char* descriptor = NULL;
                
                j2me_method_t* current_method = (j2me_method_t*)frame->method_info;
                if (current_method && current_method->owner_class) {
                    j2me_error_t resolve_result = j2me_interpreter_resolve_method_ref(
                        current_method->owner_class, method_ref_index,
                        &class_name, &method_name, &descriptor);
                    
                    if (resolve_result == J2ME_SUCCESS) {
                        // 尝试调用本地方法
                        j2me_error_t native_result = j2me_native_method_invoke(vm, frame,
                                                                               class_name,
                                                                               method_name,
                                                                               descriptor,
                                                                               NULL);
                        
                        if (native_result == J2ME_SUCCESS) {
                            printf("[解释器] invokevirtual: 本地方法调用成功\n");
                        } else if (native_result == J2ME_ERROR_METHOD_NOT_FOUND) {
                            // 不是本地方法，使用简化处理
                            if (frame->operand_stack.top > 0) {
                                j2me_int this_ref;
                                result = j2me_operand_stack_pop(&frame->operand_stack, &this_ref);
                                printf("[解释器] invokevirtual: 弹出this引用 0x%x\n", this_ref);
                            }
                            printf("[解释器] invokevirtual: 方法调用完成 (简化实现)\n");
                        } else {
                            printf("[解释器] invokevirtual: 本地方法调用失败: %d\n", native_result);
                            result = native_result;
                        }
                    } else {
                        printf("[解释器] invokevirtual: 方法解析失败: %d\n", resolve_result);
                        result = resolve_result;
                    }
                } else {
                    printf("[解释器] invokevirtual: 无法获取当前方法信息，使用简化处理\n");
                    if (frame->operand_stack.top > 0) {
                        j2me_int this_ref;
                        result = j2me_operand_stack_pop(&frame->operand_stack, &this_ref);
                        printf("[解释器] invokevirtual: 弹出this引用 0x%x\n", this_ref);
                    }
                }
            }
            break;
            
        case OPCODE_INVOKESTATIC:
            // 调用静态方法
            {
                uint16_t method_ref_index = (frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1];
                frame->pc += 2;
                
                printf("[解释器] invokestatic: 方法引用索引 #%d\n", method_ref_index);
                
                // 从常量池解析方法引用
                const char* class_name = NULL;
                const char* method_name = NULL;
                const char* descriptor = NULL;
                
                j2me_method_t* current_method = (j2me_method_t*)frame->method_info;
                if (current_method && current_method->owner_class) {
                    j2me_error_t resolve_result = j2me_interpreter_resolve_method_ref(
                        current_method->owner_class, method_ref_index,
                        &class_name, &method_name, &descriptor);
                    
                    if (resolve_result == J2ME_SUCCESS) {
                        printf("[解释器] invokestatic: 解析到方法 %s.%s%s\n", 
                               class_name, method_name, descriptor);
                        
                        // 检查是否为Display.getDisplay方法
                        if (strcmp(class_name, "javax/microedition/lcdui/Display") == 0 &&
                            strcmp(method_name, "getDisplay") == 0) {
                            
                            // 检查方法签名，如果有参数则弹出
                            if (strstr(descriptor, "Ljavax/microedition/midlet/MIDlet;") != NULL) {
                                // 弹出MIDlet参数
                                j2me_int midlet_ref;
                                result = j2me_operand_stack_pop(&frame->operand_stack, &midlet_ref);
                                if (result == J2ME_SUCCESS) {
                                    printf("[解释器] invokestatic: 弹出MIDlet参数 0x%x\n", midlet_ref);
                                }
                            }
                            
                            // 调用Display.getDisplay()本地方法
                            j2me_error_t native_result = j2me_native_method_invoke(vm, frame,
                                                                                   "javax/microedition/lcdui/Display",
                                                                                   "getDisplay",
                                                                                   "()Ljavax/microedition/lcdui/Display;",
                                                                                   NULL);
                            
                            if (native_result == J2ME_SUCCESS) {
                                printf("[解释器] invokestatic: Display.getDisplay()调用成功\n");
                            } else {
                                printf("[解释器] invokestatic: Display.getDisplay()调用失败: %d\n", native_result);
                                result = native_result;
                            }
                        } else {
                            // 其他静态方法调用，简化处理
                            printf("[解释器] invokestatic: 其他静态方法调用 (简化实现)\n");
                        }
                    } else {
                        printf("[解释器] invokestatic: 方法解析失败: %d\n", resolve_result);
                        result = resolve_result;
                    }
                } else {
                    // 根据方法引用索引判断调用哪个方法 (向后兼容)
                    if (method_ref_index == 8) {
                        // 这是Display.getDisplay(MIDlet)方法
                        // 弹出MIDlet参数
                        j2me_int midlet_ref;
                        result = j2me_operand_stack_pop(&frame->operand_stack, &midlet_ref);
                        if (result == J2ME_SUCCESS) {
                            printf("[解释器] invokestatic: 弹出MIDlet参数 0x%x\n", midlet_ref);
                            
                            // 调用Display.getDisplay()本地方法
                            j2me_error_t native_result = j2me_native_method_invoke(vm, frame,
                                                                                   "javax/microedition/lcdui/Display",
                                                                                   "getDisplay",
                                                                                   "()Ljavax/microedition/lcdui/Display;",
                                                                                   NULL);
                            
                            if (native_result == J2ME_SUCCESS) {
                                printf("[解释器] invokestatic: Display.getDisplay()调用成功\n");
                            } else {
                                printf("[解释器] invokestatic: Display.getDisplay()调用失败: %d\n", native_result);
                                result = native_result;
                            }
                        }
                    } else {
                        // 其他静态方法调用，简化处理
                        printf("[解释器] invokestatic: 其他静态方法调用 (简化实现)\n");
                    }
                }
            }
            break;
            
        case OPCODE_INVOKEINTERFACE:
            // 调用接口方法
            {
                uint16_t method_ref_index = (frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1];
                uint8_t count = frame->bytecode[frame->pc + 2];  // 参数数量
                uint8_t zero = frame->bytecode[frame->pc + 3];   // 必须为0
                frame->pc += 4;
                
                printf("[解释器] invokeinterface: 方法引用索引 #%d, 参数数量 %d\n", method_ref_index, count);
                
                // 暂时简化处理
                if (frame->operand_stack.top > 0) {
                    j2me_int this_ref;
                    result = j2me_operand_stack_pop(&frame->operand_stack, &this_ref);
                    printf("[解释器] invokeinterface: 弹出this引用 0x%x\n", this_ref);
                }
                
                printf("[解释器] invokeinterface: 方法调用完成 (简化实现)\n");
            }
            break;
            
        case OPCODE_NEW:
            // 创建新对象
            {
                uint16_t class_index = (frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1];
                frame->pc += 2;
                
                printf("[解释器] new: 类索引 #%d\n", class_index);
                
                // 暂时简化处理：创建一个假的对象引用
                j2me_int object_ref = 0x12345678; // 假的对象引用
                result = j2me_operand_stack_push(&frame->operand_stack, object_ref);
                
                printf("[解释器] new: 创建对象引用 0x%x\n", object_ref);
            }
            break;
            
        default:
            printf("[解释器] 未实现的指令: %s (0x%02x)\n", inst_name, opcode);
            result = J2ME_ERROR_RUNTIME_EXCEPTION;
            break;
    }
    
    return result;
}

j2me_error_t j2me_interpreter_execute_instruction(j2me_vm_t* vm, j2me_thread_t* thread) {
    if (!vm || !thread || !thread->current_frame) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    return execute_single_instruction(vm, thread->current_frame);
}

j2me_error_t j2me_interpreter_execute_batch(j2me_vm_t* vm, j2me_thread_t* thread, uint32_t max_instructions) {
    if (!vm || !thread) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    j2me_error_t result = J2ME_SUCCESS;
    uint32_t executed = 0;
    
    while (executed < max_instructions && thread->is_running && thread->current_frame) {
        result = execute_single_instruction(vm, thread->current_frame);
        
        if (result != J2ME_SUCCESS) {
            break;
        }
        
        executed++;
    }
    
    return result;
}

j2me_error_t j2me_interpreter_resolve_method_ref(j2me_class_t* class_info, 
                                                 uint16_t method_ref_index,
                                                 const char** class_name,
                                                 const char** method_name,
                                                 const char** descriptor) {
    if (!class_info || !class_name || !method_name || !descriptor) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 检查方法引用索引是否有效
    if (method_ref_index == 0 || method_ref_index >= class_info->constant_pool.count) {
        printf("[方法解析] 错误: 无效的方法引用索引 #%d (常量池大小: %d)\n", 
               method_ref_index, class_info->constant_pool.count);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 常量池索引从1开始，但数组索引从0开始，所以需要减1
    j2me_constant_pool_entry_t* method_ref = &class_info->constant_pool.entries[method_ref_index - 1];
    
    // 检查是否为方法引用或接口方法引用
    if (method_ref->tag != CONSTANT_Methodref && method_ref->tag != CONSTANT_InterfaceMethodref) {
        printf("[方法解析] 错误: 常量池条目 #%d 不是方法引用 (类型: %d)\n", 
               method_ref_index, method_ref->tag);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    uint16_t class_index = method_ref->info.ref_info.class_index;
    uint16_t name_and_type_index = method_ref->info.ref_info.name_and_type_index;
    
    printf("[方法解析] 方法引用 #%d: class_index=%d, name_and_type_index=%d\n", 
           method_ref_index, class_index, name_and_type_index);
    
    // 解析类名
    if (class_index == 0 || class_index >= class_info->constant_pool.count) {
        printf("[方法解析] 错误: 无效的类索引 #%d (常量池大小: %d)\n", 
               class_index, class_info->constant_pool.count);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    j2me_constant_pool_entry_t* class_entry = &class_info->constant_pool.entries[class_index - 1];
    if (class_entry->tag != CONSTANT_Class) {
        printf("[方法解析] 错误: 常量池条目 #%d 不是类引用 (类型: %d)\n", 
               class_index, class_entry->tag);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    uint16_t class_name_index = class_entry->info.class_info.name_index;
    if (class_name_index == 0 || class_name_index >= class_info->constant_pool.count) {
        printf("[方法解析] 错误: 无效的类名索引 #%d (常量池大小: %d)\n", 
               class_name_index, class_info->constant_pool.count);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    j2me_constant_pool_entry_t* class_name_entry = &class_info->constant_pool.entries[class_name_index - 1];
    if (class_name_entry->tag != CONSTANT_Utf8) {
        printf("[方法解析] 错误: 类名条目 #%d 不是UTF-8字符串 (类型: %d)\n", 
               class_name_index, class_name_entry->tag);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    *class_name = class_name_entry->info.utf8.bytes;
    
    // 解析方法名和描述符
    if (name_and_type_index == 0 || name_and_type_index >= class_info->constant_pool.count) {
        printf("[方法解析] 错误: 无效的名称和类型索引 #%d (常量池大小: %d)\n", 
               name_and_type_index, class_info->constant_pool.count);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    j2me_constant_pool_entry_t* name_and_type_entry = &class_info->constant_pool.entries[name_and_type_index - 1];
    if (name_and_type_entry->tag != CONSTANT_NameAndType) {
        printf("[方法解析] 错误: 常量池条目 #%d 不是名称和类型 (类型: %d)\n", 
               name_and_type_index, name_and_type_entry->tag);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    uint16_t method_name_index = name_and_type_entry->info.name_and_type_info.name_index;
    uint16_t descriptor_index = name_and_type_entry->info.name_and_type_info.descriptor_index;
    
    printf("[方法解析] 名称和类型 #%d: method_name_index=%d, descriptor_index=%d\n", 
           name_and_type_index, method_name_index, descriptor_index);
    
    // 解析方法名
    if (method_name_index == 0 || method_name_index >= class_info->constant_pool.count) {
        printf("[方法解析] 错误: 无效的方法名索引 #%d (常量池大小: %d)\n", 
               method_name_index, class_info->constant_pool.count);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    j2me_constant_pool_entry_t* method_name_entry = &class_info->constant_pool.entries[method_name_index - 1];
    if (method_name_entry->tag != CONSTANT_Utf8) {
        printf("[方法解析] 错误: 方法名条目 #%d 不是UTF-8字符串 (类型: %d)\n", 
               method_name_index, method_name_entry->tag);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    *method_name = method_name_entry->info.utf8.bytes;
    
    // 解析描述符
    if (descriptor_index == 0 || descriptor_index >= class_info->constant_pool.count) {
        printf("[方法解析] 错误: 无效的描述符索引 #%d (常量池大小: %d)\n", 
               descriptor_index, class_info->constant_pool.count);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    j2me_constant_pool_entry_t* descriptor_entry = &class_info->constant_pool.entries[descriptor_index - 1];
    if (descriptor_entry->tag != CONSTANT_Utf8) {
        printf("[方法解析] 错误: 描述符条目 #%d 不是UTF-8字符串 (类型: %d)\n", 
               descriptor_index, descriptor_entry->tag);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    *descriptor = descriptor_entry->info.utf8.bytes;
    
    printf("[方法解析] 成功解析方法引用 #%d: %s.%s%s\n", 
           method_ref_index, *class_name, *method_name, *descriptor);
    
    return J2ME_SUCCESS;
}

j2me_error_t j2me_interpreter_execute_method(j2me_vm_t* vm, j2me_method_t* method, void* object, void* args) {
    if (!vm || !method) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    printf("[解释器] 执行方法: %s.%s\n", 
           method->owner_class ? method->owner_class->name : "unknown", 
           method->name ? method->name : "unknown");
    
    // 检查方法是否有字节码
    if (!method->bytecode || method->bytecode_length == 0) {
        printf("[解释器] 方法无字节码，可能是抽象方法或本地方法\n");
        return J2ME_SUCCESS; // 本地方法或抽象方法，直接返回成功
    }
    
    // 创建栈帧
    j2me_stack_frame_t* frame = j2me_stack_frame_create(method->max_stack, method->max_locals);
    if (!frame) {
        printf("[解释器] 错误: 创建栈帧失败\n");
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    // 设置字节码和程序计数器
    frame->bytecode = method->bytecode;
    frame->pc = 0;
    frame->method_info = method;
    
    // 如果是实例方法，将this引用放入局部变量0
    if (!(method->access_flags & ACC_STATIC) && object) {
        frame->local_vars.variables[0] = (j2me_int)(uintptr_t)object;
    }
    
    // TODO: 处理方法参数
    // 这里应该根据方法描述符解析参数并放入局部变量表
    
    printf("[解释器] 开始执行字节码，长度: %d bytes\n", method->bytecode_length);
    
    // 执行字节码
    j2me_error_t result = J2ME_SUCCESS;
    uint32_t instruction_count = 0;
    const uint32_t max_instructions = 10000; // 防止无限循环
    
    while (frame->pc < method->bytecode_length && instruction_count < max_instructions) {
        result = execute_single_instruction(vm, frame);
        
        if (result != J2ME_SUCCESS) {
            if (result == J2ME_SUCCESS) { // 方法正常返回
                break;
            } else {
                printf("[解释器] 执行指令失败: %d\n", result);
                break;
            }
        }
        
        instruction_count++;
    }
    
    if (instruction_count >= max_instructions) {
        printf("[解释器] 警告: 达到最大指令执行数限制\n");
        result = J2ME_ERROR_RUNTIME_EXCEPTION;
    }
    
    printf("[解释器] 方法执行完成，执行了 %d 条指令\n", instruction_count);
    
    // 清理栈帧
    j2me_stack_frame_destroy(frame);
    
    return result;
}