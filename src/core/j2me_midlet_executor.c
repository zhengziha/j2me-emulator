#include "j2me_midlet_executor.h"
#include "j2me_interpreter.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/**
 * @file j2me_midlet_executor.c
 * @brief J2ME MIDlet执行器实现
 * 
 * 负责MIDlet的加载、执行和生命周期管理
 */

/**
 * @brief 获取当前时间戳 (毫秒)
 */
static uint64_t get_current_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

j2me_midlet_executor_t* j2me_midlet_executor_create(j2me_vm_t* vm, j2me_jar_file_t* jar_file) {
    if (!vm || !jar_file) {
        return NULL;
    }
    
    j2me_midlet_executor_t* executor = (j2me_midlet_executor_t*)malloc(sizeof(j2me_midlet_executor_t));
    if (!executor) {
        return NULL;
    }
    
    memset(executor, 0, sizeof(j2me_midlet_executor_t));
    
    executor->vm = vm;
    executor->jar_file = jar_file;
    executor->midlet_suite = j2me_jar_get_midlet_suite(jar_file);
    
    if (!executor->midlet_suite) {
        printf("[MIDlet执行器] 错误: 无法获取MIDlet套件\n");
        free(executor);
        return NULL;
    }
    
    // 创建类加载器
    executor->class_loader = j2me_class_loader_create(vm, ".");
    if (!executor->class_loader) {
        printf("[MIDlet执行器] 错误: 创建类加载器失败\n");
        free(executor);
        return NULL;
    }
    
    // 设置JAR文件到类加载器
    j2me_class_loader_set_jar_file(executor->class_loader, jar_file);
    
    printf("[MIDlet执行器] 创建成功，MIDlet套件: %s\n", 
           executor->midlet_suite->name ? executor->midlet_suite->name : "未知");
    
    return executor;
}

void j2me_midlet_executor_destroy(j2me_midlet_executor_t* executor) {
    if (!executor) {
        return;
    }
    
    // 销毁当前MIDlet实例
    if (executor->current_midlet) {
        j2me_midlet_executor_destroy_instance(executor, executor->current_midlet);
    }
    
    // 销毁类加载器
    if (executor->class_loader) {
        j2me_class_loader_destroy(executor->class_loader);
    }
    
    printf("[MIDlet执行器] 已销毁\n");
    free(executor);
}

j2me_error_t j2me_midlet_executor_load_midlet(j2me_midlet_executor_t* executor, j2me_midlet_t* midlet) {
    if (!executor || !midlet || !midlet->class_name) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    printf("[MIDlet执行器] 加载MIDlet类: %s\n", midlet->class_name);
    
    // 加载MIDlet类
    j2me_class_t* midlet_class = j2me_class_loader_load_class(executor->class_loader, midlet->class_name);
    if (!midlet_class) {
        printf("[MIDlet执行器] 错误: 加载MIDlet类失败: %s\n", midlet->class_name);
        return J2ME_ERROR_CLASS_NOT_FOUND;
    }
    
    // 链接类
    j2me_error_t result = j2me_class_link(midlet_class);
    if (result != J2ME_SUCCESS) {
        printf("[MIDlet执行器] 错误: 链接MIDlet类失败: %s\n", midlet->class_name);
        return result;
    }
    
    // 初始化类
    result = j2me_class_initialize(midlet_class);
    if (result != J2ME_SUCCESS) {
        printf("[MIDlet执行器] 错误: 初始化MIDlet类失败: %s\n", midlet->class_name);
        return result;
    }
    
    printf("[MIDlet执行器] MIDlet类加载成功: %s\n", midlet->class_name);
    return J2ME_SUCCESS;
}

j2me_midlet_instance_t* j2me_midlet_executor_create_instance(j2me_midlet_executor_t* executor, j2me_midlet_t* midlet) {
    if (!executor || !midlet) {
        return NULL;
    }
    
    printf("[MIDlet执行器] 创建MIDlet实例: %s\n", midlet->name);
    
    // 首先加载MIDlet类
    j2me_error_t result = j2me_midlet_executor_load_midlet(executor, midlet);
    if (result != J2ME_SUCCESS) {
        return NULL;
    }
    
    // 查找MIDlet类
    j2me_class_t* midlet_class = j2me_class_loader_find_class(executor->class_loader, midlet->class_name);
    if (!midlet_class) {
        printf("[MIDlet执行器] 错误: 找不到MIDlet类: %s\n", midlet->class_name);
        return NULL;
    }
    
    printf("[MIDlet执行器] 找到MIDlet类: %s (方法数: %d)\n", 
           midlet_class->name ? midlet_class->name : "unknown", 
           midlet_class->methods_count);
    
    // 创建MIDlet实例
    j2me_midlet_instance_t* instance = (j2me_midlet_instance_t*)malloc(sizeof(j2me_midlet_instance_t));
    if (!instance) {
        return NULL;
    }
    
    memset(instance, 0, sizeof(j2me_midlet_instance_t));
    
    instance->midlet_info = midlet;
    instance->midlet_class = midlet_class;
    instance->state = MIDLET_INSTANCE_CREATED;
    
    // 创建MIDlet对象实例
    instance->midlet_object = j2me_object_create(executor->vm, midlet_class);
    if (!instance->midlet_object) {
        printf("[MIDlet执行器] 错误: 创建MIDlet对象实例失败\n");
        free(instance);
        return NULL;
    }
    
    printf("[MIDlet执行器] MIDlet对象实例创建成功: %p\n", instance->midlet_object);
    
    // 查找生命周期方法
    instance->constructor = j2me_class_find_method(midlet_class, "<init>", "()V");
    instance->start_app = j2me_class_find_method(midlet_class, "startApp", "()V");
    instance->pause_app = j2me_class_find_method(midlet_class, "pauseApp", "()V");
    instance->destroy_app = j2me_class_find_method(midlet_class, "destroyApp", "(Z)V");
    
    printf("[MIDlet执行器] MIDlet实例创建成功: %s\n", midlet->name);
    printf("[MIDlet执行器] 生命周期方法: 构造=%p, 启动=%p, 暂停=%p, 销毁=%p\n", 
           instance->constructor, instance->start_app, instance->pause_app, instance->destroy_app);
    
    // 检查关键方法的字节码
    if (instance->constructor) {
        printf("[MIDlet执行器] 构造方法字节码: %s (%d bytes)\n", 
               instance->constructor->bytecode ? "有" : "无", 
               instance->constructor->bytecode_length);
    }
    if (instance->start_app) {
        printf("[MIDlet执行器] startApp方法字节码: %s (%d bytes)\n", 
               instance->start_app->bytecode ? "有" : "无", 
               instance->start_app->bytecode_length);
    }
    
    return instance;
}

/**
 * @brief 执行方法调用 (真实字节码执行)
 * @param executor MIDlet执行器
 * @param method 方法
 * @param object 对象实例
 * @param args 参数
 * @return 错误码
 */
static j2me_error_t execute_method_call(j2me_midlet_executor_t* executor, 
                                        j2me_method_t* method, 
                                        void* object, 
                                        void* args) {
    printf("[Execute] Method=%p, Object=%p\n", method, object);
    
    if (!method) {
        printf("[Execute] ERROR: method is NULL\n");
        return J2ME_ERROR_METHOD_NOT_FOUND;
    }
    
    printf("[Execute] Calling interpreter (bytecode_length=%d)...\n", method->bytecode_length);
    fflush(stdout);  // 确保输出被刷新
    
    // 直接调用解释器，不打印调试信息
    j2me_error_t result = j2me_interpreter_execute_method(executor->vm, method, object, args);
    
    printf("[Execute] Interpreter returned: %d\n", result);
    
    return result;
}

j2me_error_t j2me_midlet_executor_start_instance(j2me_midlet_executor_t* executor, j2me_midlet_instance_t* instance) {
    if (!executor || !instance) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    if (instance->state != MIDLET_INSTANCE_CREATED && instance->state != MIDLET_INSTANCE_PAUSED) {
        printf("[MIDlet执行器] 错误: MIDlet实例状态不正确: %s\n", 
               j2me_midlet_instance_get_state_name(instance->state));
        return J2ME_ERROR_INVALID_STATE;
    }
    
    printf("[MIDlet Executor] Starting MIDlet instance\n");
    
    // If first start, call constructor first
    if (instance->state == MIDLET_INSTANCE_CREATED) {
        printf("[MIDlet Executor] Calling constructor...\n");
        j2me_error_t result = execute_method_call(executor, instance->constructor, instance->midlet_object, NULL);
        if (result != J2ME_SUCCESS) {
            printf("[MIDlet Executor] ERROR: Constructor failed: %d\n", result);
            return result;
        }
        printf("[MIDlet Executor] Constructor executed successfully\n");
    }
    
    // Call startApp method
    printf("[MIDlet Executor] Calling startApp...\n");
    j2me_error_t result = execute_method_call(executor, instance->start_app, instance->midlet_object, NULL);
    if (result != J2ME_SUCCESS) {
        printf("[MIDlet Executor] ERROR: startApp failed: %d\n", result);
        return result;
    }
    printf("[MIDlet Executor] startApp executed successfully\n");
    
    instance->state = MIDLET_INSTANCE_STARTED;
    instance->start_time = get_current_time_ms();
    executor->current_midlet = instance;
    executor->total_midlets_run++;
    
    printf("[MIDlet执行器] MIDlet实例启动成功: %s\n", instance->midlet_info->name);
    return J2ME_SUCCESS;
}

j2me_error_t j2me_midlet_executor_pause_instance(j2me_midlet_executor_t* executor, j2me_midlet_instance_t* instance) {
    if (!executor || !instance) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    if (instance->state != MIDLET_INSTANCE_STARTED) {
        printf("[MIDlet执行器] 错误: MIDlet实例未启动\n");
        return J2ME_ERROR_INVALID_STATE;
    }
    
    printf("[MIDlet执行器] 暂停MIDlet实例: %s\n", instance->midlet_info->name);
    
    // 调用pauseApp方法
    j2me_error_t result = execute_method_call(executor, instance->pause_app, instance->midlet_object, NULL);
    if (result != J2ME_SUCCESS) {
        printf("[MIDlet执行器] 错误: 调用pauseApp方法失败\n");
        return result;
    }
    
    instance->state = MIDLET_INSTANCE_PAUSED;
    instance->pause_count++;
    
    // 更新运行时间
    uint64_t current_time = get_current_time_ms();
    instance->total_run_time += (current_time - instance->start_time);
    
    printf("[MIDlet执行器] MIDlet实例暂停成功: %s\n", instance->midlet_info->name);
    return J2ME_SUCCESS;
}

j2me_error_t j2me_midlet_executor_resume_instance(j2me_midlet_executor_t* executor, j2me_midlet_instance_t* instance) {
    if (!executor || !instance) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    if (instance->state != MIDLET_INSTANCE_PAUSED) {
        printf("[MIDlet执行器] 错误: MIDlet实例未暂停\n");
        return J2ME_ERROR_INVALID_STATE;
    }
    
    printf("[MIDlet执行器] 恢复MIDlet实例: %s\n", instance->midlet_info->name);
    
    // 恢复实际上是重新调用startApp
    return j2me_midlet_executor_start_instance(executor, instance);
}

j2me_error_t j2me_midlet_executor_destroy_instance(j2me_midlet_executor_t* executor, j2me_midlet_instance_t* instance) {
    if (!executor || !instance) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    printf("[MIDlet Executor] Destroying MIDlet instance\n");
    
    // 如果实例正在运行，先暂停
    if (instance->state == MIDLET_INSTANCE_STARTED) {
        j2me_midlet_executor_pause_instance(executor, instance);
    }
    
    // 调用destroyApp方法 - 暂时跳过以避免崩溃
    // if (instance->destroy_app) {
    //     // destroyApp(boolean unconditional) - 传递true表示无条件销毁
    //     j2me_error_t result = execute_method_call(executor, instance->destroy_app, instance->midlet_object, (void*)1);
    //     if (result != J2ME_SUCCESS) {
    //         printf("[MIDlet Executor] WARNING: destroyApp failed\n");
    //     }
    // }
    
    instance->state = MIDLET_INSTANCE_DESTROYED;
    
    // 更新总执行时间
    if (instance->total_run_time > 0) {
        executor->total_execution_time += instance->total_run_time;
    }
    
    // 如果这是当前MIDlet，清除引用
    if (executor->current_midlet == instance) {
        executor->current_midlet = NULL;
    }
    
    printf("[MIDlet Executor] MIDlet instance destroyed successfully\n");
    
    free(instance);
    return J2ME_SUCCESS;
}

j2me_error_t j2me_midlet_executor_run_midlet(j2me_midlet_executor_t* executor, const char* midlet_name) {
    if (!executor || !midlet_name) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    printf("[MIDlet执行器] 运行MIDlet: %s\n", midlet_name);
    
    // 首先尝试按名称查找MIDlet
    j2me_midlet_t* midlet = j2me_midlet_suite_find_midlet(executor->midlet_suite, midlet_name);
    
    // 如果按名称找不到，尝试按类名查找
    if (!midlet) {
        midlet = j2me_midlet_suite_find_midlet_by_class(executor->midlet_suite, midlet_name);
    }
    
    if (!midlet) {
        printf("[MIDlet执行器] 错误: 找不到MIDlet: %s\n", midlet_name);
        return J2ME_ERROR_CLASS_NOT_FOUND;
    }
    
    printf("[MIDlet执行器] 找到MIDlet: %s (类: %s)\n", 
           midlet->name ? midlet->name : "未知", 
           midlet->class_name ? midlet->class_name : "未知");
    
    // 如果有当前运行的MIDlet，先销毁它
    if (executor->current_midlet) {
        j2me_midlet_executor_destroy_instance(executor, executor->current_midlet);
    }
    
    // 创建新的MIDlet实例
    j2me_midlet_instance_t* instance = j2me_midlet_executor_create_instance(executor, midlet);
    if (!instance) {
        printf("[MIDlet执行器] 错误: 创建MIDlet实例失败: %s\n", midlet_name);
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    // 启动MIDlet实例
    j2me_error_t result = j2me_midlet_executor_start_instance(executor, instance);
    if (result != J2ME_SUCCESS) {
        printf("[MIDlet执行器] 错误: 启动MIDlet实例失败: %s\n", midlet_name);
        free(instance);
        return result;
    }
    
    printf("[MIDlet执行器] MIDlet运行成功: %s\n", midlet_name);
    return J2ME_SUCCESS;
}

void j2me_midlet_executor_get_statistics(j2me_midlet_executor_t* executor, 
                                         uint32_t* total_midlets, uint64_t* total_time) {
    if (!executor) {
        if (total_midlets) *total_midlets = 0;
        if (total_time) *total_time = 0;
        return;
    }
    
    if (total_midlets) *total_midlets = executor->total_midlets_run;
    if (total_time) *total_time = executor->total_execution_time;
}

const char* j2me_midlet_instance_get_state_name(j2me_midlet_instance_state_t state) {
    switch (state) {
        case MIDLET_INSTANCE_CREATED: return "已创建";
        case MIDLET_INSTANCE_STARTED: return "已启动";
        case MIDLET_INSTANCE_PAUSED: return "已暂停";
        case MIDLET_INSTANCE_DESTROYED: return "已销毁";
        default: return "未知";
    }
}