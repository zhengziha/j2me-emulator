/**
 * @file j2me_interpreter_optimized.c
 * @brief 优化的J2ME字节码解释器实现
 * 
 * 高性能字节码解释器，包含以下优化:
 * - 指令预解码和缓存
 * - 跳转表优化的指令分发
 * - 内联缓存的方法调用
 * - 热点检测和批量执行
 * - 性能监控和统计
 */

#include "j2me_interpreter_optimized.h"
#include "j2me_bytecode.h"
#include "j2me_vm.h"
#include "j2me_native_methods.h"
#include <stdlib.h>
#include <string.h>
#include "j2me_log.h"
#include <stdio.h>
#include <time.h>

// 获取当前时间戳 (微秒)
static j2me_long get_current_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;
}

// 指令处理函数跳转表
static j2me_instruction_handler_t instruction_handlers[256] = {0};

// 初始化指令处理函数跳转表
static void initialize_instruction_handlers(void) {
    static j2me_boolean initialized = false;
    if (initialized) return;
    
    // 基础指令
    instruction_handlers[0x00] = j2me_handle_nop;           // nop
    instruction_handlers[0x01] = j2me_handle_iconst;        // aconst_null
    instruction_handlers[0x02] = j2me_handle_iconst;        // iconst_m1
    instruction_handlers[0x03] = j2me_handle_iconst;        // iconst_0
    instruction_handlers[0x04] = j2me_handle_iconst;        // iconst_1
    instruction_handlers[0x05] = j2me_handle_iconst;        // iconst_2
    instruction_handlers[0x06] = j2me_handle_iconst;        // iconst_3
    instruction_handlers[0x07] = j2me_handle_iconst;        // iconst_4
    instruction_handlers[0x08] = j2me_handle_iconst;        // iconst_5
    
    // 加载指令
    instruction_handlers[0x10] = j2me_handle_bipush;        // bipush
    instruction_handlers[0x11] = j2me_handle_sipush;        // sipush
    instruction_handlers[0x15] = j2me_handle_iload;         // iload
    instruction_handlers[0x1a] = j2me_handle_iload;         // iload_0
    instruction_handlers[0x1b] = j2me_handle_iload;         // iload_1
    instruction_handlers[0x1c] = j2me_handle_iload;         // iload_2
    instruction_handlers[0x1d] = j2me_handle_iload;         // iload_3
    instruction_handlers[0x2a] = j2me_handle_aload;         // aload_0
    instruction_handlers[0x2b] = j2me_handle_aload;         // aload_1
    instruction_handlers[0x2c] = j2me_handle_aload;         // aload_2
    instruction_handlers[0x2d] = j2me_handle_aload;         // aload_3
    
    // 存储指令
    instruction_handlers[0x36] = j2me_handle_istore;        // istore
    instruction_handlers[0x3b] = j2me_handle_istore;        // istore_0
    instruction_handlers[0x3c] = j2me_handle_istore;        // istore_1
    instruction_handlers[0x3d] = j2me_handle_istore;        // istore_2
    instruction_handlers[0x3e] = j2me_handle_istore;        // istore_3
    instruction_handlers[0x4b] = j2me_handle_astore;        // astore_0
    instruction_handlers[0x4c] = j2me_handle_astore;        // astore_1
    instruction_handlers[0x4d] = j2me_handle_astore;        // astore_2
    instruction_handlers[0x4e] = j2me_handle_astore;        // astore_3
    
    // 栈操作指令
    instruction_handlers[0x57] = j2me_handle_pop;           // pop
    instruction_handlers[0x58] = j2me_handle_pop;           // pop2
    instruction_handlers[0x59] = j2me_handle_dup;           // dup
    
    // 算术指令
    instruction_handlers[0x60] = j2me_handle_iadd;          // iadd
    instruction_handlers[0x64] = j2me_handle_isub;          // isub
    instruction_handlers[0x68] = j2me_handle_imul;          // imul
    instruction_handlers[0x6c] = j2me_handle_idiv;          // idiv
    
    // 控制流指令
    instruction_handlers[0x99] = j2me_handle_ifeq;          // ifeq
    instruction_handlers[0x9a] = j2me_handle_ifne;          // ifne
    instruction_handlers[0xa7] = j2me_handle_goto;          // goto
    
    // 返回指令
    instruction_handlers[0xac] = j2me_handle_ireturn;       // ireturn
    instruction_handlers[0xb1] = j2me_handle_return;        // return
    
    // 方法调用指令
    instruction_handlers[0xb6] = j2me_handle_invokevirtual; // invokevirtual
    instruction_handlers[0xb7] = j2me_handle_invokespecial; // invokespecial
    instruction_handlers[0xb8] = j2me_handle_invokestatic;  // invokestatic
    
    initialized = true;
}

/**
 * @brief 创建优化的解释器
 */
j2me_optimized_interpreter_t* j2me_optimized_interpreter_create(size_t code_size) {
    j2me_optimized_interpreter_t* interpreter = 
        (j2me_optimized_interpreter_t*)malloc(sizeof(j2me_optimized_interpreter_t));
    if (!interpreter) {
        return NULL;
    }
    
    memset(interpreter, 0, sizeof(j2me_optimized_interpreter_t));
    
    // 分配预解码指令数组
    interpreter->predecoded_code = 
        (j2me_predecoded_instruction_t*)malloc(sizeof(j2me_predecoded_instruction_t) * code_size);
    if (!interpreter->predecoded_code) {
        free(interpreter);
        return NULL;
    }
    
    interpreter->code_length = code_size;
    
    // 创建内联缓存
    interpreter->inline_cache = j2me_inline_cache_create(64); // 64个缓存条目
    if (!interpreter->inline_cache) {
        free(interpreter->predecoded_code);
        free(interpreter);
        return NULL;
    }
    
    // 创建热点检测器
    interpreter->hotspot_detector = j2me_hotspot_detector_create(1000, 100, 10); // 1000个方法，100个循环，阈值10
    if (!interpreter->hotspot_detector) {
        j2me_inline_cache_destroy(interpreter->inline_cache);
        free(interpreter->predecoded_code);
        free(interpreter);
        return NULL;
    }
    
    // 创建性能统计
    interpreter->stats = j2me_performance_stats_create();
    if (!interpreter->stats) {
        j2me_hotspot_detector_destroy(interpreter->hotspot_detector);
        j2me_inline_cache_destroy(interpreter->inline_cache);
        free(interpreter->predecoded_code);
        free(interpreter);
        return NULL;
    }
    
    // 设置默认参数
    interpreter->optimization_enabled = true;
    interpreter->batch_size = 100; // 默认批量执行100条指令
    
    // 初始化指令处理函数表
    initialize_instruction_handlers();
    
    return interpreter;
}

/**
 * @brief 销毁优化的解释器
 */
void j2me_optimized_interpreter_destroy(j2me_optimized_interpreter_t* interpreter) {
    if (interpreter) {
        if (interpreter->predecoded_code) {
            free(interpreter->predecoded_code);
        }
        if (interpreter->inline_cache) {
            j2me_inline_cache_destroy(interpreter->inline_cache);
        }
        if (interpreter->hotspot_detector) {
            j2me_hotspot_detector_destroy(interpreter->hotspot_detector);
        }
        if (interpreter->stats) {
            j2me_performance_stats_destroy(interpreter->stats);
        }
        free(interpreter);
    }
}

/**
 * @brief 预解码字节码
 */
j2me_error_t j2me_predecode_bytecode(j2me_optimized_interpreter_t* interpreter,
                                     const j2me_byte* bytecode,
                                     size_t length) {
    if (!interpreter || !bytecode || length == 0) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    size_t pc = 0;
    size_t instruction_count = 0;
    
    while (pc < length && instruction_count < interpreter->code_length) {
        j2me_predecoded_instruction_t* inst = &interpreter->predecoded_code[instruction_count];
        
        // 读取操作码
        inst->opcode = bytecode[pc++];
        inst->operand_count = 0;
        inst->flags = 0;
        
        // 设置指令处理函数
        inst->handler = (void*)instruction_handlers[inst->opcode];
        
        // 预解析操作数
        switch (inst->opcode) {
            case 0x10: // bipush
                if (pc < length) {
                    inst->operands[0] = (j2me_byte)bytecode[pc++];
                    inst->operand_count = 1;
                }
                break;
                
            case 0x11: // sipush
                if (pc + 1 < length) {
                    inst->operands[0] = (j2me_short)((bytecode[pc] << 8) | bytecode[pc + 1]);
                    pc += 2;
                    inst->operand_count = 1;
                }
                break;
                
            case 0x15: // iload
            case 0x36: // istore
                if (pc < length) {
                    inst->operands[0] = bytecode[pc++];
                    inst->operand_count = 1;
                }
                break;
                
            case 0x99: // ifeq
            case 0x9a: // ifne
            case 0xa7: // goto
                if (pc + 1 < length) {
                    inst->operands[0] = (j2me_short)((bytecode[pc] << 8) | bytecode[pc + 1]);
                    pc += 2;
                    inst->operand_count = 1;
                    inst->flags |= INST_FLAG_JUMP;
                }
                break;
                
            case 0xb6: // invokevirtual
            case 0xb7: // invokespecial
            case 0xb8: // invokestatic
                if (pc + 1 < length) {
                    inst->operands[0] = (j2me_short)((bytecode[pc] << 8) | bytecode[pc + 1]);
                    pc += 2;
                    inst->operand_count = 1;
                    inst->flags |= INST_FLAG_METHOD_CALL;
                }
                break;
                
            // 常量指令预计算
            case 0x02: // iconst_m1
                inst->operands[0] = -1;
                inst->operand_count = 1;
                break;
            case 0x03: case 0x04: case 0x05: case 0x06: case 0x07: case 0x08: // iconst_0 to iconst_5
                inst->operands[0] = inst->opcode - 0x03;
                inst->operand_count = 1;
                break;
                
            // 局部变量指令预计算索引
            case 0x1a: case 0x1b: case 0x1c: case 0x1d: // iload_0 to iload_3
                inst->operands[0] = inst->opcode - 0x1a;
                inst->operand_count = 1;
                break;
            case 0x2a: case 0x2b: case 0x2c: case 0x2d: // aload_0 to aload_3
                inst->operands[0] = inst->opcode - 0x2a;
                inst->operand_count = 1;
                break;
            case 0x3b: case 0x3c: case 0x3d: case 0x3e: // istore_0 to istore_3
                inst->operands[0] = inst->opcode - 0x3b;
                inst->operand_count = 1;
                break;
            case 0x4b: case 0x4c: case 0x4d: case 0x4e: // astore_0 to astore_3
                inst->operands[0] = inst->opcode - 0x4b;
                inst->operand_count = 1;
                break;
        }
        
        instruction_count++;
    }
    
    interpreter->code_length = instruction_count;
    return J2ME_SUCCESS;
}

/**
 * @brief 优化的字节码执行
 */
j2me_error_t j2me_execute_optimized(j2me_vm_t* vm,
                                    j2me_stack_frame_t* frame,
                                    j2me_optimized_interpreter_t* interpreter,
                                    j2me_int max_instructions) {
    if (!vm || !frame || !interpreter) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 开始性能统计
    j2me_performance_stats_start(interpreter->stats);
    
    j2me_int executed_instructions = 0;
    j2me_long start_cycles = get_current_time_us();
    
    while (executed_instructions < max_instructions && 
           frame->pc < interpreter->code_length &&
           vm->state == J2ME_VM_RUNNING) {
        
        // 批量执行优化
        if (interpreter->optimization_enabled) {
            j2me_int batch_executed = j2me_execute_batch(vm, frame, interpreter, 
                                                        frame->pc, interpreter->batch_size);
            executed_instructions += batch_executed;
            
            if (batch_executed == 0) {
                break; // 执行完毕或遇到错误
            }
        } else {
            // 单条指令执行
            if (frame->pc >= interpreter->code_length) {
                break;
            }
            
            j2me_predecoded_instruction_t* inst = &interpreter->predecoded_code[frame->pc];
            
            if (inst->handler) {
                j2me_instruction_handler_t handler = (j2me_instruction_handler_t)inst->handler;
                j2me_error_t result = handler(vm, frame, inst);
                if (result != J2ME_SUCCESS) {
                    return result;
                }
            }
            
            frame->pc++;
            executed_instructions++;
        }
    }
    
    // 记录性能统计
    j2me_long end_cycles = get_current_time_us();
    j2me_performance_stats_record_instructions(interpreter->stats, 
                                              executed_instructions, 
                                              end_cycles - start_cycles);
    
    // 结束性能统计
    j2me_performance_stats_end(interpreter->stats);
    
    return J2ME_SUCCESS;
}

/**
 * @brief 批量执行指令
 */
j2me_int j2me_execute_batch(j2me_vm_t* vm,
                           j2me_stack_frame_t* frame,
                           j2me_optimized_interpreter_t* interpreter,
                           j2me_int start_pc,
                           j2me_int batch_size) {
    j2me_int executed = 0;
    j2me_int pc = start_pc;
    
    while (executed < batch_size && 
           pc < interpreter->code_length &&
           vm->state == J2ME_VM_RUNNING) {
        
        j2me_predecoded_instruction_t* inst = &interpreter->predecoded_code[pc];
        
        // 检查是否是跳转指令，如果是则结束批量执行
        if (inst->flags & (INST_FLAG_JUMP | INST_FLAG_BRANCH | INST_FLAG_RETURN)) {
            if (inst->handler) {
                frame->pc = pc;
                j2me_instruction_handler_t handler = (j2me_instruction_handler_t)inst->handler;
                j2me_error_t result = handler(vm, frame, inst);
                if (result == J2ME_SUCCESS) {
                    executed++;
                    pc = frame->pc; // 跳转后的新PC
                }
            }
            break; // 跳转指令后结束批量执行
        }
        
        // 执行普通指令
        if (inst->handler) {
            frame->pc = pc;
            j2me_instruction_handler_t handler = (j2me_instruction_handler_t)inst->handler;
            j2me_error_t result = handler(vm, frame, inst);
            if (result != J2ME_SUCCESS) {
                break;
            }
        }
        
        pc++;
        executed++;
    }
    
    frame->pc = pc;
    return executed;
}

/**
 * @brief 创建内联缓存
 */
j2me_inline_cache_t* j2me_inline_cache_create(size_t capacity) {
    j2me_inline_cache_t* cache = (j2me_inline_cache_t*)malloc(sizeof(j2me_inline_cache_t));
    if (!cache) {
        return NULL;
    }
    
    cache->entries = (j2me_inline_cache_entry_t*)malloc(sizeof(j2me_inline_cache_entry_t) * capacity);
    if (!cache->entries) {
        free(cache);
        return NULL;
    }
    
    memset(cache->entries, 0, sizeof(j2me_inline_cache_entry_t) * capacity);
    cache->size = 0;
    cache->capacity = capacity;
    cache->hit_count = 0;
    cache->miss_count = 0;
    
    return cache;
}

/**
 * @brief 销毁内联缓存
 */
void j2me_inline_cache_destroy(j2me_inline_cache_t* cache) {
    if (cache) {
        if (cache->entries) {
            free(cache->entries);
        }
        free(cache);
    }
}

/**
 * @brief 查找内联缓存
 */
void* j2me_inline_cache_lookup(j2me_inline_cache_t* cache, j2me_int method_ref) {
    if (!cache) {
        return NULL;
    }
    
    // 线性查找 (可以优化为哈希表)
    for (size_t i = 0; i < cache->size; i++) {
        if (cache->entries[i].method_ref == method_ref) {
            cache->entries[i].call_count++;
            cache->entries[i].last_access_time = get_current_time_us();
            cache->hit_count++;
            return cache->entries[i].target_method;
        }
    }
    
    cache->miss_count++;
    return NULL;
}

/**
 * @brief 更新内联缓存
 */
j2me_error_t j2me_inline_cache_update(j2me_inline_cache_t* cache,
                                      j2me_int method_ref,
                                      void* target_method) {
    if (!cache || !target_method) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 查找是否已存在
    for (size_t i = 0; i < cache->size; i++) {
        if (cache->entries[i].method_ref == method_ref) {
            cache->entries[i].target_method = target_method;
            cache->entries[i].call_count++;
            cache->entries[i].last_access_time = get_current_time_us();
            return J2ME_SUCCESS;
        }
    }
    
    // 添加新条目
    if (cache->size < cache->capacity) {
        j2me_inline_cache_entry_t* entry = &cache->entries[cache->size++];
        entry->method_ref = method_ref;
        entry->target_method = target_method;
        entry->call_count = 1;
        entry->last_access_time = get_current_time_us();
        return J2ME_SUCCESS;
    }
    
    // 缓存已满，替换最少使用的条目
    size_t lru_index = 0;
    j2me_long oldest_time = cache->entries[0].last_access_time;
    
    for (size_t i = 1; i < cache->size; i++) {
        if (cache->entries[i].last_access_time < oldest_time) {
            oldest_time = cache->entries[i].last_access_time;
            lru_index = i;
        }
    }
    
    j2me_inline_cache_entry_t* entry = &cache->entries[lru_index];
    entry->method_ref = method_ref;
    entry->target_method = target_method;
    entry->call_count = 1;
    entry->last_access_time = get_current_time_us();
    
    return J2ME_SUCCESS;
}

/**
 * @brief 创建热点检测器
 */
j2me_hotspot_detector_t* j2me_hotspot_detector_create(size_t method_count,
                                                      size_t loop_count,
                                                      j2me_int hotspot_threshold) {
    j2me_hotspot_detector_t* detector = 
        (j2me_hotspot_detector_t*)malloc(sizeof(j2me_hotspot_detector_t));
    if (!detector) {
        return NULL;
    }
    
    detector->method_counters = (j2me_int*)calloc(method_count, sizeof(j2me_int));
    if (!detector->method_counters) {
        free(detector);
        return NULL;
    }
    
    detector->loop_counters = (j2me_int*)calloc(loop_count, sizeof(j2me_int));
    if (!detector->loop_counters) {
        free(detector->method_counters);
        free(detector);
        return NULL;
    }
    
    detector->method_count = method_count;
    detector->loop_count = loop_count;
    detector->hotspot_threshold = hotspot_threshold;
    detector->compilation_threshold = hotspot_threshold * 10; // 编译阈值是热点阈值的10倍
    
    return detector;
}

/**
 * @brief 销毁热点检测器
 */
void j2me_hotspot_detector_destroy(j2me_hotspot_detector_t* detector) {
    if (detector) {
        if (detector->method_counters) {
            free(detector->method_counters);
        }
        if (detector->loop_counters) {
            free(detector->loop_counters);
        }
        free(detector);
    }
}

/**
 * @brief 记录方法调用
 */
j2me_boolean j2me_hotspot_record_method_call(j2me_hotspot_detector_t* detector,
                                          j2me_int method_id) {
    if (!detector || method_id < 0 || method_id >= detector->method_count) {
        return false;
    }
    
    detector->method_counters[method_id]++;
    return detector->method_counters[method_id] >= detector->hotspot_threshold;
}

/**
 * @brief 记录循环执行
 */
j2me_boolean j2me_hotspot_record_loop_execution(j2me_hotspot_detector_t* detector,
                                             j2me_int loop_id) {
    if (!detector || loop_id < 0 || loop_id >= detector->loop_count) {
        return false;
    }
    
    detector->loop_counters[loop_id]++;
    return detector->loop_counters[loop_id] >= detector->hotspot_threshold;
}

/**
 * @brief 创建性能统计
 */
j2me_performance_stats_t* j2me_performance_stats_create(void) {
    j2me_performance_stats_t* stats = 
        (j2me_performance_stats_t*)malloc(sizeof(j2me_performance_stats_t));
    if (!stats) {
        return NULL;
    }
    
    memset(stats, 0, sizeof(j2me_performance_stats_t));
    return stats;
}

/**
 * @brief 销毁性能统计
 */
void j2me_performance_stats_destroy(j2me_performance_stats_t* stats) {
    if (stats) {
        free(stats);
    }
}

/**
 * @brief 开始性能统计
 */
void j2me_performance_stats_start(j2me_performance_stats_t* stats) {
    if (stats) {
        stats->start_time = get_current_time_us();
    }
}

/**
 * @brief 结束性能统计
 */
void j2me_performance_stats_end(j2me_performance_stats_t* stats) {
    if (stats) {
        stats->end_time = get_current_time_us();
    }
}

/**
 * @brief 记录指令执行
 */
void j2me_performance_stats_record_instructions(j2me_performance_stats_t* stats,
                                               j2me_int instruction_count,
                                               j2me_long cycles) {
    if (stats) {
        stats->total_instructions += instruction_count;
        stats->total_cycles += cycles;
    }
}

/**
 * @brief 记录方法调用
 */
void j2me_performance_stats_record_method_call(j2me_performance_stats_t* stats) {
    if (stats) {
        stats->method_calls++;
    }
}

/**
 * @brief 记录缓存访问
 */
void j2me_performance_stats_record_cache_access(j2me_performance_stats_t* stats,
                                                j2me_boolean hit) {
    if (stats) {
        if (hit) {
            stats->cache_hits++;
        } else {
            stats->cache_misses++;
        }
    }
}

/**
 * @brief 打印性能统计报告
 */
void j2me_performance_stats_print_report(j2me_performance_stats_t* stats) {
    if (!stats) {
        return;
    }
    
    j2me_long total_time = stats->end_time - stats->start_time;
    j2me_double instructions_per_second = j2me_performance_stats_get_instructions_per_second(stats);
    j2me_double cache_hit_rate = j2me_performance_stats_get_cache_hit_rate(stats);
    
    LOG_DEBUG("\n=== 优化解释器性能统计报告 ===\n");
    LOG_DEBUG("📊 执行统计:\n");
    LOG_DEBUG("   总指令数: %lld\n", stats->total_instructions);
    LOG_DEBUG("   总执行时间: %lld 微秒\n", total_time);
    LOG_DEBUG("   执行速度: %.2f M指令/秒\n", instructions_per_second / 1000000.0);
    LOG_DEBUG("   平均指令延迟: %.3f 微秒\n", 
           total_time > 0 ? (double)stats->total_cycles / stats->total_instructions : 0.0);
    
    LOG_DEBUG("\n📞 方法调用统计:\n");
    LOG_DEBUG("   方法调用次数: %lld\n", stats->method_calls);
    LOG_DEBUG("   平均每方法指令数: %.1f\n", 
           stats->method_calls > 0 ? (double)stats->total_instructions / stats->method_calls : 0.0);
    
    LOG_DEBUG("\n🎯 缓存统计:\n");
    LOG_DEBUG("   缓存命中: %lld\n", stats->cache_hits);
    LOG_DEBUG("   缓存未命中: %lld\n", stats->cache_misses);
    LOG_DEBUG("   缓存命中率: %.1f%%\n", cache_hit_rate * 100.0);
    
    LOG_DEBUG("\n🔥 热点编译统计:\n");
    LOG_DEBUG("   热点编译次数: %lld\n", stats->hotspot_compilations);
    
    LOG_DEBUG("\n⚡ 性能评估:\n");
    if (instructions_per_second > 500000000) {
        LOG_DEBUG("   🟢 执行性能: 优秀 (>500M指令/秒)\n");
    } else if (instructions_per_second > 100000000) {
        LOG_DEBUG("   🟡 执行性能: 良好 (>100M指令/秒)\n");
    } else {
        LOG_DEBUG("   🔴 执行性能: 需要优化 (<100M指令/秒)\n");
    }
    
    if (cache_hit_rate > 0.8) {
        LOG_DEBUG("   🟢 缓存效率: 优秀 (>80%%命中率)\n");
    } else if (cache_hit_rate > 0.6) {
        LOG_DEBUG("   🟡 缓存效率: 良好 (>60%%命中率)\n");
    } else {
        LOG_DEBUG("   🔴 缓存效率: 需要优化 (<60%%命中率)\n");
    }
}

/**
 * @brief 获取指令执行速度
 */
j2me_double j2me_performance_stats_get_instructions_per_second(j2me_performance_stats_t* stats) {
    if (!stats || stats->end_time <= stats->start_time) {
        return 0.0;
    }
    
    j2me_long total_time = stats->end_time - stats->start_time;
    return (j2me_double)stats->total_instructions * 1000000.0 / total_time;
}

/**
 * @brief 获取缓存命中率
 */
j2me_double j2me_performance_stats_get_cache_hit_rate(j2me_performance_stats_t* stats) {
    if (!stats) {
        return 0.0;
    }
    
    j2me_long total_accesses = stats->cache_hits + stats->cache_misses;
    if (total_accesses == 0) {
        return 0.0;
    }
    
    return (j2me_double)stats->cache_hits / total_accesses;
}

// ============================================================================
// 优化的指令处理函数实现
// ============================================================================

/**
 * @brief 处理NOP指令
 */
j2me_error_t j2me_handle_nop(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst) {
    // 无操作，直接返回
    return J2ME_SUCCESS;
}

/**
 * @brief 处理常量指令 (iconst_*, aconst_null)
 */
j2me_error_t j2me_handle_iconst(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst) {
    j2me_int value = 0;
    
    switch (inst->opcode) {
        case 0x01: // aconst_null
            value = 0;
            break;
        case 0x02: // iconst_m1
            value = -1;
            break;
        case 0x03: case 0x04: case 0x05: case 0x06: case 0x07: case 0x08: // iconst_0 to iconst_5
            value = inst->operands[0]; // 预计算的值
            break;
        default:
            return J2ME_ERROR_INVALID_STATE;
    }
    
    return j2me_operand_stack_push(&frame->operand_stack, value);
}

/**
 * @brief 处理BIPUSH指令
 */
j2me_error_t j2me_handle_bipush(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst) {
    return j2me_operand_stack_push(&frame->operand_stack, inst->operands[0]);
}

/**
 * @brief 处理SIPUSH指令
 */
j2me_error_t j2me_handle_sipush(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst) {
    return j2me_operand_stack_push(&frame->operand_stack, inst->operands[0]);
}

/**
 * @brief 处理ILOAD指令
 */
j2me_error_t j2me_handle_iload(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst) {
    j2me_int index = inst->operands[0]; // 预计算的索引
    
    if (index >= frame->local_vars.size) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    return j2me_operand_stack_push(&frame->operand_stack, frame->local_vars.variables[index]);
}

/**
 * @brief 处理ALOAD指令
 */
j2me_error_t j2me_handle_aload(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst) {
    j2me_int index = inst->operands[0]; // 预计算的索引
    
    if (index >= frame->local_vars.size) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    return j2me_operand_stack_push(&frame->operand_stack, frame->local_vars.variables[index]);
}

/**
 * @brief 处理ISTORE指令
 */
j2me_error_t j2me_handle_istore(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst) {
    j2me_int index = inst->operands[0]; // 预计算的索引
    j2me_int value;
    
    if (index >= frame->local_vars.size) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &value);
    if (result == J2ME_SUCCESS) {
        frame->local_vars.variables[index] = value;
    }
    
    return result;
}

/**
 * @brief 处理ASTORE指令
 */
j2me_error_t j2me_handle_astore(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst) {
    j2me_int index = inst->operands[0]; // 预计算的索引
    j2me_int value;
    
    if (index >= frame->local_vars.size) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &value);
    if (result == J2ME_SUCCESS) {
        frame->local_vars.variables[index] = value;
    }
    
    return result;
}

/**
 * @brief 处理POP指令
 */
j2me_error_t j2me_handle_pop(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst) {
    j2me_int value;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &value);
    
    // POP2指令需要弹出两个值
    if (result == J2ME_SUCCESS && inst->opcode == 0x58) {
        result = j2me_operand_stack_pop(&frame->operand_stack, &value);
    }
    
    return result;
}

/**
 * @brief 处理DUP指令
 */
j2me_error_t j2me_handle_dup(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst) {
    j2me_int value;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &value);
    
    if (result == J2ME_SUCCESS) {
        // 将值压回栈两次
        result = j2me_operand_stack_push(&frame->operand_stack, value);
        if (result == J2ME_SUCCESS) {
            result = j2me_operand_stack_push(&frame->operand_stack, value);
        }
    }
    
    return result;
}

/**
 * @brief 处理IADD指令
 */
j2me_error_t j2me_handle_iadd(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst) {
    j2me_int value1, value2;
    
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
    if (result == J2ME_SUCCESS) {
        result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
        if (result == J2ME_SUCCESS) {
            result = j2me_operand_stack_push(&frame->operand_stack, value1 + value2);
        }
    }
    
    return result;
}

/**
 * @brief 处理ISUB指令
 */
j2me_error_t j2me_handle_isub(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst) {
    j2me_int value1, value2;
    
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
    if (result == J2ME_SUCCESS) {
        result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
        if (result == J2ME_SUCCESS) {
            result = j2me_operand_stack_push(&frame->operand_stack, value1 - value2);
        }
    }
    
    return result;
}

/**
 * @brief 处理IMUL指令
 */
j2me_error_t j2me_handle_imul(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst) {
    j2me_int value1, value2;
    
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
    if (result == J2ME_SUCCESS) {
        result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
        if (result == J2ME_SUCCESS) {
            result = j2me_operand_stack_push(&frame->operand_stack, value1 * value2);
        }
    }
    
    return result;
}

/**
 * @brief 处理IDIV指令
 */
j2me_error_t j2me_handle_idiv(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst) {
    j2me_int value1, value2;
    
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &value2);
    if (result == J2ME_SUCCESS) {
        result = j2me_operand_stack_pop(&frame->operand_stack, &value1);
        if (result == J2ME_SUCCESS) {
            if (value2 == 0) {
                return J2ME_ERROR_RUNTIME_EXCEPTION; // 除零异常
            }
            result = j2me_operand_stack_push(&frame->operand_stack, value1 / value2);
        }
    }
    
    return result;
}

/**
 * @brief 处理IFEQ指令
 */
j2me_error_t j2me_handle_ifeq(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst) {
    j2me_int value;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &value);
    
    if (result == J2ME_SUCCESS) {
        if (value == 0) {
            // 跳转到目标地址
            frame->pc += inst->operands[0] - 1; // -1因为PC会在批量执行后自增
        }
    }
    
    return result;
}

/**
 * @brief 处理IFNE指令
 */
j2me_error_t j2me_handle_ifne(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst) {
    j2me_int value;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &value);
    
    if (result == J2ME_SUCCESS) {
        if (value != 0) {
            // 跳转到目标地址
            frame->pc += inst->operands[0] - 1; // -1因为PC会在批量执行后自增
        }
    }
    
    return result;
}

/**
 * @brief 处理GOTO指令
 */
j2me_error_t j2me_handle_goto(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst) {
    // 无条件跳转
    frame->pc += inst->operands[0] - 1; // -1因为PC会在批量执行后自增
    return J2ME_SUCCESS;
}

/**
 * @brief 处理IRETURN指令
 */
j2me_error_t j2me_handle_ireturn(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst) {
    // 从方法返回int值
    // 这里需要实现方法返回逻辑
    // 暂时简化处理
    frame->pc = 1000000; // 设置一个大值来结束执行
    return J2ME_SUCCESS;
}

/**
 * @brief 处理RETURN指令
 */
j2me_error_t j2me_handle_return(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst) {
    // 从void方法返回
    frame->pc = 1000000; // 设置一个大值来结束执行
    return J2ME_SUCCESS;
}

/**
 * @brief 处理INVOKEVIRTUAL指令 (优化版本)
 */
j2me_error_t j2me_handle_invokevirtual(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst) {
    j2me_int method_ref = inst->operands[0];
    
    // 尝试从内联缓存查找
    if (vm->optimized_interpreter && vm->optimized_interpreter->inline_cache) {
        void* cached_method = j2me_inline_cache_lookup(vm->optimized_interpreter->inline_cache, method_ref);
        if (cached_method) {
            // 缓存命中，直接调用
            j2me_performance_stats_record_cache_access(vm->optimized_interpreter->stats, true);
            // 这里应该调用缓存的方法
            // 暂时简化处理
            return J2ME_SUCCESS;
        } else {
            // 缓存未命中，查找方法并更新缓存
            j2me_performance_stats_record_cache_access(vm->optimized_interpreter->stats, false);
            
            // 查找方法 (简化实现)
            void* target_method = (void*)(intptr_t)method_ref; // 简化的方法指针
            
            // 更新内联缓存
            j2me_inline_cache_update(vm->optimized_interpreter->inline_cache, method_ref, target_method);
        }
    }
    
    // 记录方法调用统计
    if (vm->optimized_interpreter && vm->optimized_interpreter->stats) {
        j2me_performance_stats_record_method_call(vm->optimized_interpreter->stats);
    }
    
    // 简化的方法调用实现
    return J2ME_SUCCESS;
}

/**
 * @brief 处理INVOKESPECIAL指令 (优化版本)
 */
j2me_error_t j2me_handle_invokespecial(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst) {
    j2me_int method_ref = inst->operands[0];
    
    // 记录方法调用统计
    if (vm->optimized_interpreter && vm->optimized_interpreter->stats) {
        j2me_performance_stats_record_method_call(vm->optimized_interpreter->stats);
    }
    
    // 简化的特殊方法调用实现
    return J2ME_SUCCESS;
}

/**
 * @brief 处理INVOKESTATIC指令 (优化版本)
 */
j2me_error_t j2me_handle_invokestatic(j2me_vm_t* vm, j2me_stack_frame_t* frame, j2me_predecoded_instruction_t* inst) {
    j2me_int method_ref = inst->operands[0];
    
    // 静态方法调用可以使用内联缓存
    if (vm->optimized_interpreter && vm->optimized_interpreter->inline_cache) {
        void* cached_method = j2me_inline_cache_lookup(vm->optimized_interpreter->inline_cache, method_ref);
        if (cached_method) {
            j2me_performance_stats_record_cache_access(vm->optimized_interpreter->stats, true);
        } else {
            j2me_performance_stats_record_cache_access(vm->optimized_interpreter->stats, false);
            
            // 查找静态方法并更新缓存
            void* target_method = (void*)(intptr_t)method_ref;
            j2me_inline_cache_update(vm->optimized_interpreter->inline_cache, method_ref, target_method);
        }
    }
    
    // 记录方法调用统计
    if (vm->optimized_interpreter && vm->optimized_interpreter->stats) {
        j2me_performance_stats_record_method_call(vm->optimized_interpreter->stats);
    }
    
    // 简化的静态方法调用实现
    return J2ME_SUCCESS;
}