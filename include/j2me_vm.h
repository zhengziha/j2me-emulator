#ifndef J2ME_VM_H
#define J2ME_VM_H

#include "j2me_types.h"
#include "j2me_graphics.h"
#include "j2me_input.h"
#include "j2me_interpreter_optimized.h"
#include "j2me_gc.h"
#include "j2me_heap.h"
#include <stddef.h>

/**
 * @file j2me_vm.h
 * @brief J2ME虚拟机核心接口
 * 
 * 定义虚拟机的主要结构和接口函数
 */

// 虚拟机配置
typedef struct {
    size_t heap_size;           // 堆大小 (字节)
    size_t stack_size;          // 栈大小 (字节)
    size_t max_threads;         // 最大线程数
    bool enable_gc;             // 是否启用垃圾回收
    bool enable_jit;            // 是否启用JIT编译
} j2me_vm_config_t;

// 前向声明
struct j2me_native_method_registry;

// 虚拟机实例
struct j2me_vm {
    j2me_vm_state_t state;      // 虚拟机状态
    j2me_vm_config_t config;    // 配置信息
    
    // 内存管理
    void* heap_start;           // 堆起始地址（旧系统，保留兼容）
    void* heap_end;             // 堆结束地址（旧系统，保留兼容）
    void* heap_current;         // 当前堆指针（旧系统，保留兼容）
    j2me_heap_t* heap;          // 新的对象堆系统
    j2me_gc_t* gc;              // 垃圾回收器
    
    // 线程管理
    j2me_thread_t* main_thread; // 主线程
    j2me_thread_t* current_thread; // 当前线程
    j2me_thread_t* thread_list; // 所有线程的链表
    uint32_t next_thread_id;    // 下一个线程ID
    size_t thread_count;        // 线程数量
    
    // 类加载器
    void* class_loader;         // 类加载器实例
    
    // 本地方法支持
    struct j2me_native_method_registry* native_method_registry; // 本地方法注册表
    
    // 图形显示系统
    j2me_display_t* display;    // 显示系统实例
    
    // 输入系统
    j2me_input_manager_t* input_manager; // 输入管理器
    
    // 当前活动的Canvas对象
    j2me_int current_canvas_ref; // 当前Canvas对象引用
    
    // 最后创建的Canvas类对象（用于Display.setCurrent）
    j2me_int last_canvas_object_ref; // 最后创建的Canvas类对象引用
    
    // 当前创建的Runnable对象（用于Thread构造）
    j2me_int current_runnable_ref; // 当前Runnable对象引用
    
    // 方法调用返回值（用于传递嵌套调用的返回值）
    j2me_int last_method_return_value;
    bool last_method_has_return_value;
    
    // 优化解释器
    j2me_optimized_interpreter_t* optimized_interpreter; // 优化解释器实例
    
    // 统计信息
    uint64_t instructions_executed; // 执行的指令数
    uint64_t gc_collections;    // GC次数
};

/**
 * @brief 创建虚拟机实例
 * @param config 虚拟机配置
 * @return 虚拟机实例指针，失败返回NULL
 */
j2me_vm_t* j2me_vm_create(const j2me_vm_config_t* config);

/**
 * @brief 销毁虚拟机实例
 * @param vm 虚拟机实例
 */
void j2me_vm_destroy(j2me_vm_t* vm);

/**
 * @brief 初始化虚拟机
 * @param vm 虚拟机实例
 * @return 错误码
 */
j2me_error_t j2me_vm_initialize(j2me_vm_t* vm);

/**
 * @brief 启动虚拟机
 * @param vm 虚拟机实例
 * @param main_class 主类名
 * @return 错误码
 */
j2me_error_t j2me_vm_start(j2me_vm_t* vm, const char* main_class);

/**
 * @brief 停止虚拟机
 * @param vm 虚拟机实例
 */
void j2me_vm_stop(j2me_vm_t* vm);

/**
 * @brief 执行一个时间片
 * @param vm 虚拟机实例
 * @param time_slice 时间片长度(毫秒)
 * @return 错误码
 */
j2me_error_t j2me_vm_execute_time_slice(j2me_vm_t* vm, uint32_t time_slice);

/**
 * @brief 处理输入事件
 * @param vm 虚拟机实例
 * @return 错误码
 */
j2me_error_t j2me_vm_handle_events(j2me_vm_t* vm);

/**
 * @brief 键盘事件处理回调
 * @param event 键盘事件
 * @param user_data 用户数据
 */
void j2me_vm_key_event_handler(j2me_key_event_t* event, void* user_data);

/**
 * @brief 指针事件处理回调
 * @param event 指针事件
 * @param user_data 用户数据
 */
void j2me_vm_pointer_event_handler(j2me_pointer_event_t* event, void* user_data);

/**
 * @brief 获取默认虚拟机配置
 * @return 默认配置
 */
j2me_vm_config_t j2me_vm_get_default_config(void);

/**
 * @brief 创建新线程
 * @param vm 虚拟机实例
 * @param thread_object Java Thread对象
 * @param runnable_object Runnable对象（可选）
 * @return 新线程指针，失败返回NULL
 */
j2me_thread_t* j2me_vm_create_thread(j2me_vm_t* vm, void* thread_object, void* runnable_object);

/**
 * @brief 启动线程（调用run方法）
 * @param vm 虚拟机实例
 * @param thread 线程实例
 * @return 错误码
 */
j2me_error_t j2me_vm_start_thread(j2me_vm_t* vm, j2me_thread_t* thread);

/**
 * @brief 执行线程的run方法（执行一批指令）
 * @param vm 虚拟机实例
 * @param thread 线程实例
 * @param instruction_count 要执行的指令数
 * @return 错误码
 */
j2me_error_t j2me_vm_execute_thread(j2me_vm_t* vm, j2me_thread_t* thread, uint32_t instruction_count);

/**
 * @brief 执行所有活动线程
 * @param vm 虚拟机实例
 * @param instructions_per_thread 每个线程执行的指令数
 * @return 错误码
 */
j2me_error_t j2me_vm_execute_all_threads(j2me_vm_t* vm, uint32_t instructions_per_thread);

#endif // J2ME_VM_H