#include "j2me_gc.h"
#include "j2me_vm.h"
#include "j2me_object.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/**
 * @file j2me_gc.c
 * @brief J2ME垃圾回收系统实现
 * 
 * 实现标记-清除垃圾回收器和高级内存管理功能
 */

// 内部辅助函数声明
static j2me_gc_block_t* j2me_gc_find_free_block(j2me_gc_t* gc, size_t size);
static j2me_gc_block_t* j2me_gc_split_block(j2me_gc_block_t* block, size_t size);
static void j2me_gc_merge_free_blocks(j2me_gc_t* gc);
static void j2me_gc_add_to_free_list(j2me_gc_t* gc, j2me_gc_block_t* block);
static void j2me_gc_remove_from_free_list(j2me_gc_t* gc, j2me_gc_block_t* block);
static void j2me_gc_add_to_used_list(j2me_gc_t* gc, j2me_gc_block_t* block);
static void j2me_gc_remove_from_used_list(j2me_gc_t* gc, j2me_gc_block_t* block);
static uint64_t j2me_gc_get_time_ms(void);

j2me_gc_t* j2me_gc_create(j2me_vm_t* vm, void* heap_start, size_t heap_size) {
    if (!vm || !heap_start || heap_size < sizeof(j2me_gc_block_t)) {
        return NULL;
    }
    
    j2me_gc_t* gc = (j2me_gc_t*)malloc(sizeof(j2me_gc_t));
    if (!gc) {
        return NULL;
    }
    
    // 初始化基本字段
    memset(gc, 0, sizeof(j2me_gc_t));
    gc->vm = vm;
    gc->heap_start = heap_start;
    gc->heap_end = (char*)heap_start + heap_size;
    gc->heap_size = heap_size;
    gc->heap_used = 0;
    gc->heap_threshold = heap_size * 80 / 100; // 默认80%触发GC
    gc->min_free_threshold = heap_size * 10 / 100; // 最小10%空闲
    gc->gc_enabled = true;
    gc->gc_in_progress = false;
    
    // 创建初始空闲块
    j2me_gc_block_t* initial_block = (j2me_gc_block_t*)heap_start;
    initial_block->next = NULL;
    initial_block->prev = NULL;
    initial_block->size = heap_size - sizeof(j2me_gc_block_t);
    initial_block->mark = J2ME_GC_MARK_WHITE;
    initial_block->is_free = true;
    initial_block->type_id = 0;
    
    gc->free_list = initial_block;
    gc->used_list = NULL;
    gc->root_set = NULL;
    gc->root_count = 0;
    
    printf("[GC] 垃圾回收器创建成功，堆大小: %zu bytes\n", heap_size);
    return gc;
}

void j2me_gc_destroy(j2me_gc_t* gc) {
    if (!gc) {
        return;
    }
    
    // 清理根对象链表
    j2me_gc_root_t* root = gc->root_set;
    while (root) {
        j2me_gc_root_t* next = root->next;
        free(root);
        root = next;
    }
    
    // 打印最终统计信息
    j2me_gc_print_stats(gc);
    
    free(gc);
    printf("[GC] 垃圾回收器已销毁\n");
}

void* j2me_gc_allocate(j2me_gc_t* gc, size_t size, uint32_t type_id) {
    if (!gc || size == 0) {
        return NULL;
    }
    
    // 对齐到8字节边界
    size = (size + 7) & ~7;
    
    // 检查是否需要GC
    if (j2me_gc_should_collect(gc)) {
        printf("[GC] 触发垃圾回收，当前使用: %zu/%zu bytes\n", gc->heap_used, gc->heap_size);
        j2me_gc_collect(gc);
    }
    
    // 查找合适的空闲块
    j2me_gc_block_t* block = j2me_gc_find_free_block(gc, size);
    if (!block) {
        // 尝试强制GC
        if (gc->gc_enabled && !gc->gc_in_progress) {
            printf("[GC] 内存不足，强制垃圾回收\n");
            j2me_gc_collect(gc);
            block = j2me_gc_find_free_block(gc, size);
        }
        
        if (!block) {
            gc->stats.allocation_failures++;
            printf("[GC] 错误: 内存分配失败，请求大小: %zu bytes\n", size);
            return NULL;
        }
    }
    
    // 分割块（如果需要）
    if (block->size > size + sizeof(j2me_gc_block_t) + 32) {
        j2me_gc_block_t* new_block = j2me_gc_split_block(block, size);
        if (new_block) {
            j2me_gc_add_to_free_list(gc, new_block);
        }
    }
    
    // 从空闲链表移除并添加到已用链表
    j2me_gc_remove_from_free_list(gc, block);
    block->is_free = false;
    block->mark = J2ME_GC_MARK_WHITE;
    block->type_id = type_id;
    j2me_gc_add_to_used_list(gc, block);
    
    // 更新统计信息
    gc->heap_used += block->size + sizeof(j2me_gc_block_t);
    gc->stats.allocations++;
    
    // 返回数据区域（跳过头部）
    void* data = (char*)block + sizeof(j2me_gc_block_t);
    memset(data, 0, size); // 清零内存
    
    printf("[GC] 分配内存: %zu bytes, 地址: %p, 类型ID: %u\n", size, data, type_id);
    return data;
}

j2me_error_t j2me_gc_collect(j2me_gc_t* gc) {
    if (!gc || !gc->gc_enabled || gc->gc_in_progress) {
        return J2ME_ERROR_INVALID_STATE;
    }
    
    uint64_t start_time = j2me_gc_get_time_ms();
    gc->gc_in_progress = true;
    
    printf("[GC] 开始垃圾回收...\n");
    
    // 第一阶段：标记所有对象为白色
    j2me_gc_block_t* block = gc->used_list;
    while (block) {
        block->mark = J2ME_GC_MARK_WHITE;
        block = block->next;
    }
    
    // 第二阶段：从根对象开始标记
    j2me_gc_root_t* root = gc->root_set;
    int marked_roots = 0;
    while (root) {
        if (root->object_ref && *(root->object_ref)) {
            j2me_error_t result = j2me_gc_mark_object(gc, *(root->object_ref));
            if (result == J2ME_SUCCESS) {
                marked_roots++;
            }
        }
        root = root->next;
    }
    
    printf("[GC] 标记阶段完成，标记根对象: %d个\n", marked_roots);
    
    // 第三阶段：清除未标记对象
    size_t bytes_collected = j2me_gc_sweep(gc);
    
    // 第四阶段：合并空闲块
    j2me_gc_merge_free_blocks(gc);
    
    // 更新统计信息
    uint64_t end_time = j2me_gc_get_time_ms();
    uint64_t pause_time = end_time - start_time;
    
    gc->stats.collections++;
    gc->stats.bytes_collected += bytes_collected;
    gc->stats.total_time_ms += pause_time;
    if (pause_time > gc->stats.max_pause_time_ms) {
        gc->stats.max_pause_time_ms = pause_time;
    }
    
    gc->gc_in_progress = false;
    
    printf("[GC] 垃圾回收完成，回收: %zu bytes, 耗时: %llu ms\n", 
           bytes_collected, (unsigned long long)pause_time);
    
    return J2ME_SUCCESS;
}

j2me_error_t j2me_gc_add_root(j2me_gc_t* gc, struct j2me_object** object_ref, const char* description) {
    if (!gc || !object_ref) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 检查是否已存在
    j2me_gc_root_t* existing = gc->root_set;
    while (existing) {
        if (existing->object_ref == object_ref) {
            return J2ME_SUCCESS; // 已存在
        }
        existing = existing->next;
    }
    
    // 创建新根对象
    j2me_gc_root_t* root = (j2me_gc_root_t*)malloc(sizeof(j2me_gc_root_t));
    if (!root) {
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    root->object_ref = object_ref;
    root->description = description;
    root->next = gc->root_set;
    gc->root_set = root;
    gc->root_count++;
    
    printf("[GC] 添加根对象: %s, 总数: %zu\n", description ? description : "未知", gc->root_count);
    return J2ME_SUCCESS;
}

j2me_error_t j2me_gc_remove_root(j2me_gc_t* gc, struct j2me_object** object_ref) {
    if (!gc || !object_ref) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    j2me_gc_root_t* prev = NULL;
    j2me_gc_root_t* current = gc->root_set;
    
    while (current) {
        if (current->object_ref == object_ref) {
            if (prev) {
                prev->next = current->next;
            } else {
                gc->root_set = current->next;
            }
            
            printf("[GC] 移除根对象: %s\n", current->description ? current->description : "未知");
            free(current);
            gc->root_count--;
            return J2ME_SUCCESS;
        }
        prev = current;
        current = current->next;
    }
    
    return J2ME_ERROR_INVALID_PARAMETER; // 未找到
}

j2me_error_t j2me_gc_mark_object(j2me_gc_t* gc, struct j2me_object* object) {
    if (!gc || !object) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 获取对象的内存块头部
    j2me_gc_block_t* block = (j2me_gc_block_t*)((char*)object - sizeof(j2me_gc_block_t));
    
    // 检查块是否在堆范围内
    if ((void*)block < gc->heap_start || (void*)block >= gc->heap_end) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 如果已经标记为黑色，跳过
    if (block->mark == J2ME_GC_MARK_BLACK) {
        return J2ME_SUCCESS;
    }
    
    // 标记为黑色
    block->mark = J2ME_GC_MARK_BLACK;
    
    // TODO: 递归标记对象引用的其他对象
    // 这里需要根据对象类型遍历其字段中的引用
    
    return J2ME_SUCCESS;
}

size_t j2me_gc_sweep(j2me_gc_t* gc) {
    if (!gc) {
        return 0;
    }
    
    size_t bytes_collected = 0;
    int objects_collected = 0;
    
    j2me_gc_block_t* current = gc->used_list;
    j2me_gc_block_t* prev = NULL;
    
    while (current) {
        j2me_gc_block_t* next = current->next;
        
        if (current->mark == J2ME_GC_MARK_WHITE) {
            // 未标记的对象，需要回收
            bytes_collected += current->size + sizeof(j2me_gc_block_t);
            objects_collected++;
            
            // 从已用链表移除
            if (prev) {
                prev->next = next;
            } else {
                gc->used_list = next;
            }
            if (next) {
                next->prev = prev;
            }
            
            // 添加到空闲链表
            current->is_free = true;
            current->mark = J2ME_GC_MARK_WHITE;
            current->type_id = 0;
            j2me_gc_add_to_free_list(gc, current);
            
            // 不更新prev，因为当前节点已被移除
        } else {
            prev = current;
        }
        
        current = next;
    }
    
    // 更新统计信息
    gc->heap_used -= bytes_collected;
    gc->stats.objects_collected += objects_collected;
    
    printf("[GC] 清除阶段完成，回收对象: %d个, 回收内存: %zu bytes\n", 
           objects_collected, bytes_collected);
    
    return bytes_collected;
}

bool j2me_gc_should_collect(j2me_gc_t* gc) {
    if (!gc || !gc->gc_enabled) {
        return false;
    }
    
    // 检查堆使用率是否超过阈值
    if (gc->heap_used >= gc->heap_threshold) {
        return true;
    }
    
    // 检查空闲内存是否低于最小阈值
    size_t free_bytes = gc->heap_size - gc->heap_used;
    if (free_bytes < gc->min_free_threshold) {
        return true;
    }
    
    return false;
}

j2me_gc_stats_t j2me_gc_get_stats(j2me_gc_t* gc) {
    if (!gc) {
        j2me_gc_stats_t empty_stats = {0};
        return empty_stats;
    }
    
    return gc->stats;
}

void j2me_gc_print_stats(j2me_gc_t* gc) {
    if (!gc) {
        return;
    }
    
    printf("\n=== GC统计信息 ===\n");
    printf("GC次数: %llu\n", (unsigned long long)gc->stats.collections);
    printf("回收对象数: %llu\n", (unsigned long long)gc->stats.objects_collected);
    printf("回收字节数: %llu\n", (unsigned long long)gc->stats.bytes_collected);
    printf("总GC时间: %llu ms\n", (unsigned long long)gc->stats.total_time_ms);
    printf("最大暂停时间: %llu ms\n", (unsigned long long)gc->stats.max_pause_time_ms);
    printf("总分配次数: %llu\n", (unsigned long long)gc->stats.allocations);
    printf("分配失败次数: %llu\n", (unsigned long long)gc->stats.allocation_failures);
    
    size_t used_bytes, free_bytes, total_bytes;
    j2me_gc_get_heap_info(gc, &used_bytes, &free_bytes, &total_bytes);
    printf("堆使用情况: %zu/%zu bytes (%.1f%%)\n", 
           used_bytes, total_bytes, (double)used_bytes * 100.0 / total_bytes);
    printf("根对象数量: %zu\n", gc->root_count);
    printf("==================\n\n");
}

void j2me_gc_get_heap_info(j2me_gc_t* gc, size_t* used_bytes, size_t* free_bytes, size_t* total_bytes) {
    if (!gc) {
        if (used_bytes) *used_bytes = 0;
        if (free_bytes) *free_bytes = 0;
        if (total_bytes) *total_bytes = 0;
        return;
    }
    
    if (used_bytes) *used_bytes = gc->heap_used;
    if (free_bytes) *free_bytes = gc->heap_size - gc->heap_used;
    if (total_bytes) *total_bytes = gc->heap_size;
}

void j2me_gc_set_threshold(j2me_gc_t* gc, int threshold) {
    if (!gc || threshold < 0 || threshold > 100) {
        return;
    }
    
    gc->heap_threshold = gc->heap_size * threshold / 100;
    printf("[GC] GC触发阈值设置为: %d%% (%zu bytes)\n", threshold, gc->heap_threshold);
}

void j2me_gc_set_enabled(j2me_gc_t* gc, bool enabled) {
    if (!gc) {
        return;
    }
    
    gc->gc_enabled = enabled;
    printf("[GC] 垃圾回收%s\n", enabled ? "已启用" : "已禁用");
}

// 内部辅助函数实现

static j2me_gc_block_t* j2me_gc_find_free_block(j2me_gc_t* gc, size_t size) {
    j2me_gc_block_t* current = gc->free_list;
    j2me_gc_block_t* best_fit = NULL;
    
    // 首次适应算法
    while (current) {
        if (current->is_free && current->size >= size) {
            if (!best_fit || current->size < best_fit->size) {
                best_fit = current;
            }
        }
        current = current->next;
    }
    
    return best_fit;
}

static j2me_gc_block_t* j2me_gc_split_block(j2me_gc_block_t* block, size_t size) {
    if (!block || block->size <= size + sizeof(j2me_gc_block_t)) {
        return NULL;
    }
    
    // 创建新块
    j2me_gc_block_t* new_block = (j2me_gc_block_t*)((char*)block + sizeof(j2me_gc_block_t) + size);
    new_block->size = block->size - size - sizeof(j2me_gc_block_t);
    new_block->is_free = true;
    new_block->mark = J2ME_GC_MARK_WHITE;
    new_block->type_id = 0;
    new_block->next = NULL;
    new_block->prev = NULL;
    
    // 调整原块大小
    block->size = size;
    
    return new_block;
}

static void j2me_gc_merge_free_blocks(j2me_gc_t* gc) {
    j2me_gc_block_t* current = gc->free_list;
    int merged_count = 0;
    
    while (current && current->next) {
        j2me_gc_block_t* next = current->next;
        
        // 检查是否相邻
        char* current_end = (char*)current + sizeof(j2me_gc_block_t) + current->size;
        if (current_end == (char*)next && next->is_free) {
            // 合并块
            current->size += sizeof(j2me_gc_block_t) + next->size;
            current->next = next->next;
            if (next->next) {
                next->next->prev = current;
            }
            merged_count++;
        } else {
            current = next;
        }
    }
    
    if (merged_count > 0) {
        printf("[GC] 合并空闲块: %d个\n", merged_count);
    }
}

static void j2me_gc_add_to_free_list(j2me_gc_t* gc, j2me_gc_block_t* block) {
    block->next = gc->free_list;
    block->prev = NULL;
    if (gc->free_list) {
        gc->free_list->prev = block;
    }
    gc->free_list = block;
}

static void j2me_gc_remove_from_free_list(j2me_gc_t* gc, j2me_gc_block_t* block) {
    if (block->prev) {
        block->prev->next = block->next;
    } else {
        gc->free_list = block->next;
    }
    if (block->next) {
        block->next->prev = block->prev;
    }
    block->next = NULL;
    block->prev = NULL;
}

static void j2me_gc_add_to_used_list(j2me_gc_t* gc, j2me_gc_block_t* block) {
    block->next = gc->used_list;
    block->prev = NULL;
    if (gc->used_list) {
        gc->used_list->prev = block;
    }
    gc->used_list = block;
}

static void j2me_gc_remove_from_used_list(j2me_gc_t* gc, j2me_gc_block_t* block) {
    if (block->prev) {
        block->prev->next = block->next;
    } else {
        gc->used_list = block->next;
    }
    if (block->next) {
        block->next->prev = block->prev;
    }
    block->next = NULL;
    block->prev = NULL;
}

static uint64_t j2me_gc_get_time_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    }
    return 0;
}