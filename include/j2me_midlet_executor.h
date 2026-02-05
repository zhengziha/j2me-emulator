#ifndef J2ME_MIDLET_EXECUTOR_H
#define J2ME_MIDLET_EXECUTOR_H

#include "j2me_types.h"
#include "j2me_jar.h"
#include "j2me_class.h"
#include "j2me_vm.h"

/**
 * @file j2me_midlet_executor.h
 * @brief J2ME MIDlet执行器
 * 
 * 负责MIDlet的加载、执行和生命周期管理
 */

// 前向声明
typedef struct j2me_midlet_executor j2me_midlet_executor_t;
typedef struct j2me_midlet_instance j2me_midlet_instance_t;

// MIDlet实例状态
typedef enum {
    MIDLET_INSTANCE_CREATED = 0,    // 已创建
    MIDLET_INSTANCE_STARTED,        // 已启动
    MIDLET_INSTANCE_PAUSED,         // 已暂停
    MIDLET_INSTANCE_DESTROYED       // 已销毁
} j2me_midlet_instance_state_t;

// MIDlet实例
struct j2me_midlet_instance {
    j2me_midlet_t* midlet_info;           // MIDlet信息
    j2me_class_t* midlet_class;           // MIDlet类
    void* midlet_object;                  // MIDlet对象实例
    j2me_midlet_instance_state_t state;   // 实例状态
    
    // 生命周期方法
    j2me_method_t* constructor;           // 构造方法
    j2me_method_t* start_app;             // startApp方法
    j2me_method_t* pause_app;             // pauseApp方法
    j2me_method_t* destroy_app;           // destroyApp方法
    
    // 运行时信息
    uint64_t start_time;                  // 启动时间
    uint64_t total_run_time;              // 总运行时间
    uint32_t pause_count;                 // 暂停次数
};

// MIDlet执行器
struct j2me_midlet_executor {
    j2me_vm_t* vm;                        // 虚拟机实例
    j2me_jar_file_t* jar_file;            // JAR文件
    j2me_midlet_suite_t* midlet_suite;    // MIDlet套件
    j2me_class_loader_t* class_loader;    // 类加载器
    
    // 当前运行的MIDlet
    j2me_midlet_instance_t* current_midlet;
    
    // 统计信息
    uint32_t total_midlets_run;           // 总运行MIDlet数
    uint64_t total_execution_time;        // 总执行时间
};

/**
 * @brief 创建MIDlet执行器
 * @param vm 虚拟机实例
 * @param jar_file JAR文件
 * @return MIDlet执行器指针
 */
j2me_midlet_executor_t* j2me_midlet_executor_create(j2me_vm_t* vm, j2me_jar_file_t* jar_file);

/**
 * @brief 销毁MIDlet执行器
 * @param executor MIDlet执行器
 */
void j2me_midlet_executor_destroy(j2me_midlet_executor_t* executor);

/**
 * @brief 加载MIDlet类
 * @param executor MIDlet执行器
 * @param midlet MIDlet信息
 * @return 错误码
 */
j2me_error_t j2me_midlet_executor_load_midlet(j2me_midlet_executor_t* executor, j2me_midlet_t* midlet);

/**
 * @brief 创建MIDlet实例
 * @param executor MIDlet执行器
 * @param midlet MIDlet信息
 * @return MIDlet实例指针
 */
j2me_midlet_instance_t* j2me_midlet_executor_create_instance(j2me_midlet_executor_t* executor, j2me_midlet_t* midlet);

/**
 * @brief 启动MIDlet实例
 * @param executor MIDlet执行器
 * @param instance MIDlet实例
 * @return 错误码
 */
j2me_error_t j2me_midlet_executor_start_instance(j2me_midlet_executor_t* executor, j2me_midlet_instance_t* instance);

/**
 * @brief 暂停MIDlet实例
 * @param executor MIDlet执行器
 * @param instance MIDlet实例
 * @return 错误码
 */
j2me_error_t j2me_midlet_executor_pause_instance(j2me_midlet_executor_t* executor, j2me_midlet_instance_t* instance);

/**
 * @brief 恢复MIDlet实例
 * @param executor MIDlet执行器
 * @param instance MIDlet实例
 * @return 错误码
 */
j2me_error_t j2me_midlet_executor_resume_instance(j2me_midlet_executor_t* executor, j2me_midlet_instance_t* instance);

/**
 * @brief 销毁MIDlet实例
 * @param executor MIDlet执行器
 * @param instance MIDlet实例
 * @return 错误码
 */
j2me_error_t j2me_midlet_executor_destroy_instance(j2me_midlet_executor_t* executor, j2me_midlet_instance_t* instance);

/**
 * @brief 运行MIDlet
 * @param executor MIDlet执行器
 * @param midlet_name MIDlet名称
 * @return 错误码
 */
j2me_error_t j2me_midlet_executor_run_midlet(j2me_midlet_executor_t* executor, const char* midlet_name);

/**
 * @brief 获取执行器统计信息
 * @param executor MIDlet执行器
 * @param total_midlets 总MIDlet数 (输出参数)
 * @param total_time 总执行时间 (输出参数)
 */
void j2me_midlet_executor_get_statistics(j2me_midlet_executor_t* executor, 
                                         uint32_t* total_midlets, uint64_t* total_time);

/**
 * @brief 获取MIDlet实例状态名称
 * @param state MIDlet实例状态
 * @return 状态名称字符串
 */
const char* j2me_midlet_instance_get_state_name(j2me_midlet_instance_state_t state);

#endif // J2ME_MIDLET_EXECUTOR_H