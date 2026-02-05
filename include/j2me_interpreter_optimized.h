/**
 * @file j2me_interpreter_optimized.h
 * @brief 优化的J2ME字节码解释器头文件
 * 
 * 高性能字节码解释器，包含以下优化:
 * - 指令预解码和缓存
 * - 跳转表优化的指令分发
 * - 内联缓存的方法调用
 * - 热点检测和批量执行
 * - 性能监控和统计
 */

#ifndef J2ME_INTERPRETER_OPTIMIZED_H
#define J2ME_INTERPRETER_OPTIMIZED_H

#include "j2me_types.h"
#include "j2me_interpreter.h"

#ifdef __cplusplus
extern "C" {
#endif

// 预解码指令结构
typedef struct {
    j2me_opcode_t opcode;           // 原始字节码
    j2me_byte operand_count;        // 操作数数量
    j2me_int operands[3];           // 预解析的操作数
    void* handler;                  // 指令处理函数指针
    j2me_int flags;                 // 指令标志 (跳转、方法调用等)
} j2me_predecoded_instruction_t;

// 指令标志
#define INST_FLAG_JUMP          0x01    // 跳转指令
#define INST_FLAG_METHOD_CALL   0x02    // 方法调用
#define INST_FLAG_FIELD_ACCESS  0x04    // 字段访问
#define INST_FLAG_BRANCH        0x08    // 条件分支
#define INST_FLAG_RETURN        0x10    // 返回指令

// 方法内联缓存条目
typedef struct {
    j2me_int method_ref;            // 方法引用
    void* target_method;            // 目标方法
    j2me_int call_count;            // 调用次数
    j2me_long last_access_time;     // 最后访问时间
} j2me_inline_cache_entry_t;

// 内联缓存
typedef struct {
    j2me_inline_cache_entry_t* entries;
    size_t size;
    size_t capacity;
    j2me_int hit_count;
    j2me_int miss_count;
} j2me_inline_cache_t;

// 热点检测器
typedef struct {
    j2me_int* method_counters;      // 方法调用计数器
    j2me_int* loop_counters;        // 循环执行计数器
    size_t method_count;
    size_t loop_count;
    j2me_int hotspot_threshold;     // 热点阈值
    j2me_int compilation_threshold; // 编译阈值
} j2me_hotspot_detector_t;

// 性能统计
typedef struct {
    j2me_long total_instructions;   // 总指令数
    j2me_long total_cycles;         // 总执行周期
    j2me_long method_calls;         // 方法调用次数
    j2me_long cache_hits;           // 缓存命中次数
    j2me_long cache_misses;         // 缓存未命中次数
    j2me_long hotspot_compilations; // 热点编译次数
    j2me_long start_time;           // 开始时间
    j2me_long end_time;             // 结束时间
} j2me_performance_stats_t;

// 优化的解释器上下文
typedef struct {
    j2me_predecoded_instruction_t* predecoded_code; // 预解码指令
    size_t code_length;                             // 代码长度
    j2me_inline_cache_t* inline_cache;              // 内联缓存
    j2me_hotspot_detector_t* hotspot_detector;      // 热点检测器
    j2me_performance_stats_t* stats;                // 性能统计
    j2me_boolean optimization_enabled;                 // 优化开关
    j2me_int batch_size;                           // 批量执行大小
} j2me_optimized_interpreter_t;

// 指令处理函数类型
typedef j2me_error_t (*j2me_instruction_handler_t)(j2me_vm_t* vm, 
                                                   j2me_stack_frame_t* frame,
                                                   j2me_predecoded_instruction_t* inst);

/**
 * @brief 创建优化的解释器
 * @param code_size 代码大小
 * @return 解释器实例，失败返回NULL
 */
j2me_optimized_interpreter_t* j2me_optimized_interpreter_create(size_t code_size);

/**
 * @brief 销毁优化的解释器
 * @param interpreter 解释器实例
 */
void j2me_optimized_interpreter_destroy(j2me_optimized_interpreter_t* interpreter);

/**
 * @brief 预解码字节码
 * @param interpreter 解释器实例
 * @param bytecode 原始字节码
 * @param length 字节码长度
 * @return 错误码
 */
j2me_error_t j2me_predecode_bytecode(j2me_optimized_interpreter_t* interpreter,
                                     const j2me_byte* bytecode,
                                     size_t length);

/**
 * @brief 优化的字节码执行
 * @param vm 虚拟机实例
 * @param frame 栈帧
 * @param interpreter 优化解释器
 * @param max_instructions 最大执行指令数
 * @return 错误码
 */
j2me_error_t j2me_execute_optimized(j2me_vm_t* vm,
                                    j2me_stack_frame_t* frame,
                                    j2me_optimized_interpreter_t* interpreter,
                                    j2me_int max_instructions);

/**
 * @brief 批量执行指令
 * @param vm 虚拟机实例
 * @param frame 栈帧
 * @param interpreter 优化解释器
 * @param start_pc 起始PC
 * @param batch_size 批量大小
 * @return 执行的指令数
 */
j2me_int j2me_execute_batch(j2me_vm_t* vm,
                           j2me_stack_frame_t* frame,
                           j2me_optimized_interpreter_t* interpreter,
                           j2me_int start_pc,
                           j2me_int batch_size);

/**
 * @brief 创建内联缓存
 * @param capacity 缓存容量
 * @return 内联缓存实例
 */
j2me_inline_cache_t* j2me_inline_cache_create(size_t capacity);

/**
 * @brief 销毁内联缓存
 * @param cache 内联缓存实例
 */
void j2me_inline_cache_destroy(j2me_inline_cache_t* cache);

/**
 * @brief 查找内联缓存
 * @param cache 内联缓存
 * @param method_ref 方法引用
 * @return 目标方法，未找到返回NULL
 */
void* j2me_inline_cache_lookup(j2me_inline_cache_t* cache, j2me_int method_ref);

/**
 * @brief 更新内联缓存
 * @param cache 内联缓存
 * @param method_ref 方法引用
 * @param target_method 目标方法
 * @return 错误码
 */
j2me_error_t j2me_inline_cache_update(j2me_inline_cache_t* cache,
                                      j2me_int method_ref,
                                      void* target_method);

/**
 * @brief 创建热点检测器
 * @param method_count 方法数量
 * @param loop_count 循环数量
 * @param hotspot_threshold 热点阈值
 * @return 热点检测器实例
 */
j2me_hotspot_detector_t* j2me_hotspot_detector_create(size_t method_count,
                                                      size_t loop_count,
                                                      j2me_int hotspot_threshold);

/**
 * @brief 销毁热点检测器
 * @param detector 热点检测器实例
 */
void j2me_hotspot_detector_destroy(j2me_hotspot_detector_t* detector);

/**
 * @brief 记录方法调用
 * @param detector 热点检测器
 * @param method_id 方法ID
 * @return 是否达到热点阈值
 */
j2me_boolean j2me_hotspot_record_method_call(j2me_hotspot_detector_t* detector,
                                          j2me_int method_id);

/**
 * @brief 记录循环执行
 * @param detector 热点检测器
 * @param loop_id 循环ID
 * @return 是否达到热点阈值
 */
j2me_boolean j2me_hotspot_record_loop_execution(j2me_hotspot_detector_t* detector,
                                             j2me_int loop_id);

/**
 * @brief 创建性能统计
 * @return 性能统计实例
 */
j2me_performance_stats_t* j2me_performance_stats_create(void);

/**
 * @brief 销毁性能统计
 * @param stats 性能统计实例
 */
void j2me_performance_stats_destroy(j2me_performance_stats_t* stats);

/**
 * @brief 开始性能统计
 * @param stats 性能统计实例
 */
void j2me_performance_stats_start(j2me_performance_stats_t* stats);

/**
 * @brief 结束性能统计
 * @param stats 性能统计实例
 */
void j2me_performance_stats_end(j2me_performance_stats_t* stats);

/**
 * @brief 记录指令执行
 * @param stats 性能统计实例
 * @param instruction_count 指令数量
 * @param cycles 执行周期
 */
void j2me_performance_stats_record_instructions(j2me_performance_stats_t* stats,
                                               j2me_int instruction_count,
                                               j2me_long cycles);

/**
 * @brief 记录方法调用
 * @param stats 性能统计实例
 */
void j2me_performance_stats_record_method_call(j2me_performance_stats_t* stats);

/**
 * @brief 记录缓存命中
 * @param stats 性能统计实例
 * @param hit 是否命中
 */
void j2me_performance_stats_record_cache_access(j2me_performance_stats_t* stats,
                                                j2me_boolean hit);

/**
 * @brief 打印性能统计报告
 * @param stats 性能统计实例
 */
void j2me_performance_stats_print_report(j2me_performance_stats_t* stats);

/**
 * @brief 获取指令执行速度 (指令/秒)
 * @param stats 性能统计实例
 * @return 执行速度
 */
j2me_double j2me_performance_stats_get_instructions_per_second(j2me_performance_stats_t* stats);

/**
 * @brief 获取缓存命中率
 * @param stats 性能统计实例
 * @return 命中率 (0.0-1.0)
 */
j2me_double j2me_performance_stats_get_cache_hit_rate(j2me_performance_stats_t* stats);

// 指令处理函数声明
j2me_error_t j2me_handle_nop(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst);
j2me_error_t j2me_handle_iconst(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst);
j2me_error_t j2me_handle_bipush(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst);
j2me_error_t j2me_handle_sipush(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst);
j2me_error_t j2me_handle_iload(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst);
j2me_error_t j2me_handle_aload(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst);
j2me_error_t j2me_handle_istore(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst);
j2me_error_t j2me_handle_astore(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst);
j2me_error_t j2me_handle_pop(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst);
j2me_error_t j2me_handle_dup(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst);
j2me_error_t j2me_handle_iadd(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst);
j2me_error_t j2me_handle_isub(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst);
j2me_error_t j2me_handle_imul(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst);
j2me_error_t j2me_handle_idiv(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst);
j2me_error_t j2me_handle_ifeq(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst);
j2me_error_t j2me_handle_ifne(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst);
j2me_error_t j2me_handle_goto(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst);
j2me_error_t j2me_handle_ireturn(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst);
j2me_error_t j2me_handle_return(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst);
j2me_error_t j2me_handle_invokevirtual(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst);
j2me_error_t j2me_handle_invokespecial(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst);
j2me_error_t j2me_handle_invokestatic(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst);

#ifdef __cplusplus
}
#endif

#endif // J2ME_INTERPRETER_OPTIMIZED_H