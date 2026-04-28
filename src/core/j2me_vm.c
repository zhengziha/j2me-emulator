#include "j2me_vm.h"
#include "j2me_interpreter.h"
#include "j2me_class.h"
#include "j2me_native_methods.h"
#include "j2me_graphics.h"
#include "j2me_input.h"
#include "j2me_gc.h"
#include "j2me_object.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @file j2me_vm.c
 * @brief J2ME虚拟机核心实现
 * 
 * 实现虚拟机的创建、初始化、执行和销毁功能
 */

// 默认配置常量
#define DEFAULT_HEAP_SIZE       (1024 * 1024)  // 1MB
#define DEFAULT_STACK_SIZE      (64 * 1024)    // 64KB
#define DEFAULT_MAX_THREADS     16

j2me_vm_config_t j2me_vm_get_default_config(void) {
    j2me_vm_config_t config = {
        .heap_size = DEFAULT_HEAP_SIZE,
        .stack_size = DEFAULT_STACK_SIZE,
        .max_threads = DEFAULT_MAX_THREADS,
        .enable_gc = true,
        .enable_jit = false  // 暂时禁用JIT
    };
    return config;
}

j2me_vm_t* j2me_vm_create(const j2me_vm_config_t* config) {
    if (!config) {
        return NULL;
    }
    
    // 分配虚拟机结构
    j2me_vm_t* vm = (j2me_vm_t*)malloc(sizeof(j2me_vm_t));
    if (!vm) {
        return NULL;
    }
    
    // 初始化基本字段
    memset(vm, 0, sizeof(j2me_vm_t));
    vm->state = J2ME_VM_UNINITIALIZED;
    vm->config = *config;
    vm->current_canvas_ref = 0; // 初始化Canvas引用为0
    vm->last_canvas_object_ref = 0; // 初始化最后创建的Canvas对象引用为0
    
    // 分配堆内存（旧系统，保留兼容）
    vm->heap_start = malloc(config->heap_size);
    if (!vm->heap_start) {
        free(vm);
        return NULL;
    }
    
    vm->heap_end = (char*)vm->heap_start + config->heap_size;
    vm->heap_current = vm->heap_start;
    
    // 创建新的对象堆系统
    vm->heap = j2me_heap_create(config->heap_size);
    if (!vm->heap) {
        printf("[VM] 错误: 对象堆创建失败\n");
        free(vm->heap_start);
        free(vm);
        return NULL;
    }
    
    // 创建垃圾回收器
    vm->gc = j2me_gc_create(vm, vm->heap_start, config->heap_size);
    if (!vm->gc) {
        j2me_heap_destroy(vm->heap);
        free(vm->heap_start);
        free(vm);
        return NULL;
    }
    
    printf("[VM] 虚拟机创建成功，堆大小: %zu bytes\n", config->heap_size);
    return vm;
}

void j2me_vm_destroy(j2me_vm_t* vm) {
    if (!vm) {
        return;
    }
    
    // 停止虚拟机
    if (vm->state == J2ME_VM_RUNNING) {
        j2me_vm_stop(vm);
    }
    
    // 销毁垃圾回收器
    if (vm->gc) {
        j2me_gc_destroy(vm->gc);
        vm->gc = NULL;
    }
    
    // 销毁对象堆
    if (vm->heap) {
        j2me_heap_destroy(vm->heap);
        vm->heap = NULL;
    }
    
    // 销毁线程
    if (vm->main_thread) {
        j2me_thread_destroy(vm->main_thread);
        vm->main_thread = NULL;
        vm->current_thread = NULL;
    }
    
    // 销毁输入管理器
    if (vm->input_manager) {
        j2me_input_manager_destroy(vm->input_manager);
        vm->input_manager = NULL;
    }
    
    // 销毁显示系统
    if (vm->display) {
        j2me_display_destroy((j2me_display_t*)vm->display);
        vm->display = NULL;
    }
    
    // 销毁类加载器
    if (vm->class_loader) {
        j2me_class_loader_destroy((j2me_class_loader_t*)vm->class_loader);
    }
    
    // 释放堆内存
    if (vm->heap_start) {
        free(vm->heap_start);
    }
    
    // 释放虚拟机结构
    free(vm);
    printf("[VM] 虚拟机已销毁\n");
}

j2me_error_t j2me_vm_initialize(j2me_vm_t* vm) {
    if (!vm || vm->state != J2ME_VM_UNINITIALIZED) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    vm->state = J2ME_VM_INITIALIZING;
    
    // 创建显示系统（如果还没有创建）
    if (!vm->display) {
        vm->display = j2me_display_initialize(240, 320, "J2ME Emulator");
        if (!vm->display) {
            printf("[VM] 错误: 显示系统初始化失败\n");
            return J2ME_ERROR_INITIALIZATION_FAILED;
        }
        
        // 创建图形上下文
        j2me_graphics_context_t* context = j2me_graphics_create_context((j2me_display_t*)vm->display, 240, 320);
        if (!context) {
            printf("[VM] 错误: 图形上下文创建失败\n");
            j2me_display_destroy((j2me_display_t*)vm->display);
            vm->display = NULL;
            return J2ME_ERROR_INITIALIZATION_FAILED;
        }
    } else {
        printf("[VM] 显示系统已存在，跳过创建\n");
    }
    
    // 创建输入管理器
    printf("[VM] 开始创建输入管理器...\n");
    vm->input_manager = j2me_input_manager_create();
    if (!vm->input_manager) {
        printf("[VM] 错误: 输入管理器创建失败\n");
        j2me_display_destroy((j2me_display_t*)vm->display);
        vm->display = NULL;
        return J2ME_ERROR_INITIALIZATION_FAILED;
    }
    printf("[VM] 输入管理器创建成功\n");
    
    // 设置输入事件回调
    j2me_input_set_key_callback(vm->input_manager, j2me_vm_key_event_handler, vm);
    j2me_input_set_pointer_callback(vm->input_manager, j2me_vm_pointer_event_handler, vm);
    
    printf("[VM] 输入事件回调设置成功\n");
    
    // 创建类加载器
    printf("[VM] 开始创建类加载器...\n");
    vm->class_loader = j2me_class_loader_create(vm, ".");
    if (!vm->class_loader) {
        printf("[VM] 错误: 类加载器创建失败\n");
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    printf("[VM] 类加载器创建成功\n");
    
    // 初始化本地方法注册表
    j2me_error_t result = j2me_midp_native_methods_init(vm);
    if (result != J2ME_SUCCESS) {
        printf("[VM] 错误: 本地方法初始化失败: %d\n", result);
        return result;
    }
    
    // 初始化主线程
    vm->main_thread = j2me_thread_create(1); // 线程ID为1
    if (!vm->main_thread) {
        printf("[VM] 错误: 主线程创建失败\n");
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    vm->current_thread = vm->main_thread;
    vm->thread_list = vm->main_thread;
    vm->next_thread_id = 2;
    vm->thread_count = 1;
    printf("[VM] 主线程创建成功 (ID: %d)\n", vm->main_thread->thread_id);
    
    vm->state = J2ME_VM_RUNNING;
    printf("[VM] 虚拟机初始化完成\n");
    return J2ME_SUCCESS;
}

j2me_error_t j2me_vm_start(j2me_vm_t* vm, const char* main_class) {
    if (!vm || !main_class) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    if (vm->state != J2ME_VM_RUNNING) {
        j2me_error_t result = j2me_vm_initialize(vm);
        if (result != J2ME_SUCCESS) {
            return result;
        }
    }
    
    printf("[VM] 启动主类: %s\n", main_class);
    
    // 加载主类
    j2me_class_t* main_class_ptr = j2me_class_loader_load_class(
        (j2me_class_loader_t*)vm->class_loader, main_class);
    if (!main_class_ptr) {
        printf("[VM] 错误: 无法加载主类 %s\n", main_class);
        return J2ME_ERROR_CLASS_NOT_FOUND;
    }
    
    // 链接主类
    j2me_error_t result = j2me_class_link(main_class_ptr);
    if (result != J2ME_SUCCESS) {
        printf("[VM] 错误: 主类链接失败 %s\n", main_class);
        return result;
    }
    
    // 初始化主类
    result = j2me_class_initialize(main_class_ptr);
    if (result != J2ME_SUCCESS) {
        printf("[VM] 错误: 主类初始化失败 %s\n", main_class);
        return result;
    }
    
    // 查找main方法
    j2me_method_t* main_method = j2me_class_find_method(main_class_ptr, "main", "([Ljava/lang/String;)V");
    if (!main_method) {
        printf("[VM] 错误: 找不到main方法\n");
        return J2ME_ERROR_METHOD_NOT_FOUND;
    }
    
    printf("[VM] 找到main方法，开始执行\n");
    
    // 创建主线程的栈帧并执行main方法
    if (vm->main_thread) {
        j2me_stack_frame_t* main_frame = j2me_stack_frame_create(main_method->max_stack, main_method->max_locals);
        if (!main_frame) {
            printf("[VM] 错误: 主方法栈帧创建失败\n");
            return J2ME_ERROR_OUT_OF_MEMORY;
        }
        
        // 设置栈帧信息
        main_frame->bytecode = main_method->bytecode;
        main_frame->pc = 0;
        main_frame->method_info = main_method;
        
        // 将栈帧推入线程
        j2me_thread_push_frame(vm->main_thread, main_frame);
        
        printf("[VM] 主线程栈帧已设置，开始执行main方法\n");
        
        // 执行一些初始指令来启动程序
        j2me_error_t exec_result = j2me_interpreter_execute_batch(vm, vm->main_thread, 100);
        if (exec_result != J2ME_SUCCESS) {
            printf("[VM] 警告: main方法初始执行失败: %d\n", exec_result);
        }
    }
    
    return J2ME_SUCCESS;
}

void j2me_vm_stop(j2me_vm_t* vm) {
    if (!vm) {
        return;
    }
    
    vm->state = J2ME_VM_TERMINATED;
    printf("[VM] 虚拟机已停止\n");
}

j2me_error_t j2me_vm_execute_time_slice(j2me_vm_t* vm, uint32_t time_slice) {
    if (!vm || vm->state != J2ME_VM_RUNNING) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 简单的时间片执行逻辑
    uint32_t instructions_to_execute = time_slice * 1000; // 假设每毫秒执行1000条指令
    
    if (vm->current_thread) {
        j2me_error_t result = j2me_interpreter_execute_batch(vm, vm->current_thread, instructions_to_execute);
        if (result != J2ME_SUCCESS) {
            return result;
        }
        
        vm->instructions_executed += instructions_to_execute;
    }
    
    return J2ME_SUCCESS;
}

/**
 * @brief 键盘事件处理回调
 * @param event 键盘事件
 * @param user_data 用户数据 (虚拟机实例)
 */
void j2me_vm_key_event_handler(j2me_key_event_t* event, void* user_data) {
    j2me_vm_t* vm = (j2me_vm_t*)user_data;
    if (!vm || !event) {
        return;
    }
    
    printf("[VM事件] 键盘事件: 类型=%d, 键码=%d, 字符='%c'\n", 
           event->type, event->key_code, event->key_char ? event->key_char : '?');
    
    // TODO: 调用当前Canvas的keyPressed/keyReleased方法
    // 这里需要找到当前活动的Canvas对象并调用相应的事件方法
    
    // 创建临时栈帧用于方法调用
    j2me_stack_frame_t* frame = j2me_stack_frame_create(10, 5);
    if (!frame) {
        printf("[VM事件] 错误: 创建栈帧失败\n");
        return;
    }
    
    // 模拟Canvas对象引用
    j2me_int canvas_ref = 0x30000001; // 假的Canvas对象引用
    
    // 压入参数
    j2me_operand_stack_push(&frame->operand_stack, canvas_ref);
    j2me_operand_stack_push(&frame->operand_stack, event->key_code);
    
    // 调用相应的Canvas事件方法
    j2me_error_t result;
    if (event->type == INPUT_EVENT_KEY_PRESSED) {
        result = midp_canvas_key_pressed(vm, frame, NULL);
        if (result == J2ME_SUCCESS) {
            printf("[VM事件] Canvas.keyPressed() 调用成功\n");
        }
    } else if (event->type == INPUT_EVENT_KEY_RELEASED) {
        result = midp_canvas_key_released(vm, frame, NULL);
        if (result == J2ME_SUCCESS) {
            printf("[VM事件] Canvas.keyReleased() 调用成功\n");
        }
    }
    
    // 清理栈帧
    j2me_stack_frame_destroy(frame);
}

/**
 * @brief 指针事件处理回调
 * @param event 指针事件
 * @param user_data 用户数据 (虚拟机实例)
 */
void j2me_vm_pointer_event_handler(j2me_pointer_event_t* event, void* user_data) {
    j2me_vm_t* vm = (j2me_vm_t*)user_data;
    if (!vm || !event) {
        return;
    }
    
    printf("[VM事件] 指针事件: 类型=%d, 坐标=(%d,%d)\n", 
           event->type, event->x, event->y);
    
    // TODO: 调用当前Canvas的pointerPressed/pointerReleased/pointerDragged方法
    
    // 创建临时栈帧用于方法调用
    j2me_stack_frame_t* frame = j2me_stack_frame_create(10, 5);
    if (!frame) {
        printf("[VM事件] 错误: 创建栈帧失败\n");
        return;
    }
    
    // 模拟Canvas对象引用
    j2me_int canvas_ref = 0x30000001; // 假的Canvas对象引用
    
    // 压入参数
    j2me_operand_stack_push(&frame->operand_stack, canvas_ref);
    j2me_operand_stack_push(&frame->operand_stack, event->x);
    j2me_operand_stack_push(&frame->operand_stack, event->y);
    
    // 调用相应的Canvas事件方法
    j2me_error_t result;
    if (event->type == INPUT_EVENT_POINTER_PRESSED) {
        result = midp_canvas_pointer_pressed(vm, frame, NULL);
        if (result == J2ME_SUCCESS) {
            printf("[VM事件] Canvas.pointerPressed() 调用成功\n");
        }
    } else if (event->type == INPUT_EVENT_POINTER_RELEASED) {
        result = midp_canvas_pointer_released(vm, frame, NULL);
        if (result == J2ME_SUCCESS) {
            printf("[VM事件] Canvas.pointerReleased() 调用成功\n");
        }
    } else if (event->type == INPUT_EVENT_POINTER_DRAGGED) {
        result = midp_canvas_pointer_dragged(vm, frame, NULL);
        if (result == J2ME_SUCCESS) {
            printf("[VM事件] Canvas.pointerDragged() 调用成功\n");
        }
    }
    
    // 清理栈帧
    j2me_stack_frame_destroy(frame);
}

j2me_error_t j2me_vm_handle_events(j2me_vm_t* vm) {
    if (!vm || !vm->input_manager) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    SDL_Event event;
    bool events_handled = false;
    
    // 处理所有待处理的SDL事件
    while (SDL_PollEvent(&event)) {
        events_handled = true;
        
        // 检查退出事件
        if (event.type == SDL_QUIT) {
            printf("[VM事件] 收到退出事件\n");
            j2me_vm_stop(vm);
            return J2ME_SUCCESS;
        }
        
        // 将SDL事件传递给输入管理器
        bool handled = j2me_input_handle_sdl_event(vm->input_manager, &event);
        if (handled) {
            printf("[VM事件] SDL事件已处理: 类型=%d\n", event.type);
        }
    }
    
    // 更新输入状态
    j2me_input_update(vm->input_manager);
    
    return J2ME_SUCCESS;
}

/**
 * @brief 创建新线程
 * @param vm 虚拟机实例
 * @param thread_object Java Thread对象
 * @param runnable_object Runnable对象（可选）
 * @return 新线程指针，失败返回NULL
 */
j2me_thread_t* j2me_vm_create_thread(j2me_vm_t* vm, void* thread_object, void* runnable_object) {
    if (!vm) {
        return NULL;
    }
    
    // 创建新线程
    j2me_thread_t* thread = j2me_thread_create(vm->next_thread_id++);
    if (!thread) {
        printf("[VM] 错误: 线程创建失败\n");
        return NULL;
    }
    
    // 设置线程属性
    thread->thread_object = thread_object;
    thread->runnable_object = runnable_object;
    thread->is_daemon = false;
    thread->priority = 5; // 默认优先级
    
    // 添加到线程列表
    thread->next = vm->thread_list;
    vm->thread_list = thread;
    vm->thread_count++;
    
    printf("[VM] 创建新线程成功 (ID: %d, 总线程数: %zu)\n", thread->thread_id, vm->thread_count);
    return thread;
}

/**
 * @brief 启动线程（调用run方法）
 * @param vm 虚拟机实例
 * @param thread 线程实例
 * @return 错误码
 */
j2me_error_t j2me_vm_start_thread(j2me_vm_t* vm, j2me_thread_t* thread) {
    if (!vm || !thread) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    printf("[VM] 启动线程 (ID: %d)\n", thread->thread_id);
    
    // 查找run方法
    void* target_object = thread->runnable_object ? thread->runnable_object : thread->thread_object;
    if (!target_object) {
        printf("[VM] 错误: 线程没有关联的对象\n");
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 从对象中获取类信息
    j2me_object_t* obj = (j2me_object_t*)target_object;
    j2me_class_t* class_ptr = obj->header.class_ptr;
    if (!class_ptr) {
        printf("[VM] 错误: 对象没有关联的类\n");
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 查找run()方法
    j2me_method_t* run_method = NULL;
    for (int i = 0; i < class_ptr->methods_count; i++) {
        j2me_method_t* method = &class_ptr->methods[i];
        if (strcmp(method->name, "run") == 0 && strcmp(method->descriptor, "()V") == 0) {
            run_method = method;
            break;
        }
    }
    
    if (!run_method) {
        printf("[VM] 错误: 未找到run()方法\n");
        return J2ME_ERROR_METHOD_NOT_FOUND;
    }
    
    thread->run_method = run_method;
    thread->is_running = true;
    
    printf("[VM] 线程启动成功，run方法已找到\n");
    return J2ME_SUCCESS;
}

/**
 * @brief 执行线程的run方法（执行一批指令）
 * @param vm 虚拟机实例
 * @param thread 线程实例
 * @param instruction_count 要执行的指令数
 * @return 错误码
 */
j2me_error_t j2me_vm_execute_thread(j2me_vm_t* vm, j2me_thread_t* thread, uint32_t instruction_count) {
    if (!vm || !thread || !thread->is_running) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 如果线程还没有栈帧，创建初始栈帧并调用run方法
    if (!thread->current_frame && thread->run_method) {
        j2me_method_t* run_method = (j2me_method_t*)thread->run_method;
        void* target_object = thread->runnable_object ? thread->runnable_object : thread->thread_object;
        
        printf("[VM] 开始执行线程 %d 的run方法\n", thread->thread_id);
        
        // 切换到该线程
        j2me_thread_t* prev_thread = vm->current_thread;
        vm->current_thread = thread;
        
        // 执行run方法
        j2me_error_t result = j2me_interpreter_execute_method(vm, run_method, target_object, NULL);
        
        // 恢复之前的线程
        vm->current_thread = prev_thread;
        
        if (result != J2ME_SUCCESS) {
            printf("[VM] 线程 %d 的run方法执行失败: %d\n", thread->thread_id, result);
            thread->is_running = false;
            return result;
        }
        
        printf("[VM] 线程 %d 的run方法执行完成\n", thread->thread_id);
        thread->is_running = false;
        return J2ME_SUCCESS;
    }
    
    // 如果线程有当前栈帧，继续执行
    if (thread->current_frame) {
        j2me_thread_t* prev_thread = vm->current_thread;
        vm->current_thread = thread;
        
        j2me_error_t result = j2me_interpreter_execute_batch(vm, thread, instruction_count);
        
        vm->current_thread = prev_thread;
        
        if (result != J2ME_SUCCESS) {
            thread->is_running = false;
        }
        
        return result;
    }
    
    return J2ME_SUCCESS;
}

/**
 * @brief 执行所有活动线程
 * @param vm 虚拟机实例
 * @param instructions_per_thread 每个线程执行的指令数
 * @return 错误码
 */
j2me_error_t j2me_vm_execute_all_threads(j2me_vm_t* vm, uint32_t instructions_per_thread) {
    if (!vm) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 遍历所有线程
    j2me_thread_t* thread = vm->thread_list;
    while (thread) {
        if (thread->is_running) {
            j2me_vm_execute_thread(vm, thread, instructions_per_thread);
        }
        thread = thread->next;
    }
    
    return J2ME_SUCCESS;
}
