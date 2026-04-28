#include "j2me_interpreter.h"
#include "j2me_bytecode.h"
#include "j2me_vm.h"
#include "j2me_native_methods.h"
#include "j2me_constant_pool.h"
#include "j2me_field_access.h"
#include "j2me_method_invocation.h"
#include "j2me_exception.h"
#include <stdlib.h>
#include <string.h>
#include "j2me_log.h"
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
#define OPCODE_ALOAD        0x19
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
#define OPCODE_IF_ICMPLT    0xa1
#define OPCODE_IF_ICMPGE    0xa2
#define OPCODE_IF_ICMPGT    0xa3
#define OPCODE_IF_ICMPLE    0xa4
#define OPCODE_GOTO         0xa7
#define OPCODE_IRETURN      0xac
#define OPCODE_ARETURN      0xb0
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
    
    // 初始化返回值字段
    frame->return_value = 0;
    frame->has_return_value = false;
    
    return frame;
}

j2me_error_t j2me_interpreter_parse_method_parameters(const char* descriptor, 
                                                     void* args, 
                                                     j2me_stack_frame_t* frame, 
                                                     int* local_var_index) {
    if (!descriptor || !frame || !local_var_index) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 简化的参数解析实现
    // 在实际应用中，这里应该解析完整的方法描述符
    const char* params_start = strchr(descriptor, '(');
    const char* params_end = strchr(descriptor, ')');
    
    if (!params_start || !params_end || params_start >= params_end) {
        return J2ME_SUCCESS; // 无参数方法
    }
    
    // 跳过开括号
    params_start++;
    
    // 解析参数类型
    const char* current = params_start;
    j2me_int* arg_values = (j2me_int*)args;
    int arg_count = 0;
    
    while (current < params_end && *local_var_index < frame->local_vars.size) {
        switch (*current) {
            case 'I': // int
            case 'Z': // boolean
            case 'B': // byte
            case 'C': // char
            case 'S': // short
                if (arg_values) {
                    frame->local_vars.variables[(*local_var_index)++] = arg_values[arg_count++];
                } else {
                    frame->local_vars.variables[(*local_var_index)++] = 0;
                }
                current++;
                break;
                
            case 'J': // long (占用两个局部变量槽)
                if (arg_values) {
                    frame->local_vars.variables[(*local_var_index)++] = arg_values[arg_count++];
                    frame->local_vars.variables[(*local_var_index)++] = arg_values[arg_count++];
                } else {
                    frame->local_vars.variables[(*local_var_index)++] = 0;
                    frame->local_vars.variables[(*local_var_index)++] = 0;
                }
                current++;
                break;
                
            case 'F': // float
            case 'D': // double
                if (arg_values) {
                    frame->local_vars.variables[(*local_var_index)++] = arg_values[arg_count++];
                } else {
                    frame->local_vars.variables[(*local_var_index)++] = 0;
                }
                current++;
                break;
                
            case 'L': // 对象引用
                // 跳过到分号
                while (current < params_end && *current != ';') {
                    current++;
                }
                if (*current == ';') {
                    current++;
                }
                if (arg_values) {
                    frame->local_vars.variables[(*local_var_index)++] = arg_values[arg_count++];
                } else {
                    frame->local_vars.variables[(*local_var_index)++] = 0;
                }
                break;
                
            case '[': // 数组
                // 跳过数组标记
                while (current < params_end && *current == '[') {
                    current++;
                }
                // 处理数组元素类型
                if (current < params_end) {
                    if (*current == 'L') {
                        // 对象数组
                        while (current < params_end && *current != ';') {
                            current++;
                        }
                        if (*current == ';') {
                            current++;
                        }
                    } else {
                        // 基本类型数组
                        current++;
                    }
                }
                if (arg_values) {
                    frame->local_vars.variables[(*local_var_index)++] = arg_values[arg_count++];
                } else {
                    frame->local_vars.variables[(*local_var_index)++] = 0;
                }
                break;
                
            default:
                current++;
                break;
        }
    }
    
    LOG_DEBUG("[解释器] 解析方法参数完成，参数数量: %d\n", arg_count);
    return J2ME_SUCCESS;
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
            
        case 0x12: // OPCODE_LDC
            // 从常量池加载常量
            {
                uint8_t index = frame->bytecode[frame->pc++];
                LOG_DEBUG("[解释器] ldc: 常量池索引 #%d\n", index);
                
                // 获取当前方法信息以访问常量池
                j2me_method_t* current_method = (j2me_method_t*)frame->method_info;
                if (current_method && current_method->owner_class) {
                    j2me_constant_value_t constant_value;
                    j2me_error_t resolve_result = j2me_resolve_constant_pool_entry(vm, 
                                                                                   current_method->owner_class, 
                                                                                   index, 
                                                                                   &constant_value);
                    
                    if (resolve_result == J2ME_SUCCESS) {
                        j2me_int stack_value = 0;
                        
                        switch (constant_value.type) {
                            case J2ME_CONSTANT_INTEGER:
                                stack_value = constant_value.data.int_value;
                                LOG_DEBUG("[解释器] ldc: 加载整数常量 %d\n", stack_value);
                                break;
                                
                            case J2ME_CONSTANT_FLOAT:
                                // 将float转换为int表示压入栈
                                stack_value = *(j2me_int*)&constant_value.data.float_value;
                                LOG_DEBUG("[解释器] ldc: 加载浮点常量 %f\n", constant_value.data.float_value);
                                break;
                                
                            case J2ME_CONSTANT_STRING:
                                // 字符串对象引用
                                stack_value = (j2me_int)(intptr_t)constant_value.data.object_ref;
                                LOG_DEBUG("[解释器] ldc: 加载字符串常量引用 0x%x\n", stack_value);
                                break;
                                
                            case J2ME_CONSTANT_CLASS:
                                // 类对象引用
                                stack_value = (j2me_int)(intptr_t)constant_value.data.class_ref;
                                LOG_DEBUG("[解释器] ldc: 加载类常量引用 0x%x\n", stack_value);
                                break;
                                
                            default:
                                LOG_DEBUG("[解释器] ldc: 警告: 不支持的常量类型 %d\n", constant_value.type);
                                stack_value = index; // 回退到索引值
                                break;
                        }
                        
                        result = j2me_operand_stack_push(&frame->operand_stack, stack_value);
                    } else {
                        LOG_DEBUG("[解释器] ldc: 常量解析失败: %d，使用索引值\n", resolve_result);
                        result = j2me_operand_stack_push(&frame->operand_stack, index);
                    }
                } else {
                    LOG_DEBUG("[解释器] ldc: 无法获取类信息，使用索引值\n");
                    result = j2me_operand_stack_push(&frame->operand_stack, index);
                }
            }
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
            
        case OPCODE_ALOAD:
            // 从局部变量加载引用
            value1 = frame->bytecode[frame->pc++];
            if (value1 < frame->local_vars.size) {
                result = j2me_operand_stack_push(&frame->operand_stack, frame->local_vars.variables[value1]);
            } else {
                result = J2ME_ERROR_INVALID_PARAMETER;
            }
            break;
            
        case OPCODE_ILOAD_0:
        case OPCODE_ILOAD_1:
        case OPCODE_ILOAD_2:
        case OPCODE_ILOAD_3:
            // 从局部变量加载int到栈
            value1 = opcode - OPCODE_ILOAD_0;
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
            
        case OPCODE_ASTORE:
            // 存储引用到局部变量
            {
                uint8_t local_index = frame->bytecode[frame->pc++];
                LOG_DEBUG("[解释器] astore: 存储到局部变量 #%d\n", local_index);
                
                j2me_int ref_value;
                result = j2me_operand_stack_pop(&frame->operand_stack, &ref_value);
                if (result == J2ME_SUCCESS && local_index < frame->local_vars.size) {
                    frame->local_vars.variables[local_index] = ref_value;
                    LOG_DEBUG("[解释器] astore: 存储引用 0x%x 到局部变量 #%d\n", ref_value, local_index);
                } else {
                    LOG_DEBUG("[解释器] astore: 局部变量索引 %d 超出范围 (大小: %zu)\n", 
                           local_index, frame->local_vars.size);
                    result = J2ME_ERROR_INVALID_PARAMETER;
                }
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
                if (result != J2ME_SUCCESS) {
                    printf("[ERROR] istore_%d: 栈弹出失败, result=%d, stack_top=%d\n", 
                           value1, result, frame->operand_stack.top);
                } else {
                    printf("[ERROR] istore_%d: 局部变量索引越界, index=%d, size=%d\n", 
                           value1, value1, frame->local_vars.size);
                }
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
            
        case OPCODE_IF_ICMPLT:
            // 如果value1 < value2则跳转
            result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
            if (result == J2ME_SUCCESS) {
                result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
                if (result == J2ME_SUCCESS) {
                    j2me_short offset = (j2me_short)((frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1]);
                    frame->pc += 2;
                    if (value1 < value2) {
                        frame->pc = (frame->pc - 3) + offset; // 跳转到目标位置
                    }
                }
            }
            break;
            
        case OPCODE_IF_ICMPGE:
            // 如果value1 >= value2则跳转
            result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
            if (result == J2ME_SUCCESS) {
                result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
                if (result == J2ME_SUCCESS) {
                    j2me_short offset = (j2me_short)((frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1]);
                    frame->pc += 2;
                    if (value1 >= value2) {
                        frame->pc = (frame->pc - 3) + offset; // 跳转到目标位置
                    }
                }
            }
            break;
            
        case OPCODE_IF_ICMPGT:
            // 如果value1 > value2则跳转
            result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
            if (result == J2ME_SUCCESS) {
                result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
                if (result == J2ME_SUCCESS) {
                    j2me_short offset = (j2me_short)((frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1]);
                    frame->pc += 2;
                    if (value1 > value2) {
                        frame->pc = (frame->pc - 3) + offset; // 跳转到目标位置
                    }
                }
            }
            break;
            
        case OPCODE_IF_ICMPLE:
            // 如果value1 <= value2则跳转
            result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
            if (result == J2ME_SUCCESS) {
                result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
                if (result == J2ME_SUCCESS) {
                    j2me_short offset = (j2me_short)((frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1]);
                    frame->pc += 2;
                    if (value1 <= value2) {
                        frame->pc = (frame->pc - 3) + offset; // 跳转到目标位置
                    }
                }
            }
            break;
            
        case 0xc6: // OPCODE_IFNULL
            // 如果引用为null则跳转
            result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
            if (result == J2ME_SUCCESS) {
                j2me_short offset = (j2me_short)((frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1]);
                frame->pc += 2;
                if (value1 == 0) {
                    frame->pc = (frame->pc - 3) + offset; // 跳转到目标位置
                    LOG_DEBUG("[解释器] ifnull: 引用为null，跳转到偏移 %d\n", offset);
                } else {
                    LOG_DEBUG("[解释器] ifnull: 引用非null (0x%x)，继续执行\n", value1);
                }
            }
            break;
            
        case 0xc7: // OPCODE_IFNONNULL
            // 如果引用不为null则跳转
            result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
            if (result == J2ME_SUCCESS) {
                j2me_short offset = (j2me_short)((frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1]);
                frame->pc += 2;
                if (value1 != 0) {
                    frame->pc = (frame->pc - 3) + offset; // 跳转到目标位置
                    LOG_DEBUG("[解释器] ifnonnull: 引用非null (0x%x)，跳转到偏移 %d\n", value1, offset);
                } else {
                    LOG_DEBUG("[解释器] ifnonnull: 引用为null，继续执行\n");
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
            if (result == J2ME_SUCCESS) {
                // 将返回值存储到栈帧中，供调用者获取
                frame->return_value = value1;
                frame->has_return_value = true;
                // 设置PC到一个超出范围的值，停止执行
                frame->pc = 0xFFFFFFFF;
                LOG_DEBUG("[解释器] ireturn: 返回值 %d\n", value1);
            }
            return J2ME_SUCCESS; // 特殊处理，表示方法结束
            
        case OPCODE_ARETURN:
            // 返回引用值
            result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
            if (result == J2ME_SUCCESS) {
                // 将返回值存储到栈帧中，供调用者获取
                frame->return_value = value1;
                frame->has_return_value = true;
                // 设置PC到一个超出范围的值，停止执行
                frame->pc = 0xFFFFFFFF;
                LOG_DEBUG("[解释器] areturn: 返回引用 0x%x\n", value1);
            }
            return J2ME_SUCCESS; // 特殊处理，表示方法结束
            
        case OPCODE_RETURN:
            // 方法返回
            // 设置PC到一个超出范围的值，停止执行
            frame->pc = 0xFFFFFFFF;
            return J2ME_SUCCESS; // 特殊处理，表示方法结束
            
        case OPCODE_GETSTATIC:
            // 获取静态字段
            {
                uint16_t field_ref_index = (frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1];
                frame->pc += 2;
                
                LOG_DEBUG("[解释器] getstatic: 字段引用索引 #%d\n", field_ref_index);
                
                // 获取当前方法信息以访问常量池
                j2me_method_t* current_method = (j2me_method_t*)frame->method_info;
                if (current_method && current_method->owner_class) {
                    // 解析字段引用，获取目标类
                    j2me_constant_pool_entry_t* field_ref = 
                        &current_method->owner_class->constant_pool.entries[field_ref_index - 1];
                    
                    if (field_ref->tag == J2ME_CONSTANT_FIELDREF) {
                        // 获取类索引
                        uint16_t class_index = field_ref->info.ref_info.class_index;
                        j2me_constant_pool_entry_t* class_entry = 
                            &current_method->owner_class->constant_pool.entries[class_index - 1];
                        
                        const char* class_name = NULL;
                        if (class_entry->tag == J2ME_CONSTANT_CLASS) {
                            j2me_constant_pool_entry_t* name_entry = 
                                &current_method->owner_class->constant_pool.entries[class_entry->info.class_info.name_index - 1];
                            if (name_entry->tag == J2ME_CONSTANT_UTF8) {
                                class_name = name_entry->info.utf8.bytes;
                            }
                        }
                        
                        // 确保目标类已加载和初始化
                        if (class_name) {
                            j2me_class_t* target_class = j2me_class_loader_find_class(vm->class_loader, class_name);
                            if (!target_class) {
                                LOG_DEBUG("[解释器] getstatic: 类未加载，尝试加载类 %s\n", class_name);
                                target_class = j2me_class_loader_load_class(vm->class_loader, class_name);
                            }
                            
                            // 确保类已初始化
                            if (target_class && target_class->state != CLASS_INITIALIZED) {
                                LOG_DEBUG("[解释器] getstatic: 类未初始化，执行初始化 %s\n", class_name);
                                j2me_error_t init_result = j2me_class_initialize(target_class);
                                if (init_result != J2ME_SUCCESS) {
                                    LOG_DEBUG("[解释器] getstatic: 类初始化失败: %d\n", init_result);
                                }
                            }
                        }
                    }
                    
                    j2me_value_t field_value;
                    j2me_error_t field_result = j2me_get_static_field(vm, 
                                                                      current_method->owner_class, 
                                                                      field_ref_index, 
                                                                      &field_value);
                    
                    if (field_result == J2ME_SUCCESS) {
                        result = j2me_operand_stack_push(&frame->operand_stack, field_value.int_value);
                        LOG_DEBUG("[解释器] getstatic: 获取静态字段值 0x%x\n", field_value.int_value);
                    } else {
                        LOG_DEBUG("[解释器] getstatic: 字段访问失败: %d，使用null\n", field_result);
                        j2me_int default_value = 0; // 返回null而不是假引用
                        result = j2me_operand_stack_push(&frame->operand_stack, default_value);
                    }
                } else {
                    LOG_DEBUG("[解释器] getstatic: 无法获取类信息，使用null\n");
                    j2me_int default_value = 0; // 返回null而不是假引用
                    result = j2me_operand_stack_push(&frame->operand_stack, default_value);
                }
            }
            break;
            
        case OPCODE_PUTSTATIC:
            // 设置静态字段
            {
                uint16_t field_ref_index = (frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1];
                frame->pc += 2;
                
                LOG_DEBUG("[解释器] putstatic: 字段引用索引 #%d\n", field_ref_index);
                
                // 弹出要设置的值
                j2me_int field_value;
                result = j2me_operand_stack_pop(&frame->operand_stack, &field_value);
                
                if (result == J2ME_SUCCESS) {
                    // 获取当前方法信息以访问常量池
                    j2me_method_t* current_method = (j2me_method_t*)frame->method_info;
                    if (current_method && current_method->owner_class) {
                        // 解析字段引用，获取目标类
                        j2me_constant_pool_entry_t* field_ref = 
                            &current_method->owner_class->constant_pool.entries[field_ref_index - 1];
                        
                        if (field_ref->tag == J2ME_CONSTANT_FIELDREF) {
                            // 获取类索引
                            uint16_t class_index = field_ref->info.ref_info.class_index;
                            j2me_constant_pool_entry_t* class_entry = 
                                &current_method->owner_class->constant_pool.entries[class_index - 1];
                            
                            const char* class_name = NULL;
                            if (class_entry->tag == J2ME_CONSTANT_CLASS) {
                                j2me_constant_pool_entry_t* name_entry = 
                                    &current_method->owner_class->constant_pool.entries[class_entry->info.class_info.name_index - 1];
                                if (name_entry->tag == J2ME_CONSTANT_UTF8) {
                                    class_name = name_entry->info.utf8.bytes;
                                }
                            }
                            
                            // 确保目标类已加载和初始化
                            if (class_name) {
                                j2me_class_t* target_class = j2me_class_loader_find_class(vm->class_loader, class_name);
                                if (!target_class) {
                                    LOG_DEBUG("[解释器] putstatic: 类未加载，尝试加载类 %s\n", class_name);
                                    target_class = j2me_class_loader_load_class(vm->class_loader, class_name);
                                }
                                
                                // 确保类已初始化
                                if (target_class && target_class->state != CLASS_INITIALIZED) {
                                    LOG_DEBUG("[解释器] putstatic: 类未初始化，执行初始化 %s\n", class_name);
                                    j2me_error_t init_result = j2me_class_initialize(target_class);
                                    if (init_result != J2ME_SUCCESS) {
                                        LOG_DEBUG("[解释器] putstatic: 类初始化失败: %d\n", init_result);
                                    }
                                }
                            }
                        }
                        
                        j2me_value_t value;
                        value.type = J2ME_TYPE_INT;
                        value.int_value = field_value;
                        
                        j2me_error_t field_result = j2me_set_static_field(vm, 
                                                                          current_method->owner_class, 
                                                                          field_ref_index, 
                                                                          &value);
                        
                        if (field_result == J2ME_SUCCESS) {
                            LOG_DEBUG("[解释器] putstatic: 设置静态字段值 0x%x\n", field_value);
                        } else {
                            LOG_DEBUG("[解释器] putstatic: 字段设置失败: %d\n", field_result);
                        }
                    } else {
                        LOG_DEBUG("[解释器] putstatic: 无法获取类信息，忽略设置操作\n");
                    }
                }
            }
            break;
            
        case OPCODE_GETFIELD:
            // 获取实例字段
            {
                uint16_t field_ref_index = (frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1];
                frame->pc += 2;
                
                LOG_DEBUG("[解释器] getfield: 字段引用索引 #%d\n", field_ref_index);
                
                // 弹出对象引用
                j2me_int object_ref;
                result = j2me_operand_stack_pop(&frame->operand_stack, &object_ref);
                
                if (result == J2ME_SUCCESS) {
                    // 获取当前方法信息以访问常量池
                    j2me_method_t* current_method = (j2me_method_t*)frame->method_info;
                    if (current_method && current_method->owner_class) {
                        j2me_value_t field_value;
                        j2me_object_t* object = (j2me_object_t*)(intptr_t)object_ref;
                        
                        // 如果对象引用为null，返回0
                        if (object_ref == 0) {
                            LOG_DEBUG("[解释器] getfield: 对象引用为null，返回0\n");
                            result = j2me_operand_stack_push(&frame->operand_stack, 0);
                        } else {
                            j2me_error_t field_result = j2me_get_instance_field(vm, 
                                                                                object,
                                                                                current_method->owner_class, 
                                                                                field_ref_index, 
                                                                                &field_value);
                            
                            if (field_result == J2ME_SUCCESS) {
                                result = j2me_operand_stack_push(&frame->operand_stack, field_value.int_value);
                                LOG_DEBUG("[解释器] getfield: 从对象 0x%x 获取字段值 0x%x\n", object_ref, field_value.int_value);
                            } else {
                                LOG_DEBUG("[解释器] getfield: 字段访问失败: %d，返回0\n", field_result);
                                result = j2me_operand_stack_push(&frame->operand_stack, 0);
                            }
                        }
                    } else {
                        LOG_DEBUG("[解释器] getfield: 无法获取类信息，返回0\n");
                        result = j2me_operand_stack_push(&frame->operand_stack, 0);
                    }
                }
            }
            break;
            
        case OPCODE_PUTFIELD:
            // 设置实例字段
            {
                uint16_t field_ref_index = (frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1];
                frame->pc += 2;
                
                LOG_DEBUG("[解释器] putfield: 字段引用索引 #%d\n", field_ref_index);
                
                // 弹出字段值和对象引用
                j2me_int field_value, object_ref;
                result = j2me_operand_stack_pop(&frame->operand_stack, &field_value);
                if (result == J2ME_SUCCESS) {
                    result = j2me_operand_stack_pop(&frame->operand_stack, &object_ref);
                    
                    if (result == J2ME_SUCCESS) {
                        // 获取当前方法信息以访问常量池
                        j2me_method_t* current_method = (j2me_method_t*)frame->method_info;
                        if (current_method && current_method->owner_class) {
                            j2me_value_t value;
                            value.type = J2ME_TYPE_INT;
                            value.int_value = field_value;
                            
                            j2me_object_t* object = (j2me_object_t*)(intptr_t)object_ref;
                            
                            // 如果对象引用为null（构造过程中），直接忽略字段设置
                            if (object_ref == 0) {
                                LOG_DEBUG("[解释器] putfield: 对象引用为null，忽略字段设置 (字段引用: #%d)\n", field_ref_index);
                            } else {
                                j2me_error_t field_result = j2me_set_instance_field(vm, 
                                                                                    object,
                                                                                    current_method->owner_class, 
                                                                                    field_ref_index, 
                                                                                    &value);
                                
                                if (field_result == J2ME_SUCCESS) {
                                    LOG_DEBUG("[解释器] putfield: 设置对象 0x%x 的字段值 0x%x\n", object_ref, field_value);
                                } else {
                                    LOG_DEBUG("[解释器] putfield: 字段设置失败: %d，忽略继续执行\n", field_result);
                                }
                            }
                        } else {
                            LOG_DEBUG("[解释器] putfield: 无法获取类信息，忽略设置操作\n");
                        }
                    }
                }
            }
            break;
            
        case OPCODE_INVOKESPECIAL:
            // 调用特殊方法 (构造方法、私有方法、父类方法)
            {
                // 获取方法引用索引 (2字节)
                uint16_t method_ref_index = (frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1];
                frame->pc += 2;
                
                // 清除之前的返回值标志
                vm->last_method_has_return_value = false;
                
                // 使用新的方法调用系统
                result = j2me_method_invocation_invoke_special(vm, frame, method_ref_index);
                if (result != J2ME_SUCCESS) {
                    LOG_DEBUG("[解释器] invokespecial: 方法调用失败: %d\n", result);
                    
                    // 检查是否有异常
                    if (j2me_has_pending_exception(vm)) {
                        j2me_exception_t* exception = j2me_get_current_exception(vm);
                        j2me_handle_exception(vm, exception);
                    }
                } else {
                    // 如果方法有返回值，将其压入栈
                    if (vm->last_method_has_return_value) {
                        result = j2me_operand_stack_push(&frame->operand_stack, vm->last_method_return_value);
                        LOG_DEBUG("[解释器] invokespecial: 压入返回值 0x%x\n", vm->last_method_return_value);
                        vm->last_method_has_return_value = false;
                    }
                }
            }
            break;
            
        case OPCODE_INVOKEVIRTUAL:
            // 调用虚方法
            {
                uint16_t method_ref_index = (frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1];
                frame->pc += 2;
                
                // 清除之前的返回值标志
                vm->last_method_has_return_value = false;
                
                // 使用新的方法调用系统
                result = j2me_method_invocation_invoke_virtual(vm, frame, method_ref_index);
                if (result != J2ME_SUCCESS) {
                    LOG_DEBUG("[解释器] invokevirtual: 方法调用失败: %d\n", result);
                    
                    // 检查是否有异常
                    if (j2me_has_pending_exception(vm)) {
                        j2me_exception_t* exception = j2me_get_current_exception(vm);
                        j2me_handle_exception(vm, exception);
                    }
                } else {
                    // 如果方法有返回值，将其压入栈
                    if (vm->last_method_has_return_value) {
                        result = j2me_operand_stack_push(&frame->operand_stack, vm->last_method_return_value);
                        LOG_DEBUG("[解释器] invokevirtual: 压入返回值 0x%x\n", vm->last_method_return_value);
                        vm->last_method_has_return_value = false;
                    }
                }
            }
            break;
            
        case OPCODE_INVOKESTATIC:
            // 调用静态方法
            {
                uint16_t method_ref_index = (frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1];
                frame->pc += 2;
                
                // 清除之前的返回值标志
                vm->last_method_has_return_value = false;
                
                // 使用新的方法调用系统
                result = j2me_method_invocation_invoke_static(vm, frame, method_ref_index);
                if (result != J2ME_SUCCESS) {
                    LOG_DEBUG("[解释器] invokestatic: 方法调用失败: %d\n", result);
                    
                    // 检查是否有异常
                    if (j2me_has_pending_exception(vm)) {
                        j2me_exception_t* exception = j2me_get_current_exception(vm);
                        j2me_handle_exception(vm, exception);
                    }
                } else {
                    // 如果方法有返回值，将其压入栈
                    if (vm->last_method_has_return_value) {
                        result = j2me_operand_stack_push(&frame->operand_stack, vm->last_method_return_value);
                        LOG_DEBUG("[解释器] invokestatic: 压入返回值 0x%x\n", vm->last_method_return_value);
                        vm->last_method_has_return_value = false;
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
                
                // 使用新的方法调用系统
                result = j2me_method_invocation_invoke_interface(vm, frame, method_ref_index, count);
                if (result != J2ME_SUCCESS) {
                    LOG_DEBUG("[解释器] invokeinterface: 方法调用失败: %d\n", result);
                    
                    // 检查是否有异常
                    if (j2me_has_pending_exception(vm)) {
                        j2me_exception_t* exception = j2me_get_current_exception(vm);
                        j2me_handle_exception(vm, exception);
                    }
                }
            }
            break;
            
        case OPCODE_NEW:
            // 创建新对象
            {
                uint16_t class_index = (frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1];
                frame->pc += 2;
                
                printf("[解释器] NEW: 类索引 #%d\n", class_index);
                
                // 获取当前方法信息以访问常量池
                j2me_method_t* current_method = (j2me_method_t*)frame->method_info;
                j2me_int object_ref = 0;
                
                if (current_method && current_method->owner_class && vm && vm->heap) {
                    // 解析类引用
                    j2me_constant_pool_entry_t* class_entry = 
                        &current_method->owner_class->constant_pool.entries[class_index - 1];
                    
                    if (class_entry->tag == J2ME_CONSTANT_CLASS) {
                        j2me_constant_pool_entry_t* name_entry = 
                            &current_method->owner_class->constant_pool.entries[class_entry->info.class_info.name_index - 1];
                        
                        if (name_entry->tag == J2ME_CONSTANT_UTF8) {
                            const char* class_name = name_entry->info.utf8.bytes;
                            printf("[解释器] NEW: 创建类 %s 的实例\n", class_name);
                            
                            // 查找类，如果未找到则尝试加载
                            j2me_class_t* target_class = j2me_class_loader_find_class(vm->class_loader, class_name);
                            if (!target_class) {
                                printf("[解释器] NEW: 类未加载，尝试加载类 %s\n", class_name);
                                target_class = j2me_class_loader_load_class(vm->class_loader, class_name);
                            }
                            
                            if (target_class) {
                                // 确保类已初始化
                                if (target_class->state != CLASS_INITIALIZED) {
                                    printf("[解释器] NEW: 类未初始化，执行初始化 %s\n", class_name);
                                    j2me_error_t init_result = j2me_class_initialize(target_class);
                                    if (init_result != J2ME_SUCCESS) {
                                        printf("[解释器] NEW: 类初始化失败: %d\n", init_result);
                                    }
                                }
                                
                                // 使用类指针地址作为class_id
                                uint32_t class_id = (uint32_t)(uintptr_t)target_class;
                                
                                // 计算对象大小（简化：使用固定大小）
                                size_t object_size = target_class->instance_size > 0 ? target_class->instance_size : 64;
                                
                                // 在堆上创建对象
                                object_ref = j2me_heap_alloc(vm->heap, class_id, object_size);
                                if (object_ref != J2ME_NULL_REF) {
                                    printf("[解释器] NEW: 成功创建对象 0x%x (类: %s, 大小: %zu)\n", 
                                             object_ref, class_name, object_size);
                                    
                                    // 获取对象并设置类指针
                                    j2me_heap_object_header_t* obj = j2me_heap_get_object(vm->heap, object_ref);
                                    if (obj) {
                                        // 将类指针存储在对象数据的开头（兼容旧的对象系统）
                                        if (object_size >= sizeof(void*)) {
                                            *((j2me_class_t**)obj->data) = target_class;
                                        }
                                    }
                                    
                                    // 如果是Canvas类或其子类，保存到VM
                                    // 使用类继承信息准确判断
                                    if (j2me_class_is_subclass_of(target_class, "javax/microedition/lcdui/Canvas") ||
                                        j2me_class_is_subclass_of(target_class, "javax/microedition/lcdui/game/GameCanvas")) {
                                        vm->last_canvas_object_ref = object_ref;
                                        printf("[解释器] NEW: 保存Canvas子类对象引用到VM: 0x%x (类: %s, 父类: %s)\n", 
                                               object_ref, class_name, target_class->super_name ? target_class->super_name : "none");
                                    }
                                } else {
                                    printf("[解释器] NEW: 堆分配失败，使用假引用\n");
                                    object_ref = 0x12345678; // 回退到假引用
                                }
                            } else {
                                printf("[解释器] NEW: 无法加载类 %s，创建通用对象\n", class_name);
                                // 对于无法加载的类（如系统类），创建一个通用对象
                                uint32_t class_id = 0xFFFFFFFF; // 特殊class_id表示未知类
                                size_t object_size = 64; // 默认大小
                                
                                object_ref = j2me_heap_alloc(vm->heap, class_id, object_size);
                                if (object_ref != J2ME_NULL_REF) {
                                    printf("[解释器] NEW: 创建通用对象成功 0x%x (类: %s)\n", object_ref, class_name);
                                } else {
                                    printf("[解释器] NEW: 堆分配失败，使用假引用\n");
                                    object_ref = 0x12345678; // 回退到假引用
                                }
                            }
                        }
                    }
                } else {
                    printf("[解释器] NEW: VM或堆未初始化，使用假引用\n");
                    object_ref = 0x12345678; // 回退到假引用
                }
                
                result = j2me_operand_stack_push(&frame->operand_stack, object_ref);
                
                // 保存最后创建的对象引用到VM（用于后续的putfield等操作）
                if (vm) {
                    vm->last_method_return_value = object_ref;
                    vm->last_method_has_return_value = true;
                }
                
                printf("[解释器] NEW: 对象引用已压栈 0x%x\n", object_ref);
            }
            break;
            
        case 0x13: // OPCODE_LDC_W
            // 从常量池加载常量 (宽索引)
            {
                uint16_t index = (frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1];
                frame->pc += 2;
                LOG_DEBUG("[解释器] ldc_w: 常量池索引 #%d\n", index);
                
                // 获取当前方法信息以访问常量池
                j2me_method_t* current_method = (j2me_method_t*)frame->method_info;
                if (current_method && current_method->owner_class) {
                    j2me_constant_value_t constant_value;
                    j2me_error_t resolve_result = j2me_resolve_constant_pool_entry(vm, 
                                                                                   current_method->owner_class, 
                                                                                   index, 
                                                                                   &constant_value);
                    
                    if (resolve_result == J2ME_SUCCESS) {
                        j2me_int stack_value = 0;
                        
                        switch (constant_value.type) {
                            case J2ME_CONSTANT_INTEGER:
                                stack_value = constant_value.data.int_value;
                                LOG_DEBUG("[解释器] ldc_w: 加载整数常量 %d\n", stack_value);
                                break;
                                
                            case J2ME_CONSTANT_FLOAT:
                                // 将float转换为int表示压入栈
                                stack_value = *(j2me_int*)&constant_value.data.float_value;
                                LOG_DEBUG("[解释器] ldc_w: 加载浮点常量 %f\n", constant_value.data.float_value);
                                break;
                                
                            case J2ME_CONSTANT_STRING:
                                // 字符串对象引用
                                stack_value = (j2me_int)(intptr_t)constant_value.data.object_ref;
                                LOG_DEBUG("[解释器] ldc_w: 加载字符串常量引用 0x%x\n", stack_value);
                                break;
                                
                            case J2ME_CONSTANT_CLASS:
                                // 类对象引用
                                stack_value = (j2me_int)(intptr_t)constant_value.data.class_ref;
                                LOG_DEBUG("[解释器] ldc_w: 加载类常量引用 0x%x\n", stack_value);
                                break;
                                
                            default:
                                LOG_DEBUG("[解释器] ldc_w: 警告: 不支持的常量类型 %d\n", constant_value.type);
                                stack_value = index; // 回退到索引值
                                break;
                        }
                        
                        result = j2me_operand_stack_push(&frame->operand_stack, stack_value);
                    } else {
                        LOG_DEBUG("[解释器] ldc_w: 常量解析失败: %d，使用索引值\n", resolve_result);
                        result = j2me_operand_stack_push(&frame->operand_stack, index);
                    }
                } else {
                    LOG_DEBUG("[解释器] ldc_w: 无法获取类信息，使用索引值\n");
                    result = j2me_operand_stack_push(&frame->operand_stack, index);
                }
            }
            break;
            
        case 0x84: // OPCODE_IINC
            // 局部变量增量
            {
                uint8_t index = frame->bytecode[frame->pc++];
                int8_t increment = (int8_t)frame->bytecode[frame->pc++];
                
                LOG_DEBUG("[解释器] iinc: 局部变量 %d 增加 %d\n", index, increment);
                
                if (index < frame->local_vars.size) {
                    frame->local_vars.variables[index] += increment;
                    LOG_DEBUG("[解释器] iinc: 局部变量 %d 新值: %d\n", index, frame->local_vars.variables[index]);
                } else {
                    LOG_DEBUG("[解释器] iinc: 错误: 无效的局部变量索引 %d\n", index);
                    result = J2ME_ERROR_INVALID_PARAMETER;
                }
            }
            break;
            
        case 0xa8: // OPCODE_JSR
            // 跳转到子程序
            {
                j2me_short offset = (j2me_short)((frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1]);
                frame->pc += 2;
                
                LOG_DEBUG("[解释器] jsr: 跳转到子程序，偏移 %d\n", offset);
                
                // 将返回地址压入栈
                result = j2me_operand_stack_push(&frame->operand_stack, frame->pc);
                if (result == J2ME_SUCCESS) {
                    frame->pc = (frame->pc - 3) + offset; // 跳转到目标位置
                }
            }
            break;
            
        case 0xa9: // OPCODE_RET
            // 从子程序返回
            {
                uint8_t index = frame->bytecode[frame->pc++];
                
                LOG_DEBUG("[解释器] ret: 从子程序返回，局部变量索引 %d\n", index);
                
                if (index < frame->local_vars.size) {
                    frame->pc = frame->local_vars.variables[index];
                    LOG_DEBUG("[解释器] ret: 返回到地址 %d\n", frame->pc);
                } else {
                    LOG_DEBUG("[解释器] ret: 错误: 无效的局部变量索引 %d\n", index);
                    result = J2ME_ERROR_INVALID_PARAMETER;
                }
            }
            break;
            
        case 0xaa: // OPCODE_TABLESWITCH
            // 表格跳转
            {
                // 跳过填充字节，使偏移量对齐到4字节边界
                int padding = (4 - ((frame->pc - 1) % 4)) % 4;
                frame->pc += padding;
                
                // 读取默认偏移量
                int32_t default_offset = (frame->bytecode[frame->pc] << 24) | 
                                        (frame->bytecode[frame->pc + 1] << 16) |
                                        (frame->bytecode[frame->pc + 2] << 8) | 
                                        frame->bytecode[frame->pc + 3];
                frame->pc += 4;
                
                // 读取低值和高值
                int32_t low = (frame->bytecode[frame->pc] << 24) | 
                             (frame->bytecode[frame->pc + 1] << 16) |
                             (frame->bytecode[frame->pc + 2] << 8) | 
                             frame->bytecode[frame->pc + 3];
                frame->pc += 4;
                
                int32_t high = (frame->bytecode[frame->pc] << 24) | 
                              (frame->bytecode[frame->pc + 1] << 16) |
                              (frame->bytecode[frame->pc + 2] << 8) | 
                              frame->bytecode[frame->pc + 3];
                frame->pc += 4;
                
                LOG_DEBUG("[解释器] tableswitch: 范围 %d-%d, 默认偏移 %d\n", low, high, default_offset);
                
                // 弹出索引值
                j2me_int index;
                result = j2me_operand_stack_pop(&frame->operand_stack, &index);
                if (result == J2ME_SUCCESS) {
                    if (index >= low && index <= high) {
                        // 读取对应的跳转偏移量
                        int table_index = index - low;
                        int offset_pos = frame->pc + table_index * 4;
                        int32_t jump_offset = (frame->bytecode[offset_pos] << 24) | 
                                             (frame->bytecode[offset_pos + 1] << 16) |
                                             (frame->bytecode[offset_pos + 2] << 8) | 
                                             frame->bytecode[offset_pos + 3];
                        
                        frame->pc = (frame->pc - (padding + 13)) + jump_offset;
                        LOG_DEBUG("[解释器] tableswitch: 跳转到索引 %d, 偏移 %d\n", index, jump_offset);
                    } else {
                        frame->pc = (frame->pc - (padding + 13)) + default_offset;
                        LOG_DEBUG("[解释器] tableswitch: 使用默认跳转，偏移 %d\n", default_offset);
                    }
                    
                    // 跳过跳转表
                    frame->pc += (high - low + 1) * 4;
                }
            }
            break;
            
        case OPCODE_CHECKCAST:
            // 类型检查转换
            {
                uint16_t class_index = (frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1];
                frame->pc += 2;
                
                LOG_DEBUG("[解释器] checkcast: 类索引 #%d\n", class_index);
                
                // 弹出对象引用
                j2me_int object_ref;
                result = j2me_operand_stack_pop(&frame->operand_stack, &object_ref);
                
                if (result == J2ME_SUCCESS) {
                    // 简化实现：如果对象引用为null，直接通过
                    if (object_ref == 0) {
                        LOG_DEBUG("[解释器] checkcast: null引用，检查通过\n");
                        result = j2me_operand_stack_push(&frame->operand_stack, object_ref);
                    } else {
                        // 简化实现：假设所有非null对象都通过类型检查
                        LOG_DEBUG("[解释器] checkcast: 对象 0x%x 类型检查通过 (简化实现)\n", object_ref);
                        result = j2me_operand_stack_push(&frame->operand_stack, object_ref);
                    }
                }
            }
            break;
            
        case OPCODE_INSTANCEOF:
            // 实例检查
            {
                uint16_t class_index = (frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1];
                frame->pc += 2;
                
                LOG_DEBUG("[解释器] instanceof: 类索引 #%d\n", class_index);
                
                // 弹出对象引用
                j2me_int object_ref;
                result = j2me_operand_stack_pop(&frame->operand_stack, &object_ref);
                
                if (result == J2ME_SUCCESS) {
                    // 简化实现：如果对象引用为null，返回false；否则返回true
                    j2me_int is_instance = (object_ref != 0) ? 1 : 0;
                    LOG_DEBUG("[解释器] instanceof: 对象 0x%x 实例检查结果 %d\n", object_ref, is_instance);
                    result = j2me_operand_stack_push(&frame->operand_stack, is_instance);
                }
            }
            break;
            
        case OPCODE_NEWARRAY:
            // 创建基本类型数组
            {
                uint8_t array_type = frame->bytecode[frame->pc++];
                LOG_DEBUG("[解释器] newarray: 数组类型 %d\n", array_type);
                
                // 弹出数组长度
                j2me_int array_length;
                result = j2me_operand_stack_pop(&frame->operand_stack, &array_length);
                
                if (result == J2ME_SUCCESS) {
                    if (array_length < 0) {
                        LOG_DEBUG("[解释器] newarray: 数组长度为负数 %d\n", array_length);
                        result = J2ME_ERROR_RUNTIME_EXCEPTION;
                    } else if (array_length > 10000) {
                        LOG_DEBUG("[解释器] newarray: 数组长度过大 %d，限制为1000\n", array_length);
                        array_length = 1000;
                    }
                    
                    if (result == J2ME_SUCCESS) {
                        // 创建数组对象引用（简化实现）
                        // 数组引用格式：0x50000000 + (类型 << 16) + 长度
                        j2me_int array_ref = 0x50000000 + (array_type << 16) + (array_length & 0xFFFF);
                        
                        result = j2me_operand_stack_push(&frame->operand_stack, array_ref);
                        
                        const char* type_names[] = {
                            "unknown", "unknown", "unknown", "unknown", 
                            "boolean", "char", "float", "double", 
                            "byte", "short", "int", "long"
                        };
                        const char* type_name = (array_type >= 4 && array_type <= 11) ? 
                                                type_names[array_type] : "unknown";
                        
                        LOG_DEBUG("[解释器] newarray: 创建 %s[%d] 数组，引用 0x%x\n", 
                               type_name, array_length, array_ref);
                    }
                }
            }
            break;
            
        case OPCODE_ANEWARRAY:
            // 创建引用类型数组
            {
                uint16_t class_index = (frame->bytecode[frame->pc] << 8) | frame->bytecode[frame->pc + 1];
                frame->pc += 2;
                
                LOG_DEBUG("[解释器] anewarray: 类索引 #%d\n", class_index);
                
                // 弹出数组长度
                j2me_int array_length;
                result = j2me_operand_stack_pop(&frame->operand_stack, &array_length);
                
                if (result == J2ME_SUCCESS) {
                    if (array_length < 0) {
                        LOG_DEBUG("[解释器] anewarray: 数组长度为负数 %d\n", array_length);
                        result = J2ME_ERROR_RUNTIME_EXCEPTION;
                    } else if (array_length > 10000) {
                        LOG_DEBUG("[解释器] anewarray: 数组长度过大 %d，限制为1000\n", array_length);
                        array_length = 1000;
                    }
                    
                    if (result == J2ME_SUCCESS) {
                        // 创建引用数组对象引用（简化实现）
                        // 引用数组格式：0x60000000 + (类索引 << 8) + 长度
                        j2me_int array_ref = 0x60000000 + (class_index << 8) + (array_length & 0xFF);
                        
                        result = j2me_operand_stack_push(&frame->operand_stack, array_ref);
                        
                        LOG_DEBUG("[解释器] anewarray: 创建引用数组[%d]，引用 0x%x\n", 
                               array_length, array_ref);
                    }
                }
            }
            break;
            
        case 0xab: // OPCODE_LOOKUPSWITCH
            // 查找跳转指令
            {
                // 弹出key值
                j2me_int key;
                result = j2me_operand_stack_pop(&frame->operand_stack, &key);
                
                if (result == J2ME_SUCCESS) {
                    // lookupswitch指令需要4字节对齐
                    uint32_t base_pc = frame->pc - 1; // lookupswitch指令的起始位置
                    uint32_t aligned_pc = ((base_pc + 4) & ~3); // 对齐到4字节边界
                    
                    // 读取default偏移量（4字节，大端序）
                    int32_t default_offset = (frame->bytecode[aligned_pc] << 24) |
                                            (frame->bytecode[aligned_pc + 1] << 16) |
                                            (frame->bytecode[aligned_pc + 2] << 8) |
                                            frame->bytecode[aligned_pc + 3];
                    
                    // 读取npairs（匹配对数量）
                    int32_t npairs = (frame->bytecode[aligned_pc + 4] << 24) |
                                    (frame->bytecode[aligned_pc + 5] << 16) |
                                    (frame->bytecode[aligned_pc + 6] << 8) |
                                    frame->bytecode[aligned_pc + 7];
                    
                    // 查找匹配的key
                    bool found = false;
                    int32_t target_offset = default_offset;
                    
                    for (int32_t i = 0; i < npairs; i++) {
                        uint32_t pair_offset = aligned_pc + 8 + (i * 8);
                        
                        // 读取match值
                        int32_t match = (frame->bytecode[pair_offset] << 24) |
                                       (frame->bytecode[pair_offset + 1] << 16) |
                                       (frame->bytecode[pair_offset + 2] << 8) |
                                       frame->bytecode[pair_offset + 3];
                        
                        if (match == key) {
                            // 读取offset
                            target_offset = (frame->bytecode[pair_offset + 4] << 24) |
                                          (frame->bytecode[pair_offset + 5] << 16) |
                                          (frame->bytecode[pair_offset + 6] << 8) |
                                          frame->bytecode[pair_offset + 7];
                            found = true;
                            break;
                        }
                    }
                    
                    // 跳转到目标位置
                    frame->pc = base_pc + target_offset;
                    
                    // LOG_DEBUG("[解释器] lookupswitch: key=%d, %s, 跳转到偏移 %d\n", 
                    //        key, found ? "找到匹配" : "使用default", target_offset);
                }
            }
            break;
            
        default:
            LOG_DEBUG("[解释器] 未实现的指令: %s (0x%02x)\n", inst_name, opcode);
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
        LOG_DEBUG("[方法解析] 错误: 无效的方法引用索引 #%d (常量池大小: %d)\n", 
               method_ref_index, class_info->constant_pool.count);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 常量池索引从1开始，但数组索引从0开始，所以需要减1
    j2me_constant_pool_entry_t* method_ref = &class_info->constant_pool.entries[method_ref_index - 1];
    
    // 检查是否为方法引用或接口方法引用
    if (method_ref->tag != J2ME_CONSTANT_METHODREF && method_ref->tag != J2ME_CONSTANT_INTERFACE_METHODREF) {
        LOG_DEBUG("[方法解析] 错误: 常量池条目 #%d 不是方法引用 (类型: %d)\n", 
               method_ref_index, method_ref->tag);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    uint16_t class_index = method_ref->info.ref_info.class_index;
    uint16_t name_and_type_index = method_ref->info.ref_info.name_and_type_index;
    
    LOG_DEBUG("[方法解析] 方法引用 #%d: class_index=%d, name_and_type_index=%d\n", 
           method_ref_index, class_index, name_and_type_index);
    
    // 解析类名
    if (class_index == 0 || class_index >= class_info->constant_pool.count) {
        LOG_DEBUG("[方法解析] 错误: 无效的类索引 #%d (常量池大小: %d)\n", 
               class_index, class_info->constant_pool.count);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    j2me_constant_pool_entry_t* class_entry = &class_info->constant_pool.entries[class_index - 1];
    if (class_entry->tag != J2ME_CONSTANT_CLASS) {
        LOG_DEBUG("[方法解析] 错误: 常量池条目 #%d 不是类引用 (类型: %d)\n", 
               class_index, class_entry->tag);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    uint16_t class_name_index = class_entry->info.class_info.name_index;
    if (class_name_index == 0 || class_name_index >= class_info->constant_pool.count) {
        LOG_DEBUG("[方法解析] 错误: 无效的类名索引 #%d (常量池大小: %d)\n", 
               class_name_index, class_info->constant_pool.count);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    j2me_constant_pool_entry_t* class_name_entry = &class_info->constant_pool.entries[class_name_index - 1];
    if (class_name_entry->tag != J2ME_CONSTANT_UTF8) {
        LOG_DEBUG("[方法解析] 错误: 类名条目 #%d 不是UTF-8字符串 (类型: %d)\n", 
               class_name_index, class_name_entry->tag);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    *class_name = class_name_entry->info.utf8.bytes;
    
    // 解析方法名和描述符
    if (name_and_type_index == 0 || name_and_type_index >= class_info->constant_pool.count) {
        LOG_DEBUG("[方法解析] 错误: 无效的名称和类型索引 #%d (常量池大小: %d)\n", 
               name_and_type_index, class_info->constant_pool.count);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    j2me_constant_pool_entry_t* name_and_type_entry = &class_info->constant_pool.entries[name_and_type_index - 1];
    if (name_and_type_entry->tag != J2ME_CONSTANT_NAME_AND_TYPE) {
        LOG_DEBUG("[方法解析] 错误: 常量池条目 #%d 不是名称和类型 (类型: %d)\n", 
               name_and_type_index, name_and_type_entry->tag);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    uint16_t method_name_index = name_and_type_entry->info.name_and_type_info.name_index;
    uint16_t descriptor_index = name_and_type_entry->info.name_and_type_info.descriptor_index;
    
    LOG_DEBUG("[方法解析] 名称和类型 #%d: method_name_index=%d, descriptor_index=%d\n", 
           name_and_type_index, method_name_index, descriptor_index);
    
    // 解析方法名
    if (method_name_index == 0 || method_name_index >= class_info->constant_pool.count) {
        LOG_DEBUG("[方法解析] 错误: 无效的方法名索引 #%d (常量池大小: %d)\n", 
               method_name_index, class_info->constant_pool.count);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    j2me_constant_pool_entry_t* method_name_entry = &class_info->constant_pool.entries[method_name_index - 1];
    if (method_name_entry->tag != J2ME_CONSTANT_UTF8) {
        LOG_DEBUG("[方法解析] 错误: 方法名条目 #%d 不是UTF-8字符串 (类型: %d)\n", 
               method_name_index, method_name_entry->tag);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    *method_name = method_name_entry->info.utf8.bytes;
    
    // 解析描述符
    if (descriptor_index == 0 || descriptor_index >= class_info->constant_pool.count) {
        LOG_DEBUG("[方法解析] 错误: 无效的描述符索引 #%d (常量池大小: %d)\n", 
               descriptor_index, class_info->constant_pool.count);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    j2me_constant_pool_entry_t* descriptor_entry = &class_info->constant_pool.entries[descriptor_index - 1];
    if (descriptor_entry->tag != J2ME_CONSTANT_UTF8) {
        LOG_DEBUG("[方法解析] 错误: 描述符条目 #%d 不是UTF-8字符串 (类型: %d)\n", 
               descriptor_index, descriptor_entry->tag);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    *descriptor = descriptor_entry->info.utf8.bytes;
    
    LOG_DEBUG("[方法解析] 成功解析方法引用 #%d: %s.%s%s\n", 
           method_ref_index, *class_name, *method_name, *descriptor);
    
    return J2ME_SUCCESS;
}

j2me_error_t j2me_interpreter_execute_method(j2me_vm_t* vm, j2me_method_t* method, void* object, void* args) {
    if (!vm || !method) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 检查方法是否有字节码
    if (!method->bytecode || method->bytecode_length == 0) {
        return J2ME_SUCCESS;
    }
    
    // 创建栈帧
    j2me_stack_frame_t* frame = j2me_stack_frame_create(method->max_stack, method->max_locals);
    if (!frame) {
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    // 设置字节码和程序计数器
    frame->bytecode = method->bytecode;
    frame->pc = 0;
    frame->method_info = method;
    
    // 如果是实例方法，将this引用放入局部变量0
    int local_var_index = 0;
    if (!(method->access_flags & ACC_STATIC) && object) {
        frame->local_vars.variables[local_var_index++] = (j2me_int)(uintptr_t)object;
    }
    
    // 处理方法参数
    if (method->descriptor && args) {
        j2me_error_t param_result = j2me_interpreter_parse_method_parameters(
            method->descriptor, args, frame, &local_var_index);
    }
    
    // 执行字节码
    j2me_error_t result = J2ME_SUCCESS;
    uint32_t instruction_count = 0;
    const uint32_t max_instructions = 1000000; // 增加到 100 万条指令
    const uint32_t debug_interval = 10000; // 每10000条指令输出一次调试信息
    
    printf("[Interpreter] Starting execution, bytecode_length=%d\n", method->bytecode_length);
    fflush(stdout);
    
    while (frame->pc < method->bytecode_length && instruction_count < max_instructions) {
        result = execute_single_instruction(vm, frame);
        
        if (result != J2ME_SUCCESS) {
            break;
        }
        
        instruction_count++;
        
        // 定期输出调试信息
        if (instruction_count % debug_interval == 0) {
            printf("[Interpreter] Executed %d instructions, pc=%d/%d\n", 
                   instruction_count, frame->pc, method->bytecode_length);
            fflush(stdout);
        }
    }
    
    printf("[Interpreter] Execution finished: instructions=%d, result=%d\n", instruction_count, result);
    fflush(stdout);
    
    if (instruction_count >= max_instructions) {
        printf("[Interpreter] ERROR: Max instructions reached!\n");
        result = J2ME_ERROR_RUNTIME_EXCEPTION;
    }
    
    // 保存返回值到VM（如果有）
    if (frame->has_return_value && vm) {
        vm->last_method_return_value = frame->return_value;
        vm->last_method_has_return_value = true;
    } else if (vm) {
        vm->last_method_has_return_value = false;
    }
    
    // 清理栈帧
    j2me_stack_frame_destroy(frame);
    
    return result;
}

// 线程管理函数实现

j2me_thread_t* j2me_thread_create(uint32_t thread_id) {
    j2me_thread_t* thread = (j2me_thread_t*)malloc(sizeof(j2me_thread_t));
    if (!thread) {
        return NULL;
    }
    
    memset(thread, 0, sizeof(j2me_thread_t));
    thread->thread_id = thread_id;
    thread->is_running = true;
    
    LOG_DEBUG("[线程] 创建线程成功 (ID: %d)\n", thread_id);
    return thread;
}

void j2me_thread_destroy(j2me_thread_t* thread) {
    if (!thread) {
        return;
    }
    
    // 清理所有栈帧
    while (thread->current_frame) {
        j2me_stack_frame_t* frame = thread->current_frame;
        thread->current_frame = frame->previous;
        j2me_stack_frame_destroy(frame);
    }
    
    LOG_DEBUG("[线程] 销毁线程 (ID: %d)\n", thread->thread_id);
    free(thread);
}

j2me_error_t j2me_thread_push_frame(j2me_thread_t* thread, j2me_stack_frame_t* frame) {
    if (!thread || !frame) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 设置栈帧链接
    frame->previous = thread->current_frame;
    thread->current_frame = frame;
    thread->frame_count++;
    
    LOG_DEBUG("[线程] 推入栈帧 (线程ID: %d, 栈帧数: %zu)\n", thread->thread_id, thread->frame_count);
    return J2ME_SUCCESS;
}

j2me_stack_frame_t* j2me_thread_pop_frame(j2me_thread_t* thread) {
    if (!thread || !thread->current_frame) {
        return NULL;
    }
    
    j2me_stack_frame_t* frame = thread->current_frame;
    thread->current_frame = frame->previous;
    thread->frame_count--;
    
    LOG_DEBUG("[线程] 弹出栈帧 (线程ID: %d, 栈帧数: %zu)\n", thread->thread_id, thread->frame_count);
    return frame;
}