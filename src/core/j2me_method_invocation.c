#include "j2me_method_invocation.h"
#include "j2me_vm.h"
#include "j2me_interpreter.h"
#include "j2me_native_methods.h"
#include "j2me_string.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static j2me_heap_object_header_t* j2me_try_get_heap_object(j2me_vm_t* vm, j2me_ref_t ref) {
    if (!vm || !vm->heap || ref == J2ME_NULL_REF) {
        return NULL;
    }
    return j2me_heap_get_object(vm->heap, ref);
}

static bool j2me_stringbuilder_get_value_ref(j2me_vm_t* vm, j2me_ref_t builder_ref, j2me_ref_t* out_value_ref) {
    if (!out_value_ref) {
        return false;
    }
    *out_value_ref = J2ME_NULL_REF;

    j2me_heap_object_header_t* obj = j2me_try_get_heap_object(vm, builder_ref);
    if (!obj) {
        return false;
    }
    if (obj->size < sizeof(void*) + sizeof(j2me_ref_t)) {
        return false;
    }

    uint8_t* base = (uint8_t*)obj->data;
    j2me_ref_t* value_slot = (j2me_ref_t*)(base + sizeof(void*));
    *out_value_ref = *value_slot;
    return true;
}

static bool j2me_stringbuilder_set_value_ref(j2me_vm_t* vm, j2me_ref_t builder_ref, j2me_ref_t value_ref) {
    j2me_heap_object_header_t* obj = j2me_try_get_heap_object(vm, builder_ref);
    if (!obj) {
        return false;
    }
    if (obj->size < sizeof(void*) + sizeof(j2me_ref_t)) {
        return false;
    }

    uint8_t* base = (uint8_t*)obj->data;
    j2me_ref_t* value_slot = (j2me_ref_t*)(base + sizeof(void*));
    *value_slot = value_ref;
    return true;
}

/**
 * @file j2me_method_invocation_simple.c
 * @brief J2ME方法调用系统简化实现
 * 
 * 简化的方法调用机制，用于测试和基本功能
 */

/**
 * @brief 调用虚方法（简化实现）
 * @param vm 虚拟机实例
 * @param caller_frame 调用者栈帧
 * @param method_ref_index 方法引用索引
 * @return 错误码
 */
j2me_error_t j2me_method_invocation_invoke_virtual(
    j2me_vm_t* vm,
    j2me_stack_frame_t* caller_frame,
    uint16_t method_ref_index) {
    
    if (!vm || !caller_frame) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // printf("[方法调用] invokevirtual: 方法引用索引 #%d\n", method_ref_index);
    
    // 获取当前方法信息以访问常量池
    j2me_method_t* current_method = (j2me_method_t*)caller_frame->method_info;
    if (!current_method || !current_method->owner_class) {
        // printf("[方法调用] invokevirtual: 无法获取类信息\n");
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 解析方法引用
    j2me_constant_pool_entry_t* method_ref = 
        &current_method->owner_class->constant_pool.entries[method_ref_index - 1];
    
    if (method_ref->tag != J2ME_CONSTANT_METHODREF) {
        // printf("[方法调用] invokevirtual: 不是方法引用 (类型: %d)\n", method_ref->tag);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 解析类名
    uint16_t class_index = method_ref->info.ref_info.class_index;
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
    
    // 解析方法名和描述符
    uint16_t name_and_type_index = method_ref->info.ref_info.name_and_type_index;
    j2me_constant_pool_entry_t* name_and_type = 
        &current_method->owner_class->constant_pool.entries[name_and_type_index - 1];
    
    const char* method_name = NULL;
    const char* method_descriptor = NULL;
    
    if (name_and_type->tag == J2ME_CONSTANT_NAME_AND_TYPE) {
        j2me_constant_pool_entry_t* name_entry = 
            &current_method->owner_class->constant_pool.entries[name_and_type->info.name_and_type_info.name_index - 1];
        j2me_constant_pool_entry_t* desc_entry = 
            &current_method->owner_class->constant_pool.entries[name_and_type->info.name_and_type_info.descriptor_index - 1];
        
        if (name_entry->tag == J2ME_CONSTANT_UTF8) {
            method_name = name_entry->info.utf8.bytes;
        }
        if (desc_entry->tag == J2ME_CONSTANT_UTF8) {
            method_descriptor = desc_entry->info.utf8.bytes;
        }
    }
    
                // // printf("[方法调用] invokevirtual: %s.%s%s\n", 
                //        class_name ? class_name : "未知类",
                //        method_name ? method_name : "未知方法",
                //        method_descriptor ? method_descriptor : "");
    
    // 特殊处理Display.setCurrent()
    if (class_name && method_name &&
        strcmp(class_name, "javax/microedition/lcdui/Display") == 0 &&
        strcmp(method_name, "setCurrent") == 0) {
        
        printf("[方法调用] Display.setCurrent: 栈深度=%d\n", caller_frame->operand_stack.top);
        
        // 弹出Displayable参数（Canvas）
        j2me_int canvas_ref = 0;
        if (caller_frame->operand_stack.top > 0) {
            j2me_operand_stack_pop(&caller_frame->operand_stack, &canvas_ref);
            printf("[方法调用] Display.setCurrent: 弹出Canvas参数=0x%x\n", canvas_ref);
        }
        
        // 弹出Display的this引用
        j2me_int display_ref = 0;
        if (caller_frame->operand_stack.top > 0) {
            j2me_operand_stack_pop(&caller_frame->operand_stack, &display_ref);
            printf("[方法调用] Display.setCurrent: 弹出Display引用=0x%x\n", display_ref);
        }
        
        // 如果Canvas引用是假引用或0，尝试使用VM中最后创建的Canvas对象
        if (canvas_ref == 0 || canvas_ref == 0x87654321 || canvas_ref == 0x12345678 || canvas_ref == 0x11223344) {
            printf("[方法调用] Display.setCurrent: Canvas引用无效，使用VM中最后创建的Canvas对象\n");
            if (vm && vm->last_canvas_object_ref != 0) {
                canvas_ref = vm->last_canvas_object_ref;
                printf("[方法调用] Display.setCurrent: 使用Canvas对象引用 0x%x\n", canvas_ref);
            }
        }
        
        // 保存当前Canvas到VM
        vm->current_canvas_ref = canvas_ref;
        
        printf("[方法调用] Display.setCurrent: Canvas=0x%x\n", canvas_ref);
        return J2ME_SUCCESS;
    }
    
    // 特殊处理PrintStream.println()
    if (class_name && method_name &&
        strcmp(class_name, "java/io/PrintStream") == 0 &&
        strcmp(method_name, "println") == 0) {
        
        printf("[方法调用] PrintStream.println: 调用本地方法\n");
        
        // 调用本地方法实现
        return java_system_out_println(vm, caller_frame, NULL);
    }
    
    // 特殊处理PrintStream.print()
    if (class_name && method_name &&
        strcmp(class_name, "java/io/PrintStream") == 0 &&
        strcmp(method_name, "print") == 0) {
        
        printf("[方法调用] PrintStream.print: 调用本地方法\n");
        
        // 调用本地方法实现
        return java_system_out_print(vm, caller_frame, NULL);
    }
    
    // 如果是Graphics方法，调用相应的本地方法
    if (class_name && strstr(class_name, "Graphics")) {
        if (method_name && method_descriptor) {
            // 根据方法名调用相应的Graphics本地方法
            if (strcmp(method_name, "setColor") == 0) {
                if (strcmp(method_descriptor, "(III)V") == 0) {
                    return midp_graphics_set_color_rgb(vm, caller_frame, NULL);
                } else if (strcmp(method_descriptor, "(I)V") == 0) {
                    return midp_graphics_set_color(vm, caller_frame, NULL);
                }
            } else if (strcmp(method_name, "drawLine") == 0 && strcmp(method_descriptor, "(IIII)V") == 0) {
                return midp_graphics_draw_line(vm, caller_frame, NULL);
            } else if (strcmp(method_name, "drawRect") == 0 && strcmp(method_descriptor, "(IIII)V") == 0) {
                return midp_graphics_draw_rect(vm, caller_frame, NULL);
            } else if (strcmp(method_name, "fillRect") == 0 && strcmp(method_descriptor, "(IIII)V") == 0) {
                return midp_graphics_fill_rect(vm, caller_frame, NULL);
            } else if (strcmp(method_name, "drawString") == 0 && strcmp(method_descriptor, "(Ljava/lang/String;II)V") == 0) {
                return midp_graphics_draw_string(vm, caller_frame, NULL);
            } else if (strcmp(method_name, "drawOval") == 0 && strcmp(method_descriptor, "(IIII)V") == 0) {
                return midp_graphics_draw_oval(vm, caller_frame, NULL);
            } else if (strcmp(method_name, "fillOval") == 0 && strcmp(method_descriptor, "(IIII)V") == 0) {
                return midp_graphics_fill_oval(vm, caller_frame, NULL);
            } else if (strcmp(method_name, "drawArc") == 0 && strcmp(method_descriptor, "(IIIII)V") == 0) {
                return midp_graphics_draw_arc(vm, caller_frame, NULL);
            } else if (strcmp(method_name, "drawImage") == 0 && strcmp(method_descriptor, "(Ljavax/microedition/lcdui/Image;III)V") == 0) {
                return midp_graphics_draw_image(vm, caller_frame, NULL);
            }
        }
        
        // 未知的Graphics方法，弹出this引用并返回成功
        if (caller_frame->operand_stack.top > 0) {
            j2me_int this_ref;
            j2me_operand_stack_pop(&caller_frame->operand_stack, &this_ref);
            // printf("[方法调用] invokevirtual: 弹出Graphics引用 0x%x\n", this_ref);
        }
        // printf("[方法调用] invokevirtual: Graphics方法调用完成 (简化实现)\n");
        return J2ME_SUCCESS;
    }
    
    // 如果是String方法，调用相应的本地方法
    if (class_name && strcmp(class_name, "java/lang/StringBuilder") == 0) {
        if (method_name && method_descriptor) {
            if (strcmp(method_name, "append") == 0) {
                if (strcmp(method_descriptor, "(Ljava/lang/String;)Ljava/lang/StringBuilder;") == 0) {
                    j2me_int arg_ref_int = 0;
                    j2me_error_t result = j2me_operand_stack_pop(&caller_frame->operand_stack, &arg_ref_int);
                    if (result != J2ME_SUCCESS) {
                        return result;
                    }

                    j2me_int this_ref_int = 0;
                    result = j2me_operand_stack_pop(&caller_frame->operand_stack, &this_ref_int);
                    if (result != J2ME_SUCCESS) {
                        return result;
                    }

                    j2me_ref_t this_ref = (j2me_ref_t)this_ref_int;
                    j2me_ref_t current = J2ME_NULL_REF;
                    j2me_stringbuilder_get_value_ref(vm, this_ref, &current);
                    if (current == J2ME_NULL_REF) {
                        current = j2me_heap_string_create(vm->heap, "");
                    }

                    j2me_ref_t arg_ref = (j2me_ref_t)arg_ref_int;
                    if (arg_ref == J2ME_NULL_REF) {
                        arg_ref = j2me_heap_string_create(vm->heap, "null");
                    }

                    j2me_ref_t combined = j2me_heap_string_concat(vm->heap, current, arg_ref);
                    j2me_stringbuilder_set_value_ref(vm, this_ref, combined);

                    return j2me_operand_stack_push(&caller_frame->operand_stack, (j2me_int)this_ref);
                }

                if (strcmp(method_descriptor, "(I)Ljava/lang/StringBuilder;") == 0) {
                    j2me_int int_value = 0;
                    j2me_error_t result = j2me_operand_stack_pop(&caller_frame->operand_stack, &int_value);
                    if (result != J2ME_SUCCESS) {
                        return result;
                    }

                    j2me_int this_ref_int = 0;
                    result = j2me_operand_stack_pop(&caller_frame->operand_stack, &this_ref_int);
                    if (result != J2ME_SUCCESS) {
                        return result;
                    }

                    char buf[32];
                    snprintf(buf, sizeof(buf), "%d", int_value);
                    j2me_ref_t arg_ref = j2me_heap_string_create(vm->heap, buf);

                    j2me_ref_t this_ref = (j2me_ref_t)this_ref_int;
                    j2me_ref_t current = J2ME_NULL_REF;
                    j2me_stringbuilder_get_value_ref(vm, this_ref, &current);
                    if (current == J2ME_NULL_REF) {
                        current = j2me_heap_string_create(vm->heap, "");
                    }

                    j2me_ref_t combined = j2me_heap_string_concat(vm->heap, current, arg_ref);
                    j2me_stringbuilder_set_value_ref(vm, this_ref, combined);

                    return j2me_operand_stack_push(&caller_frame->operand_stack, (j2me_int)this_ref);
                }
            }

            if (strcmp(method_name, "toString") == 0 && strcmp(method_descriptor, "()Ljava/lang/String;") == 0) {
                j2me_int this_ref_int = 0;
                j2me_error_t result = j2me_operand_stack_pop(&caller_frame->operand_stack, &this_ref_int);
                if (result != J2ME_SUCCESS) {
                    return result;
                }
                j2me_ref_t this_ref = (j2me_ref_t)this_ref_int;
                j2me_ref_t current = J2ME_NULL_REF;
                j2me_stringbuilder_get_value_ref(vm, this_ref, &current);
                if (current == J2ME_NULL_REF) {
                    current = j2me_heap_string_create(vm->heap, "");
                    j2me_stringbuilder_set_value_ref(vm, this_ref, current);
                }
                return j2me_operand_stack_push(&caller_frame->operand_stack, (j2me_int)current);
            }
        }
        return J2ME_SUCCESS;
    }

    if (class_name && strcmp(class_name, "java/lang/String") == 0) {
        if (method_name && method_descriptor) {
            // 弹出this引用
            j2me_int this_ref;
            j2me_error_t result = j2me_operand_stack_pop(&caller_frame->operand_stack, &this_ref);
            if (result != J2ME_SUCCESS) {
                return result;
            }
            
            // 根据方法名调用相应的String本地方法
            if (strcmp(method_name, "length") == 0 && strcmp(method_descriptor, "()I") == 0) {
                // 返回一个假的字符串长度
                j2me_int length = 10; // 假的字符串长度
                result = j2me_operand_stack_push(&caller_frame->operand_stack, length);
                // printf("[方法调用] invokevirtual: String.length() 返回 %d\n", length);
                return result;
            } else if (strcmp(method_name, "charAt") == 0 && strcmp(method_descriptor, "(I)C") == 0) {
                // 弹出索引参数
                j2me_int index;
                result = j2me_operand_stack_pop(&caller_frame->operand_stack, &index);
                if (result == J2ME_SUCCESS) {
                    // 返回一个假的字符
                    j2me_int ch = 'A'; // 假的字符
                    result = j2me_operand_stack_push(&caller_frame->operand_stack, ch);
                    // printf("[方法调用] invokevirtual: String.charAt(%d) 返回 '%c'\n", index, ch);
                }
                return result;
            } else if (strcmp(method_name, "substring") == 0 && strcmp(method_descriptor, "(II)Ljava/lang/String;") == 0) {
                // 弹出endIndex和startIndex参数
                j2me_int end_index, start_index;
                result = j2me_operand_stack_pop(&caller_frame->operand_stack, &end_index);
                if (result == J2ME_SUCCESS) {
                    result = j2me_operand_stack_pop(&caller_frame->operand_stack, &start_index);
                    if (result == J2ME_SUCCESS) {
                        // 返回一个假的字符串引用
                        j2me_int substring_ref = 0x30000001;
                        result = j2me_operand_stack_push(&caller_frame->operand_stack, substring_ref);
                        // printf("[方法调用] invokevirtual: String.substring(%d,%d) 返回 0x%x\n", 
                        //        start_index, end_index, substring_ref);
                    }
                }
                return result;
            }
        }
        
        // 未知的String方法，返回成功
        // printf("[方法调用] invokevirtual: String方法调用完成 (简化实现)\n");
        return J2ME_SUCCESS;
    }
    
    // 其他方法，尝试查找并执行
    // 首先弹出this引用
    j2me_int this_ref = 0;
    if (caller_frame->operand_stack.top > 0) {
        j2me_error_t pop_result = j2me_operand_stack_pop(&caller_frame->operand_stack, &this_ref);
        if (pop_result != J2ME_SUCCESS) {
            return pop_result;
        }
    }
    
    // 解析方法描述符，确定参数数量（不包括this）
    int param_count = 0;
    if (method_descriptor) {
        const char* p = strchr(method_descriptor, '(');
        if (p) {
            p++; // 跳过'('
            while (*p && *p != ')') {
                switch (*p) {
                    case 'I': case 'Z': case 'B': case 'C': case 'S': case 'F':
                        param_count++;
                        p++;
                        break;
                    case 'J': case 'D': // long和double占用两个槽位
                        param_count += 2;
                        p++;
                        break;
                    case 'L': // 对象引用
                        param_count++;
                        while (*p && *p != ';') p++;
                        if (*p == ';') p++;
                        break;
                    case '[': // 数组
                        param_count++;
                        p++;
                        while (*p == '[') p++;
                        if (*p == 'L') {
                            while (*p && *p != ';') p++;
                            if (*p == ';') p++;
                        } else {
                            p++;
                        }
                        break;
                    default:
                        p++;
                        break;
                }
            }
        }
    }
    
    // 从调用者栈中弹出参数
    j2me_int* args = NULL;
    if (param_count > 0) {
        args = (j2me_int*)malloc(sizeof(j2me_int) * param_count);
        if (args) {
            // 从栈中弹出参数（注意顺序：最后一个参数在栈顶）
            for (int i = param_count - 1; i >= 0; i--) {
                if (j2me_operand_stack_pop(&caller_frame->operand_stack, &args[i]) != J2ME_SUCCESS) {
                    printf("[方法调用] invokevirtual: 警告：弹出参数失败\n");
                    args[i] = 0;
                }
            }
            printf("[方法调用] invokevirtual: 从栈弹出%d个参数\n", param_count);
        }
    }
    
    // 查找并调用方法
    j2me_class_t* target_class = j2me_class_loader_find_class(vm->class_loader, class_name);
    if (!target_class) {
        printf("[方法调用] invokevirtual: 类未加载，尝试加载类 %s\n", class_name);
        target_class = j2me_class_loader_load_class(vm->class_loader, class_name);
    }
    
    if (target_class) {
        printf("[方法调用] invokevirtual: 找到类 %s\n", class_name);
        j2me_method_t* target_method = j2me_class_find_method(target_class, method_name, method_descriptor);
        if (target_method) {
            printf("[方法调用] invokevirtual: 找到方法 %s%s，开始执行\n", method_name, method_descriptor);
            j2me_error_t exec_result = j2me_interpreter_execute_method(vm, target_method, (void*)(intptr_t)this_ref, args);
            
            // 释放参数数组
            if (args) {
                free(args);
            }
            
            if (exec_result != J2ME_SUCCESS) {
                printf("[方法调用] invokevirtual: 方法执行失败 (错误: %d)\n", exec_result);
                return exec_result;
            }
            return exec_result;
        } else {
            printf("[方法调用] invokevirtual: 未找到方法 %s%s\n", method_name, method_descriptor);
        }
    } else {
        printf("[方法调用] invokevirtual: 无法加载类 %s\n", class_name);
    }
    
    // 释放参数数组
    if (args) {
        free(args);
    }
    
    printf("[方法调用] invokevirtual: 方法调用完成 (简化实现)\n");
    return J2ME_SUCCESS;
}

/**
 * @brief 调用静态方法（简化实现）
 * @param vm 虚拟机实例
 * @param caller_frame 调用者栈帧
 * @param method_ref_index 方法引用索引
 * @return 错误码
 */
j2me_error_t j2me_method_invocation_invoke_static(
    j2me_vm_t* vm,
    j2me_stack_frame_t* caller_frame,
    uint16_t method_ref_index) {
    
    if (!vm || !caller_frame) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 获取当前方法信息以访问常量池
    j2me_method_t* current_method = (j2me_method_t*)caller_frame->method_info;
    if (!current_method || !current_method->owner_class) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 解析方法引用
    j2me_constant_pool_entry_t* method_ref = 
        &current_method->owner_class->constant_pool.entries[method_ref_index - 1];
    
    if (method_ref->tag != J2ME_CONSTANT_METHODREF) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 解析类名
    uint16_t class_index = method_ref->info.ref_info.class_index;
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
    
    // 解析方法名和描述符
    uint16_t name_and_type_index = method_ref->info.ref_info.name_and_type_index;
    j2me_constant_pool_entry_t* name_and_type = 
        &current_method->owner_class->constant_pool.entries[name_and_type_index - 1];
    
    const char* method_name = NULL;
    const char* method_descriptor = NULL;
    
    if (name_and_type->tag == J2ME_CONSTANT_NAME_AND_TYPE) {
        j2me_constant_pool_entry_t* name_entry = 
            &current_method->owner_class->constant_pool.entries[name_and_type->info.name_and_type_info.name_index - 1];
        j2me_constant_pool_entry_t* desc_entry = 
            &current_method->owner_class->constant_pool.entries[name_and_type->info.name_and_type_info.descriptor_index - 1];
        
        if (name_entry->tag == J2ME_CONSTANT_UTF8) {
            method_name = name_entry->info.utf8.bytes;
        }
        if (desc_entry->tag == J2ME_CONSTANT_UTF8) {
            method_descriptor = desc_entry->info.utf8.bytes;
        }
    }
    
    // printf("[方法调用] invokestatic: %s.%s%s\n", 
    //        class_name ? class_name : "未知类",
    //        method_name ? method_name : "未知方法",
    //        method_descriptor ? method_descriptor : "");
    
    // 特殊处理Display.getDisplay()
    if (class_name && method_name && 
        strcmp(class_name, "javax/microedition/lcdui/Display") == 0 &&
        strcmp(method_name, "getDisplay") == 0) {
        
        // 弹出MIDlet参数
        if (caller_frame->operand_stack.top > 0) {
            j2me_int midlet_ref;
            j2me_operand_stack_pop(&caller_frame->operand_stack, &midlet_ref);
        }
        
        // 返回Display对象引用（使用vm->display的地址）
        if (vm->display) {
            j2me_operand_stack_push(&caller_frame->operand_stack, (j2me_int)(uintptr_t)vm->display);
            return J2ME_SUCCESS;
        }
    }
    
    // 查找并调用静态方法，如果类未找到则尝试加载
    j2me_class_t* target_class = j2me_class_loader_find_class(vm->class_loader, class_name);
    if (!target_class) {
        printf("[方法调用] invokestatic: 类未加载，尝试加载类 %s\n", class_name);
        target_class = j2me_class_loader_load_class(vm->class_loader, class_name);
    }
    
    if (target_class) {
        // 确保类已初始化
        if (target_class->state != CLASS_INITIALIZED) {
            printf("[方法调用] invokestatic: 类未初始化，执行初始化 %s\n", class_name);
            j2me_error_t init_result = j2me_class_initialize(target_class);
            if (init_result != J2ME_SUCCESS) {
                printf("[方法调用] invokestatic: 类初始化失败: %d\n", init_result);
            }
        }
        
        printf("[方法调用] invokestatic: 找到类 %s\n", class_name);
        j2me_method_t* target_method = j2me_class_find_method(target_class, method_name, method_descriptor);
        if (target_method) {
            printf("[方法调用] invokestatic: 找到方法 %s%s，开始执行\n", method_name, method_descriptor);
            
            // 解析方法描述符，确定参数数量
            int param_count = 0;
            if (method_descriptor) {
                const char* p = strchr(method_descriptor, '(');
                if (p) {
                    p++; // 跳过'('
                    while (*p && *p != ')') {
                        switch (*p) {
                            case 'I': case 'Z': case 'B': case 'C': case 'S': case 'F':
                                param_count++;
                                p++;
                                break;
                            case 'J': case 'D': // long和double占用两个槽位
                                param_count += 2;
                                p++;
                                break;
                            case 'L': // 对象引用
                                param_count++;
                                while (*p && *p != ';') p++;
                                if (*p == ';') p++;
                                break;
                            case '[': // 数组
                                param_count++;
                                p++;
                                while (*p == '[') p++;
                                if (*p == 'L') {
                                    while (*p && *p != ';') p++;
                                    if (*p == ';') p++;
                                } else {
                                    p++;
                                }
                                break;
                            default:
                                p++;
                                break;
                        }
                    }
                }
            }
            
            // 从调用者栈中弹出参数
            j2me_int* args = NULL;
            if (param_count > 0) {
                args = (j2me_int*)malloc(sizeof(j2me_int) * param_count);
                if (args) {
                    // 从栈中弹出参数（注意顺序：最后一个参数在栈顶）
                    for (int i = param_count - 1; i >= 0; i--) {
                        if (j2me_operand_stack_pop(&caller_frame->operand_stack, &args[i]) != J2ME_SUCCESS) {
                            printf("[方法调用] invokestatic: 警告：弹出参数失败\n");
                            args[i] = 0;
                        }
                    }
                    printf("[方法调用] invokestatic: 从栈弹出%d个参数\n", param_count);
                }
            }
            
            j2me_error_t result = j2me_interpreter_execute_method(vm, target_method, NULL, args);
            
            // 释放参数数组
            if (args) {
                free(args);
            }
            
            // 检查方法是否有返回值（从VM中获取）
            if (result == J2ME_SUCCESS && vm->last_method_has_return_value) {
                j2me_int return_value = vm->last_method_return_value;
                printf("[方法调用] invokestatic: 方法返回值 0x%x\n", return_value);
                
                // 注意：不要在这里压栈！解释器会自动处理返回值压栈
                // 只需要检查是否是Canvas对象并保存到VM
                
                // 如果返回值看起来是对象引用（非0且不是小整数），保存到VM
                if (return_value > 0 && return_value < 0x1000) {
                    // 检查是否是y类的静态方法返回的对象（Canvas子类）
                    if (strcmp(class_name, "y") == 0) {
                        vm->last_canvas_object_ref = return_value;
                        printf("[方法调用] invokestatic: 保存y类对象引用到VM: 0x%x\n", return_value);
                    }
                }
            }
            
            if (result != J2ME_SUCCESS) {
                printf("[方法调用] invokestatic: 方法执行失败 (错误: %d)\n", result);
                return result;
            }
            return result;
        } else {
            printf("[方法调用] invokestatic: 未找到方法 %s%s\n", method_name, method_descriptor);
        }
    } else {
        printf("[方法调用] invokestatic: 未找到类 %s\n", class_name);
    }
    
    // 方法未找到，返回成功（简化处理）
    printf("[方法调用] invokestatic: 方法调用完成 (简化实现)\n");
    return J2ME_SUCCESS;
}

/**
 * @brief 调用特殊方法（简化实现）
 * @param vm 虚拟机实例
 * @param caller_frame 调用者栈帧
 * @param method_ref_index 方法引用索引
 * @return 错误码
 */
j2me_error_t j2me_method_invocation_invoke_special(
    j2me_vm_t* vm,
    j2me_stack_frame_t* caller_frame,
    uint16_t method_ref_index) {
    
    if (!vm || !caller_frame) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // printf("[方法调用] invokespecial: 方法引用索引 #%d\n", method_ref_index);
    
    // 获取当前方法信息以访问常量池
    j2me_method_t* current_method = (j2me_method_t*)caller_frame->method_info;
    if (!current_method || !current_method->owner_class) {
        // printf("[方法调用] invokespecial: 无法获取类信息\n");
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 解析方法引用
    j2me_constant_pool_entry_t* method_ref = 
        &current_method->owner_class->constant_pool.entries[method_ref_index - 1];
    
    if (method_ref->tag != J2ME_CONSTANT_METHODREF) {
        // printf("[方法调用] invokespecial: 不是方法引用 (类型: %d)\n", method_ref->tag);
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 解析类名
    uint16_t class_index = method_ref->info.ref_info.class_index;
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
    
    // 解析方法名和描述符
    uint16_t name_and_type_index = method_ref->info.ref_info.name_and_type_index;
    j2me_constant_pool_entry_t* name_and_type = 
        &current_method->owner_class->constant_pool.entries[name_and_type_index - 1];
    
    const char* method_name = NULL;
    const char* method_descriptor = NULL;
    
    if (name_and_type->tag == J2ME_CONSTANT_NAME_AND_TYPE) {
        j2me_constant_pool_entry_t* name_entry = 
            &current_method->owner_class->constant_pool.entries[name_and_type->info.name_and_type_info.name_index - 1];
        j2me_constant_pool_entry_t* desc_entry = 
            &current_method->owner_class->constant_pool.entries[name_and_type->info.name_and_type_info.descriptor_index - 1];
        
        if (name_entry->tag == J2ME_CONSTANT_UTF8) {
            method_name = name_entry->info.utf8.bytes;
        }
        if (desc_entry->tag == J2ME_CONSTANT_UTF8) {
            method_descriptor = desc_entry->info.utf8.bytes;
        }
    }
    
    // printf("[方法调用] invokespecial: %s.%s%s\n", 
    //        class_name ? class_name : "未知类",
    //        method_name ? method_name : "未知方法",
    //        method_descriptor ? method_descriptor : "");
    
    // 特殊处理Thread.<init>
    if (class_name && method_name &&
        strcmp(class_name, "java/lang/Thread") == 0 &&
        strcmp(method_name, "<init>") == 0) {
        
        // 弹出Runnable参数（如果有）
        j2me_int runnable_ref = 0;
        if (caller_frame->operand_stack.top > 0 && 
            method_descriptor && strstr(method_descriptor, "Runnable")) {
            j2me_operand_stack_pop(&caller_frame->operand_stack, &runnable_ref);
            printf("[方法调用] Thread.<init>: Runnable=0x%x (从栈)\n", runnable_ref);
            
            // 如果从栈弹出的是0，尝试使用VM中保存的最后一个对象引用
            if (runnable_ref == 0 && vm && vm->last_method_has_return_value) {
                runnable_ref = vm->last_method_return_value;
                printf("[方法调用] Thread.<init>: 使用VM中的对象引用 0x%x\n", runnable_ref);
            }
            
            // 保存Runnable引用到VM的专用字段，以便Thread.start()使用
            if (vm && runnable_ref != 0) {
                vm->current_runnable_ref = runnable_ref;
                printf("[方法调用] Thread.<init>: 保存Runnable到VM (0x%x)\n", runnable_ref);
            }
        }
        
        // 弹出Thread的this引用
        if (caller_frame->operand_stack.top > 0) {
            j2me_int thread_ref;
            j2me_operand_stack_pop(&caller_frame->operand_stack, &thread_ref);
        }
        
        printf("[方法调用] Thread.<init>: 线程初始化完成\n");
        return J2ME_SUCCESS;
    }

    if (class_name && method_name &&
        strcmp(class_name, "java/lang/StringBuilder") == 0 &&
        strcmp(method_name, "<init>") == 0) {
        if (method_descriptor && strcmp(method_descriptor, "()V") == 0) {
            j2me_int this_ref_int = 0;
            j2me_error_t result = j2me_operand_stack_pop(&caller_frame->operand_stack, &this_ref_int);
            if (result != J2ME_SUCCESS) {
                return result;
            }
            j2me_ref_t empty = j2me_heap_string_create(vm->heap, "");
            j2me_stringbuilder_set_value_ref(vm, (j2me_ref_t)this_ref_int, empty);
            return J2ME_SUCCESS;
        }

        if (method_descriptor && strcmp(method_descriptor, "(Ljava/lang/String;)V") == 0) {
            j2me_int arg_ref_int = 0;
            j2me_error_t result = j2me_operand_stack_pop(&caller_frame->operand_stack, &arg_ref_int);
            if (result != J2ME_SUCCESS) {
                return result;
            }
            j2me_int this_ref_int = 0;
            result = j2me_operand_stack_pop(&caller_frame->operand_stack, &this_ref_int);
            if (result != J2ME_SUCCESS) {
                return result;
            }
            j2me_ref_t initial = (j2me_ref_t)arg_ref_int;
            if (initial == J2ME_NULL_REF) {
                initial = j2me_heap_string_create(vm->heap, "null");
            }
            j2me_stringbuilder_set_value_ref(vm, (j2me_ref_t)this_ref_int, initial);
            return J2ME_SUCCESS;
        }

        j2me_int this_ref_int = 0;
        j2me_error_t result = j2me_operand_stack_pop(&caller_frame->operand_stack, &this_ref_int);
        if (result != J2ME_SUCCESS) {
            return result;
        }
        j2me_ref_t empty = j2me_heap_string_create(vm->heap, "");
        j2me_stringbuilder_set_value_ref(vm, (j2me_ref_t)this_ref_int, empty);
        return J2ME_SUCCESS;
    }
    
    // 弹出this引用 (如果栈不为空)
    j2me_int this_ref = 0;
    if (caller_frame->operand_stack.top > 0) {
        j2me_error_t result = j2me_operand_stack_pop(&caller_frame->operand_stack, &this_ref);
    }
    
    // 解析方法描述符，确定参数数量（不包括this）
    int param_count = 0;
    if (method_descriptor) {
        const char* p = strchr(method_descriptor, '(');
        if (p) {
            p++; // 跳过'('
            while (*p && *p != ')') {
                switch (*p) {
                    case 'I': case 'Z': case 'B': case 'C': case 'S': case 'F':
                        param_count++;
                        p++;
                        break;
                    case 'J': case 'D': // long和double占用两个槽位
                        param_count += 2;
                        p++;
                        break;
                    case 'L': // 对象引用
                        param_count++;
                        while (*p && *p != ';') p++;
                        if (*p == ';') p++;
                        break;
                    case '[': // 数组
                        param_count++;
                        p++;
                        while (*p == '[') p++;
                        if (*p == 'L') {
                            while (*p && *p != ';') p++;
                            if (*p == ';') p++;
                        } else {
                            p++;
                        }
                        break;
                    default:
                        p++;
                        break;
                }
            }
        }
    }
    
    // 从调用者栈中弹出参数
    j2me_int* args = NULL;
    if (param_count > 0) {
        args = (j2me_int*)malloc(sizeof(j2me_int) * param_count);
        if (args) {
            // 从栈中弹出参数（注意顺序：最后一个参数在栈顶）
            for (int i = param_count - 1; i >= 0; i--) {
                if (j2me_operand_stack_pop(&caller_frame->operand_stack, &args[i]) != J2ME_SUCCESS) {
                    printf("[方法调用] invokespecial: 警告：弹出参数失败\n");
                    args[i] = 0;
                }
            }
            printf("[方法调用] invokespecial: 从栈弹出%d个参数\n", param_count);
        }
    }
    
    // 查找并调用方法，如果类未找到则尝试加载
    j2me_class_t* target_class = j2me_class_loader_find_class(vm->class_loader, class_name);
    if (!target_class) {
        // 特殊处理java/lang/Object - 如果找不到就跳过
        if (class_name && strcmp(class_name, "java/lang/Object") == 0) {
            printf("[方法调用] invokespecial: java/lang/Object.<init> - 跳过\n");
            if (args) {
                free(args);
            }
            return J2ME_SUCCESS;
        }
        
        printf("[方法调用] invokespecial: 类未加载，尝试加载类 %s\n", class_name);
        target_class = j2me_class_loader_load_class(vm->class_loader, class_name);
    }
    
    if (target_class) {
        printf("[方法调用] invokespecial: 找到类 %s\n", class_name);
        j2me_method_t* target_method = j2me_class_find_method(target_class, method_name, method_descriptor);
        if (target_method) {
            printf("[方法调用] invokespecial: 找到方法，开始执行\n");
            j2me_error_t result = j2me_interpreter_execute_method(vm, target_method, (void*)(intptr_t)this_ref, args);
            
            // 释放参数数组
            if (args) {
                free(args);
            }
            
            if (result != J2ME_SUCCESS) {
                printf("[方法调用] invokespecial: 方法执行失败 (错误: %d)\n", result);
                return result;
            }
            return result;
        } else {
            printf("[方法调用] invokespecial: 未找到方法 %s%s\n", method_name, method_descriptor);
        }
    } else {
        printf("[方法调用] invokespecial: 无法加载类 %s\n", class_name);
    }
    
    // 释放参数数组
    if (args) {
        free(args);
    }
    
    printf("[方法调用] invokespecial: 方法调用完成 (简化实现)\n");
    return J2ME_SUCCESS;
}

/**
 * @brief 调用接口方法（简化实现）
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
    uint8_t count) {
    
    if (!vm || !caller_frame) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    printf("[方法调用] invokeinterface: 方法引用索引 #%d, 参数数量 %d (简化实现)\n", 
           method_ref_index, count);
    
    // 弹出this引用
    if (caller_frame->operand_stack.top > 0) {
        j2me_int this_ref;
        j2me_error_t result = j2me_operand_stack_pop(&caller_frame->operand_stack, &this_ref);
        if (result == J2ME_SUCCESS) {
            printf("[方法调用] invokeinterface: 弹出this引用 0x%x\n", this_ref);
        }
    }
    
    // 弹出其他参数
    for (int i = 1; i < count && caller_frame->operand_stack.top > 0; i++) {
        j2me_int param;
        j2me_operand_stack_pop(&caller_frame->operand_stack, &param);
        printf("[方法调用] invokeinterface: 弹出参数[%d] = 0x%x\n", i, param);
    }
    
    printf("[方法调用] invokeinterface: 接口方法调用完成 (简化实现)\n");
    return J2ME_SUCCESS;
}

// 其他函数的简化实现（暂时返回未实现错误）

j2me_method_invocation_context_t* j2me_method_invocation_create_context(
    j2me_vm_t* vm,
    j2me_stack_frame_t* caller_frame,
    j2me_method_t* method,
    j2me_value_t* args,
    int arg_count) {
    
    printf("[方法调用] 创建调用上下文 (简化实现)\n");
    return NULL; // 简化实现
}

void j2me_method_invocation_destroy_context(j2me_method_invocation_context_t* context) {
    printf("[方法调用] 销毁调用上下文 (简化实现)\n");
    // 简化实现：什么都不做
}

j2me_error_t j2me_method_invocation_resolve_method_ref(
    j2me_vm_t* vm,
    j2me_class_t* class_info,
    uint16_t method_ref_index,
    j2me_method_t** resolved_method) {
    
    printf("[方法调用] 解析方法引用 (简化实现)\n");
    return J2ME_ERROR_NOT_IMPLEMENTED;
}

j2me_error_t j2me_method_invocation_prepare_args(
    j2me_method_invocation_context_t* context,
    j2me_operand_stack_t* caller_stack) {
    
    printf("[方法调用] 准备参数 (简化实现)\n");
    return J2ME_ERROR_NOT_IMPLEMENTED;
}

j2me_stack_frame_t* j2me_method_invocation_create_frame(j2me_method_invocation_context_t* context) {
    printf("[方法调用] 创建栈帧 (简化实现)\n");
    return NULL;
}

j2me_error_t j2me_method_invocation_execute(j2me_method_invocation_context_t* context) {
    printf("[方法调用] 执行方法 (简化实现)\n");
    return J2ME_ERROR_NOT_IMPLEMENTED;
}
