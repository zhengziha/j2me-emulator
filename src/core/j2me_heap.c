#include "j2me_heap.h"
#include <stdlib.h>
#include <string.h>
#include "j2me_log.h"
#include <stdio.h>

/**
 * @file j2me_heap.c
 * @brief J2ME堆内存管理实现
 */

j2me_heap_t* j2me_heap_create(size_t size) {
    j2me_heap_t* heap = (j2me_heap_t*)malloc(sizeof(j2me_heap_t));
    if (!heap) {
        LOG_DEBUG("[堆] 错误: 无法分配堆结构\n");
        return NULL;
    }
    
    // 分配堆内存
    heap->memory = (uint8_t*)malloc(size);
    if (!heap->memory) {
        LOG_DEBUG("[堆] 错误: 无法分配堆内存 (%zu bytes)\n", size);
        free(heap);
        return NULL;
    }
    
    heap->size = size;
    heap->used = 0;
    
    // 初始化对象表（初始容量256）
    heap->object_capacity = 256;
    heap->objects = (j2me_heap_object_header_t**)calloc(heap->object_capacity, sizeof(j2me_heap_object_header_t*));
    if (!heap->objects) {
        LOG_DEBUG("[堆] 错误: 无法分配对象表\n");
        free(heap->memory);
        free(heap);
        return NULL;
    }
    
    heap->object_count = 0;
    heap->next_ref = 1; // 引用从1开始，0表示NULL
    
    LOG_DEBUG("[堆] 创建堆成功: 大小=%zu bytes, 对象表容量=%zu\n", size, heap->object_capacity);
    return heap;
}

void j2me_heap_destroy(j2me_heap_t* heap) {
    if (!heap) {
        return;
    }
    
    LOG_DEBUG("[堆] 销毁堆: 已使用=%zu/%zu bytes, 对象数=%zu\n", 
           heap->used, heap->size, heap->object_count);
    
    if (heap->objects) {
        free(heap->objects);
    }
    
    if (heap->memory) {
        free(heap->memory);
    }
    
    free(heap);
}

j2me_ref_t j2me_heap_alloc(j2me_heap_t* heap, uint32_t class_id, size_t size) {
    if (!heap) {
        return J2ME_NULL_REF;
    }
    
    // 计算总大小（对象头 + 数据）
    size_t total_size = sizeof(j2me_heap_object_header_t) + size;
    
    // 检查堆空间
    if (heap->used + total_size > heap->size) {
        LOG_DEBUG("[堆] 错误: 堆空间不足 (需要=%zu, 可用=%zu)\n", 
               total_size, heap->size - heap->used);
        return J2ME_NULL_REF;
    }
    
    // 扩展对象表（如果需要）- 使用next_ref而非object_count，因为释放后object_count会减少但next_ref不会
    if (heap->next_ref >= heap->object_capacity) {
        size_t new_capacity = heap->object_capacity * 2;
        j2me_heap_object_header_t** new_objects = (j2me_heap_object_header_t**)realloc(
            heap->objects, new_capacity * sizeof(j2me_heap_object_header_t*));
        
        if (!new_objects) {
            LOG_DEBUG("[堆] 错误: 无法扩展对象表\n");
            return J2ME_NULL_REF;
        }
        
        // 清零新分配的部分
        memset(new_objects + heap->object_capacity, 0, 
               (new_capacity - heap->object_capacity) * sizeof(j2me_heap_object_header_t*));
        
        heap->objects = new_objects;
        heap->object_capacity = new_capacity;
        
        LOG_DEBUG("[堆] 对象表扩展: %zu -> %zu\n", heap->object_capacity / 2, new_capacity);
    }
    
    // 在堆上分配对象
    j2me_heap_object_header_t* obj = (j2me_heap_object_header_t*)(heap->memory + heap->used);
    obj->class_id = class_id;
    obj->size = size;
    obj->ref_count = 1;
    obj->flags = 0;
    
    // 清零对象数据
    memset(obj->data, 0, size);
    
    heap->used += total_size;
    
    // 分配引用ID
    j2me_ref_t ref = heap->next_ref++;
    
    // 存储到对象表
    heap->objects[ref] = obj;
    heap->object_count++;
    
    // LOG_DEBUG("[堆] 分配对象: ref=0x%x, class_id=%u, size=%zu, 总大小=%zu\n", 
    //        ref, class_id, size, total_size);
    
    return ref;
}

void j2me_heap_free(j2me_heap_t* heap, j2me_ref_t ref) {
    if (!heap || ref == J2ME_NULL_REF || ref >= heap->next_ref) {
        return;
    }
    
    j2me_heap_object_header_t* obj = heap->objects[ref];
    if (!obj) {
        return;
    }
    
    // LOG_DEBUG("[堆] 释放对象: ref=0x%x, class_id=%u, size=%u\n", 
    //        ref, obj->class_id, obj->size);
    
    // 从对象表中移除
    heap->objects[ref] = NULL;
    heap->object_count--;
    
    // 注意: 这里简化实现，不实际回收内存
    // 真实的GC会在垃圾回收时统一回收
}

j2me_heap_object_header_t* j2me_heap_get_object(j2me_heap_t* heap, j2me_ref_t ref) {
    if (!heap || ref == J2ME_NULL_REF || ref >= heap->next_ref) {
        return NULL;
    }
    
    return heap->objects[ref];
}

void j2me_heap_retain(j2me_heap_t* heap, j2me_ref_t ref) {
    j2me_heap_object_header_t* obj = j2me_heap_get_object(heap, ref);
    if (obj) {
        obj->ref_count++;
        LOG_DEBUG("[堆] 增加引用: ref=0x%x, ref_count=%u\n", ref, obj->ref_count);
    }
}

void j2me_heap_release(j2me_heap_t* heap, j2me_ref_t ref) {
    j2me_heap_object_header_t* obj = j2me_heap_get_object(heap, ref);
    if (obj) {
        obj->ref_count--;
        LOG_DEBUG("[堆] 减少引用: ref=0x%x, ref_count=%u\n", ref, obj->ref_count);
        
        // 如果引用计数为0，释放对象
        if (obj->ref_count == 0) {
            j2me_heap_free(heap, ref);
        }
    }
}

void* j2me_heap_get_object_data(j2me_heap_t* heap, j2me_ref_t ref) {
    j2me_heap_object_header_t* obj = j2me_heap_get_object(heap, ref);
    if (!obj) {
        return NULL;
    }
    
    return obj->data;
}

void j2me_heap_get_stats(j2me_heap_t* heap, size_t* used, size_t* total, size_t* objects) {
    if (!heap) {
        return;
    }
    
    if (used) *used = heap->used;
    if (total) *total = heap->size;
    if (objects) *objects = heap->object_count;
}

void j2me_heap_print_stats(j2me_heap_t* heap) {
    if (!heap) {
        return;
    }
    
    LOG_DEBUG("\n=== 堆统计信息 ===\n");
    LOG_DEBUG("  总大小: %zu bytes\n", heap->size);
    LOG_DEBUG("  已使用: %zu bytes (%.1f%%)\n", heap->used, 
           (double)heap->used / heap->size * 100.0);
    LOG_DEBUG("  可用: %zu bytes\n", heap->size - heap->used);
    LOG_DEBUG("  对象数: %zu / %zu\n", heap->object_count, heap->object_capacity);
    LOG_DEBUG("  下一个引用ID: 0x%x\n", heap->next_ref);
    LOG_DEBUG("==================\n\n");
}

// ============================================================================
// MIDP对象创建函数
// ============================================================================

#include "j2me_graphics.h"

j2me_ref_t j2me_heap_create_canvas(j2me_heap_t* heap, int width, int height) {
    if (!heap) {
        LOG_DEBUG("[堆] 错误: 堆指针为空\n");
        return J2ME_NULL_REF;
    }
    
    // 分配Canvas对象
    j2me_ref_t canvas_ref = j2me_heap_alloc(heap, J2ME_CLASS_ID_CANVAS, sizeof(j2me_canvas_object_t));
    if (canvas_ref == J2ME_NULL_REF) {
        LOG_DEBUG("[堆] 错误: 无法分配Canvas对象\n");
        return J2ME_NULL_REF;
    }
    
    // 获取对象数据并初始化
    j2me_canvas_object_t* canvas = (j2me_canvas_object_t*)j2me_heap_get_object_data(heap, canvas_ref);
    if (!canvas) {
        LOG_DEBUG("[堆] 错误: 无法获取Canvas对象数据\n");
        j2me_heap_free(heap, canvas_ref);
        return J2ME_NULL_REF;
    }
    
    canvas->width = width;
    canvas->height = height;
    canvas->graphics_ref = J2ME_NULL_REF;  // 稍后创建Graphics对象
    
    LOG_DEBUG("[堆] 创建Canvas对象成功: ref=0x%x, 大小=%dx%d\n", canvas_ref, width, height);
    return canvas_ref;
}

j2me_ref_t j2me_heap_create_graphics(j2me_heap_t* heap, void* context) {
    if (!heap) {
        LOG_DEBUG("[堆] 错误: 堆指针为空\n");
        return J2ME_NULL_REF;
    }
    
    if (!context) {
        LOG_DEBUG("[堆] 错误: 图形上下文为空\n");
        return J2ME_NULL_REF;
    }
    
    // 分配Graphics对象
    j2me_ref_t graphics_ref = j2me_heap_alloc(heap, J2ME_CLASS_ID_GRAPHICS, sizeof(j2me_graphics_object_t));
    if (graphics_ref == J2ME_NULL_REF) {
        LOG_DEBUG("[堆] 错误: 无法分配Graphics对象\n");
        return J2ME_NULL_REF;
    }
    
    // 获取对象数据并初始化
    j2me_graphics_object_t* graphics = (j2me_graphics_object_t*)j2me_heap_get_object_data(heap, graphics_ref);
    if (!graphics) {
        LOG_DEBUG("[堆] 错误: 无法获取Graphics对象数据\n");
        j2me_heap_free(heap, graphics_ref);
        return J2ME_NULL_REF;
    }
    
    graphics->context = context;
    graphics->color = 0x000000;  // 默认黑色
    
    LOG_DEBUG("[堆] 创建Graphics对象成功: ref=0x%x\n", graphics_ref);
    return graphics_ref;
}

bool j2me_heap_is_valid_ref(j2me_heap_t* heap, j2me_ref_t ref) {
    if (!heap || ref == J2ME_NULL_REF || ref == J2ME_INVALID_REF) {
        return false;
    }
    
    // 检查引用是否在对象表范围内
    if (ref >= heap->object_capacity) {
        return false;
    }
    
    // 检查对象是否存在
    if (heap->objects[ref] == NULL) {
        return false;
    }
    
    return true;
}
