#ifndef J2ME_HEAP_H
#define J2ME_HEAP_H

#include "j2me_types.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @file j2me_heap.h
 * @brief J2ME堆内存管理系统
 * 
 * 实现真实的对象堆分配、引用管理和垃圾回收
 */

// 堆对象头结构（与旧的j2me_object_header_t区分）
typedef struct {
    uint32_t class_id;      // 类ID
    uint32_t size;          // 对象大小（字节）
    uint32_t ref_count;     // 引用计数（用于简单GC）
    uint32_t flags;         // 标志位（GC标记等）
    uint8_t data[];         // 对象数据（柔性数组）
} j2me_heap_object_header_t;

// 堆管理结构
typedef struct {
    uint8_t* memory;                        // 堆内存起始地址
    size_t size;                            // 堆总大小
    size_t used;                            // 已使用大小
    j2me_heap_object_header_t** objects;    // 对象表（引用 -> 对象指针）
    size_t object_capacity;                 // 对象表容量
    size_t object_count;                    // 当前对象数量
    uint32_t next_ref;                      // 下一个可用引用ID
} j2me_heap_t;

// 对象引用类型（引用 = 对象表索引）
typedef uint32_t j2me_ref_t;

// 特殊引用值
#define J2ME_NULL_REF 0                 // 空引用
#define J2ME_INVALID_REF 0xFFFFFFFF     // 无效引用

/**
 * @brief 创建堆
 * @param size 堆大小（字节）
 * @return 堆指针，失败返回NULL
 */
j2me_heap_t* j2me_heap_create(size_t size);

/**
 * @brief 销毁堆
 * @param heap 堆指针
 */
void j2me_heap_destroy(j2me_heap_t* heap);

/**
 * @brief 在堆上分配对象
 * @param heap 堆指针
 * @param class_id 类ID
 * @param size 对象数据大小（不包括对象头）
 * @return 对象引用，失败返回J2ME_NULL_REF
 */
j2me_ref_t j2me_heap_alloc(j2me_heap_t* heap, uint32_t class_id, size_t size);

/**
 * @brief 释放对象
 * @param heap 堆指针
 * @param ref 对象引用
 */
void j2me_heap_free(j2me_heap_t* heap, j2me_ref_t ref);

/**
 * @brief 获取对象指针
 * @param heap 堆指针
 * @param ref 对象引用
 * @return 对象指针，失败返回NULL
 */
j2me_heap_object_header_t* j2me_heap_get_object(j2me_heap_t* heap, j2me_ref_t ref);

/**
 * @brief 增加对象引用计数
 * @param heap 堆指针
 * @param ref 对象引用
 */
void j2me_heap_retain(j2me_heap_t* heap, j2me_ref_t ref);

/**
 * @brief 减少对象引用计数（可能触发释放）
 * @param heap 堆指针
 * @param ref 对象引用
 */
void j2me_heap_release(j2me_heap_t* heap, j2me_ref_t ref);

/**
 * @brief 获取对象数据指针
 * @param heap 堆指针
 * @param ref 对象引用
 * @return 对象数据指针，失败返回NULL
 */
void* j2me_heap_get_object_data(j2me_heap_t* heap, j2me_ref_t ref);

/**
 * @brief 获取堆使用统计
 * @param heap 堆指针
 * @param used 输出已使用大小
 * @param total 输出总大小
 * @param objects 输出对象数量
 */
void j2me_heap_get_stats(j2me_heap_t* heap, size_t* used, size_t* total, size_t* objects);

/**
 * @brief 打印堆状态（调试用）
 * @param heap 堆指针
 */
void j2me_heap_print_stats(j2me_heap_t* heap);

// ============================================================================
// MIDP对象类型定义
// ============================================================================

// 前向声明
struct j2me_graphics_context;

// 类ID定义
#define J2ME_CLASS_ID_STRING   0x0001
#define J2ME_CLASS_ID_CANVAS   0x0002
#define J2ME_CLASS_ID_GRAPHICS 0x0003

/**
 * @brief Canvas对象数据结构（在堆上分配）
 */
typedef struct {
    int width;                  // Canvas宽度
    int height;                 // Canvas高度
    j2me_ref_t graphics_ref;    // 关联的Graphics对象引用
} j2me_canvas_object_t;

/**
 * @brief Graphics对象数据结构（在堆上分配）
 */
typedef struct {
    void* context;  // SDL图形上下文指针（使用void*避免类型冲突）
    int color;      // 当前颜色（RGB格式）
} j2me_graphics_object_t;

/**
 * @brief 创建Canvas对象
 * @param heap 堆指针
 * @param width Canvas宽度
 * @param height Canvas高度
 * @return Canvas对象引用，失败返回J2ME_NULL_REF
 */
j2me_ref_t j2me_heap_create_canvas(j2me_heap_t* heap, int width, int height);

/**
 * @brief 创建Graphics对象
 * @param heap 堆指针
 * @param context SDL图形上下文
 * @return Graphics对象引用，失败返回J2ME_NULL_REF
 */
j2me_ref_t j2me_heap_create_graphics(j2me_heap_t* heap, void* context);

/**
 * @brief 检查引用是否有效
 * @param heap 堆指针
 * @param ref 对象引用
 * @return true表示有效，false表示无效
 */
bool j2me_heap_is_valid_ref(j2me_heap_t* heap, j2me_ref_t ref);

#endif // J2ME_HEAP_H
