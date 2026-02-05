#ifndef J2ME_GC_H
#define J2ME_GC_H

#include "j2me_types.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @file j2me_gc.h
 * @brief J2ME垃圾回收系统接口
 * 
 * 实现标记-清除垃圾回收器和高级内存管理功能
 */

// 前向声明
typedef struct j2me_vm j2me_vm_t;
typedef struct j2me_object j2me_object_t;

// GC统计信息
typedef struct {
    uint64_t collections;           // GC次数
    uint64_t objects_collected;     // 回收对象数
    uint64_t bytes_collected;       // 回收字节数
    uint64_t total_time_ms;         // 总GC时间(毫秒)
    uint64_t max_pause_time_ms;     // 最大暂停时间(毫秒)
    uint64_t allocations;           // 总分配次数
    uint64_t allocation_failures;   // 分配失败次数
} j2me_gc_stats_t;

// 对象标记状态
typedef enum {
    J2ME_GC_MARK_WHITE = 0,         // 白色：未标记，可回收
    J2ME_GC_MARK_GRAY = 1,          // 灰色：已标记，待扫描
    J2ME_GC_MARK_BLACK = 2          // 黑色：已标记，已扫描
} j2me_gc_mark_t;

// 内存块头部
typedef struct j2me_gc_block {
    struct j2me_gc_block* next;     // 下一个块
    struct j2me_gc_block* prev;     // 前一个块
    size_t size;                    // 块大小
    j2me_gc_mark_t mark;            // 标记状态
    bool is_free;                   // 是否空闲
    uint32_t type_id;               // 对象类型ID
} j2me_gc_block_t;

// 根对象引用
typedef struct j2me_gc_root {
    j2me_object_t** object_ref;     // 对象引用指针
    const char* description;        // 描述信息
    struct j2me_gc_root* next;      // 下一个根对象
} j2me_gc_root_t;

// 垃圾回收器
typedef struct {
    // 堆内存管理
    void* heap_start;               // 堆起始地址
    void* heap_end;                 // 堆结束地址
    size_t heap_size;               // 堆总大小
    size_t heap_used;               // 已使用内存
    size_t heap_threshold;          // GC触发阈值
    
    // 内存块链表
    j2me_gc_block_t* free_list;     // 空闲块链表
    j2me_gc_block_t* used_list;     // 已用块链表
    
    // 根对象集合
    j2me_gc_root_t* root_set;       // 根对象链表
    size_t root_count;              // 根对象数量
    
    // GC配置
    bool gc_enabled;                // GC是否启用
    bool gc_in_progress;            // GC是否正在进行
    size_t min_free_threshold;      // 最小空闲内存阈值
    
    // 统计信息
    j2me_gc_stats_t stats;          // GC统计
    
    // 虚拟机引用
    j2me_vm_t* vm;                  // 虚拟机实例
} j2me_gc_t;

/**
 * @brief 创建垃圾回收器
 * @param vm 虚拟机实例
 * @param heap_start 堆起始地址
 * @param heap_size 堆大小
 * @return 垃圾回收器实例，失败返回NULL
 */
j2me_gc_t* j2me_gc_create(j2me_vm_t* vm, void* heap_start, size_t heap_size);

/**
 * @brief 销毁垃圾回收器
 * @param gc 垃圾回收器实例
 */
void j2me_gc_destroy(j2me_gc_t* gc);

/**
 * @brief 分配对象内存
 * @param gc 垃圾回收器实例
 * @param size 对象大小
 * @param type_id 对象类型ID
 * @return 分配的内存地址，失败返回NULL
 */
void* j2me_gc_allocate(j2me_gc_t* gc, size_t size, uint32_t type_id);

/**
 * @brief 执行垃圾回收
 * @param gc 垃圾回收器实例
 * @return 错误码
 */
j2me_error_t j2me_gc_collect(j2me_gc_t* gc);

/**
 * @brief 添加根对象
 * @param gc 垃圾回收器实例
 * @param object_ref 对象引用指针
 * @param description 描述信息
 * @return 错误码
 */
j2me_error_t j2me_gc_add_root(j2me_gc_t* gc, j2me_object_t** object_ref, const char* description);

/**
 * @brief 移除根对象
 * @param gc 垃圾回收器实例
 * @param object_ref 对象引用指针
 * @return 错误码
 */
j2me_error_t j2me_gc_remove_root(j2me_gc_t* gc, j2me_object_t** object_ref);

/**
 * @brief 标记对象
 * @param gc 垃圾回收器实例
 * @param object 对象指针
 * @return 错误码
 */
j2me_error_t j2me_gc_mark_object(j2me_gc_t* gc, j2me_object_t* object);

/**
 * @brief 清除未标记对象
 * @param gc 垃圾回收器实例
 * @return 回收的字节数
 */
size_t j2me_gc_sweep(j2me_gc_t* gc);

/**
 * @brief 压缩堆内存
 * @param gc 垃圾回收器实例
 * @return 错误码
 */
j2me_error_t j2me_gc_compact(j2me_gc_t* gc);

/**
 * @brief 检查是否需要触发GC
 * @param gc 垃圾回收器实例
 * @return true表示需要GC
 */
bool j2me_gc_should_collect(j2me_gc_t* gc);

/**
 * @brief 获取GC统计信息
 * @param gc 垃圾回收器实例
 * @return GC统计信息
 */
j2me_gc_stats_t j2me_gc_get_stats(j2me_gc_t* gc);

/**
 * @brief 打印GC统计信息
 * @param gc 垃圾回收器实例
 */
void j2me_gc_print_stats(j2me_gc_t* gc);

/**
 * @brief 获取堆使用情况
 * @param gc 垃圾回收器实例
 * @param used_bytes 输出已使用字节数
 * @param free_bytes 输出空闲字节数
 * @param total_bytes 输出总字节数
 */
void j2me_gc_get_heap_info(j2me_gc_t* gc, size_t* used_bytes, size_t* free_bytes, size_t* total_bytes);

/**
 * @brief 设置GC触发阈值
 * @param gc 垃圾回收器实例
 * @param threshold 阈值（堆使用百分比，0-100）
 */
void j2me_gc_set_threshold(j2me_gc_t* gc, int threshold);

/**
 * @brief 启用或禁用GC
 * @param gc 垃圾回收器实例
 * @param enabled 是否启用
 */
void j2me_gc_set_enabled(j2me_gc_t* gc, bool enabled);

#endif // J2ME_GC_H