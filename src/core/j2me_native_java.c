#include "j2me_native_methods.h"
#include "j2me_string.h"
#include "j2me_object.h"
#include "j2me_heap.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "j2me_log.h"
#include <stdio.h>

j2me_error_t java_system_out_println(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_ref_t string_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, (j2me_int*)&string_ref);
    if (result != J2ME_SUCCESS) return result;
    j2me_int out_ref;
    result = j2me_operand_stack_pop(&frame->operand_stack, &out_ref);
    if (result != J2ME_SUCCESS) return result;
    if (vm->heap && string_ref != J2ME_NULL_REF) {
        const char* str = j2me_heap_string_get_chars(vm->heap, string_ref);
        if (str) {
            LOG_DEBUG("%s\n", str);
        } else {
            LOG_DEBUG("(null)\n");
        }
    } else {
        LOG_DEBUG("(null)\n");
    }
    return J2ME_SUCCESS;
}

j2me_error_t java_system_out_print(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_ref_t string_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, (j2me_int*)&string_ref);
    if (result != J2ME_SUCCESS) return result;
    j2me_int out_ref;
    result = j2me_operand_stack_pop(&frame->operand_stack, &out_ref);
    if (result != J2ME_SUCCESS) return result;
    if (vm->heap && string_ref != J2ME_NULL_REF) {
        const char* str = j2me_heap_string_get_chars(vm->heap, string_ref);
        if (str) {
            LOG_DEBUG("%s", str);
        } else {
            LOG_DEBUG("(null)");
        }
    } else {
        LOG_DEBUG("(null)");
    }
    return J2ME_SUCCESS;
}

j2me_error_t java_thread_start(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_ref_t thread_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, (j2me_int*)&thread_ref);
    if (result != J2ME_SUCCESS) return result;
    void* runnable_obj = NULL;
    if (vm->current_runnable_ref != 0) {
        runnable_obj = j2me_heap_get_object(vm->heap, vm->current_runnable_ref);
    }
    void* target_obj = runnable_obj ? runnable_obj : j2me_heap_get_object(vm->heap, thread_ref);
    if (!target_obj) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_thread_t* vm_thread = j2me_vm_create_thread(vm, target_obj, runnable_obj);
    if (!vm_thread) return J2ME_ERROR_OUT_OF_MEMORY;
    result = j2me_vm_start_thread(vm, vm_thread);
    return result;
}

j2me_error_t java_thread_sleep(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int millis_low, millis_high;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &millis_low);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &millis_high);
    if (result != J2ME_SUCCESS) return result;
    return J2ME_SUCCESS;
}

j2me_error_t java_thread_yield(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    return J2ME_SUCCESS;
}

j2me_error_t java_thread_current_thread(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    if (vm->current_thread && vm->current_thread->thread_object) {
        j2me_ref_t thread_ref = (j2me_ref_t)(uintptr_t)vm->current_thread->thread_object;
        j2me_operand_stack_push(&frame->operand_stack, thread_ref);
    } else {
        j2me_operand_stack_push(&frame->operand_stack, J2ME_NULL_REF);
    }
    return J2ME_SUCCESS;
}

j2me_error_t java_object_get_class(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int object_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &object_ref);
    if (result != J2ME_SUCCESS) return result;
    j2me_int class_ref = J2ME_NULL_REF;
    if (vm->heap && object_ref != J2ME_NULL_REF) {
        j2me_heap_object_header_t* obj = j2me_heap_get_object(vm->heap, (j2me_ref_t)object_ref);
        if (obj && obj->size >= sizeof(j2me_class_t*)) {
            j2me_class_t* cls = *((j2me_class_t**)obj->data);
            if (cls) class_ref = (j2me_int)(uintptr_t)cls;
        }
    }
    return j2me_operand_stack_push(&frame->operand_stack, class_ref);
}

j2me_error_t java_object_hash_code(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int object_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &object_ref);
    if (result != J2ME_SUCCESS) return result;
    return j2me_operand_stack_push(&frame->operand_stack, object_ref);
}

j2me_error_t java_object_notify(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int object_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &object_ref);
    return result;
}

j2me_error_t java_object_notify_all(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int object_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &object_ref);
    return result;
}

j2me_error_t java_object_wait(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int timeout_low, timeout_high, object_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &timeout_low);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &timeout_high);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &object_ref);
    return result;
}

j2me_error_t java_system_current_time_millis(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    int64_t millis = (int64_t)(ts.tv_sec) * 1000LL + (int64_t)(ts.tv_nsec) / 1000000LL;
    j2me_int millis_high = (j2me_int)((millis >> 32) & 0xFFFFFFFF);
    j2me_int millis_low = (j2me_int)(millis & 0xFFFFFFFF);
    j2me_error_t result = j2me_operand_stack_push(&frame->operand_stack, millis_high);
    if (result != J2ME_SUCCESS) return result;
    return j2me_operand_stack_push(&frame->operand_stack, millis_low);
}

j2me_error_t java_system_arraycopy(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int length, dst_offset, dst_ref, src_offset, src_ref;
    j2me_error_t result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &length);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &dst_offset);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &dst_ref);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &src_offset);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &src_ref);
    if (result != J2ME_SUCCESS) return result;
    if (src_ref == J2ME_NULL_REF || dst_ref == J2ME_NULL_REF || length <= 0) return J2ME_SUCCESS;
    if (vm->heap) {
        j2me_heap_object_header_t* src_obj = j2me_heap_get_object(vm->heap, (j2me_ref_t)src_ref);
        j2me_heap_object_header_t* dst_obj = j2me_heap_get_object(vm->heap, (j2me_ref_t)dst_ref);
        if (src_obj && dst_obj) {
            uint32_t src_len = *(uint32_t*)src_obj->data;
            uint32_t dst_len = *(uint32_t*)dst_obj->data;
            uint8_t* src_data = src_obj->data + sizeof(uint32_t);
            uint8_t* dst_data = dst_obj->data + sizeof(uint32_t);
            int copy_bytes = length * sizeof(j2me_int);
            int src_start = src_offset * sizeof(j2me_int);
            int dst_start = dst_offset * sizeof(j2me_int);
            if (src_start + copy_bytes <= (int)(src_len * sizeof(j2me_int)) &&
                dst_start + copy_bytes <= (int)(dst_len * sizeof(j2me_int))) {
                memmove(dst_data + dst_start, src_data + src_start, copy_bytes);
            }
        }
    }
    return J2ME_SUCCESS;
}

j2me_error_t java_system_identity_hash_code(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int object_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &object_ref);
    if (result != J2ME_SUCCESS) return result;
    return j2me_operand_stack_push(&frame->operand_stack, object_ref);
}

j2me_error_t java_system_get_property0(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int key_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &key_ref);
    if (result != J2ME_SUCCESS) return result;
    const char* value = "";
    if (vm->heap && key_ref != J2ME_NULL_REF) {
        const char* key = j2me_heap_string_get_chars(vm->heap, (j2me_ref_t)key_ref);
        if (key) {
            if (strcmp(key, "microedition.platform") == 0) value = "phoneME-X/1.0";
            else if (strcmp(key, "microedition.encoding") == 0) value = "UTF-8";
            else if (strcmp(key, "microedition.locale") == 0) value = "zh-CN";
            else if (strcmp(key, "microedition.configuration") == 0) value = "CLDC-1.1";
            else if (strcmp(key, "microedition.profiles") == 0) value = "MIDP-2.0";
            else if (strcmp(key, "line.separator") == 0) value = "\n";
            else if (strcmp(key, "file.separator") == 0) value = "/";
            else if (strcmp(key, "path.separator") == 0) value = ":";
        }
    }
    j2me_ref_t result_ref = J2ME_NULL_REF;
    if (vm->heap) result_ref = j2me_heap_string_create(vm->heap, value);
    return j2me_operand_stack_push(&frame->operand_stack, (j2me_int)result_ref);
}

j2me_error_t java_string_equals(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int other_ref, this_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &other_ref);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &this_ref);
    if (result != J2ME_SUCCESS) return result;
    j2me_int equals = 0;
    if (vm->heap && this_ref != J2ME_NULL_REF && other_ref != J2ME_NULL_REF) {
        const char* this_str = j2me_heap_string_get_chars(vm->heap, (j2me_ref_t)this_ref);
        const char* other_str = j2me_heap_string_get_chars(vm->heap, (j2me_ref_t)other_ref);
        if (this_str && other_str) equals = (strcmp(this_str, other_str) == 0) ? 1 : 0;
    } else if (this_ref == other_ref) {
        equals = 1;
    }
    return j2me_operand_stack_push(&frame->operand_stack, equals);
}

j2me_error_t java_string_hash_code(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int this_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &this_ref);
    if (result != J2ME_SUCCESS) return result;
    j2me_int hash = 0;
    if (vm->heap && this_ref != J2ME_NULL_REF) {
        const char* str = j2me_heap_string_get_chars(vm->heap, (j2me_ref_t)this_ref);
        if (str) {
            int h = 0;
            for (const char* p = str; *p; p++) h = 31 * h + (int)(*p);
            hash = h;
        }
    }
    return j2me_operand_stack_push(&frame->operand_stack, hash);
}

j2me_error_t java_string_index_of(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int ch, this_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &ch);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &this_ref);
    if (result != J2ME_SUCCESS) return result;
    j2me_int idx = -1;
    if (vm->heap && this_ref != J2ME_NULL_REF) {
        const char* str = j2me_heap_string_get_chars(vm->heap, (j2me_ref_t)this_ref);
        if (str) {
            const char* found = strchr(str, (char)(ch & 0xFFFF));
            if (found) idx = (j2me_int)(found - str);
        }
    }
    return j2me_operand_stack_push(&frame->operand_stack, idx);
}

j2me_error_t java_string_index_of_from(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int from_index, ch, this_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &from_index);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &ch);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &this_ref);
    if (result != J2ME_SUCCESS) return result;
    j2me_int idx = -1;
    if (vm->heap && this_ref != J2ME_NULL_REF) {
        const char* str = j2me_heap_string_get_chars(vm->heap, (j2me_ref_t)this_ref);
        if (str && from_index >= 0) {
            const char* found = strchr(str + from_index, (char)(ch & 0xFFFF));
            if (found) idx = (j2me_int)(found - str);
        }
    }
    return j2me_operand_stack_push(&frame->operand_stack, idx);
}

j2me_error_t java_string_last_index_of(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int ch, this_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &ch);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &this_ref);
    if (result != J2ME_SUCCESS) return result;
    j2me_int idx = -1;
    if (vm->heap && this_ref != J2ME_NULL_REF) {
        const char* str = j2me_heap_string_get_chars(vm->heap, (j2me_ref_t)this_ref);
        if (str) {
            const char* last = strrchr(str, (char)(ch & 0xFFFF));
            if (last) idx = (j2me_int)(last - str);
        }
    }
    return j2me_operand_stack_push(&frame->operand_stack, idx);
}

j2me_error_t java_string_last_index_of_from(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int from_index, ch, this_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &from_index);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &ch);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &this_ref);
    if (result != J2ME_SUCCESS) return result;
    j2me_int idx = -1;
    if (vm->heap && this_ref != J2ME_NULL_REF) {
        const char* str = j2me_heap_string_get_chars(vm->heap, (j2me_ref_t)this_ref);
        if (str && from_index >= 0) {
            int len = (int)strlen(str);
            int start = (from_index < len) ? from_index : len - 1;
            char c = (char)(ch & 0xFFFF);
            for (int i = start; i >= 0; i--) {
                if (str[i] == c) { idx = i; break; }
            }
        }
    }
    return j2me_operand_stack_push(&frame->operand_stack, idx);
}

j2me_error_t java_string_intern(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int this_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &this_ref);
    if (result != J2ME_SUCCESS) return result;
    return j2me_operand_stack_push(&frame->operand_stack, this_ref);
}

static j2me_error_t java_math_double_op(j2me_vm_t* vm, j2me_stack_frame_t* frame, double (*op)(double)) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int val_low, val_high;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &val_low);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &val_high);
    if (result != J2ME_SUCCESS) return result;
    double d;
    int64_t bits = ((int64_t)val_high << 32) | ((int64_t)val_low & 0xFFFFFFFF);
    memcpy(&d, &bits, sizeof(double));
    double ret = op(d);
    int64_t ret_bits;
    memcpy(&ret_bits, &ret, sizeof(double));
    result = j2me_operand_stack_push(&frame->operand_stack, (j2me_int)((ret_bits >> 32) & 0xFFFFFFFF));
    if (result != J2ME_SUCCESS) return result;
    return j2me_operand_stack_push(&frame->operand_stack, (j2me_int)(ret_bits & 0xFFFFFFFF));
}

j2me_error_t java_math_sin(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) { return java_math_double_op(vm, frame, sin); }
j2me_error_t java_math_cos(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) { return java_math_double_op(vm, frame, cos); }
j2me_error_t java_math_tan(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) { return java_math_double_op(vm, frame, tan); }
j2me_error_t java_math_sqrt(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) { return java_math_double_op(vm, frame, sqrt); }
j2me_error_t java_math_ceil(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) { return java_math_double_op(vm, frame, ceil); }
j2me_error_t java_math_floor(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) { return java_math_double_op(vm, frame, floor); }

j2me_error_t java_runtime_gc(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int runtime_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &runtime_ref);
    if (result != J2ME_SUCCESS) return result;
    if (vm->gc) vm->gc_collections++;
    return J2ME_SUCCESS;
}

j2me_error_t java_runtime_free_memory(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int runtime_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &runtime_ref);
    if (result != J2ME_SUCCESS) return result;
    int64_t free_mem = 0;
    if (vm->heap) {
        size_t used, total, objects;
        j2me_heap_get_stats(vm->heap, &used, &total, &objects);
        free_mem = (int64_t)(total - used);
    }
    result = j2me_operand_stack_push(&frame->operand_stack, (j2me_int)((free_mem >> 32) & 0xFFFFFFFF));
    if (result != J2ME_SUCCESS) return result;
    return j2me_operand_stack_push(&frame->operand_stack, (j2me_int)(free_mem & 0xFFFFFFFF));
}

j2me_error_t java_runtime_total_memory(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int runtime_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &runtime_ref);
    if (result != J2ME_SUCCESS) return result;
    int64_t total_mem = 0;
    if (vm->heap) {
        size_t used, total, objects;
        j2me_heap_get_stats(vm->heap, &used, &total, &objects);
        total_mem = (int64_t)total;
    }
    result = j2me_operand_stack_push(&frame->operand_stack, (j2me_int)((total_mem >> 32) & 0xFFFFFFFF));
    if (result != J2ME_SUCCESS) return result;
    return j2me_operand_stack_push(&frame->operand_stack, (j2me_int)(total_mem & 0xFFFFFFFF));
}

j2me_error_t java_runtime_exit_internal(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int status, runtime_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &status);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &runtime_ref);
    if (result != J2ME_SUCCESS) return result;
    vm->state = J2ME_VM_TERMINATED;
    return J2ME_SUCCESS;
}

j2me_error_t java_throwable_print_stack_trace(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int throwable_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &throwable_ref);
    if (result != J2ME_SUCCESS) return result;
    if (vm->current_thread) {
        j2me_stack_frame_t* current = vm->current_thread->current_frame;
        int depth = 0;
        while (current && depth < 20) {
            j2me_method_t* method = (j2me_method_t*)current->method_info;
            if (method && method->name) {
                printf("  at %s%s (PC=%d)\n",
                       method->owner_class ? (method->owner_class->name ? method->owner_class->name : "?") : "?",
                       method->name, current->pc);
            }
            current = current->previous;
            depth++;
        }
    }
    return J2ME_SUCCESS;
}

j2me_error_t java_throwable_fill_in_stack_trace(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int throwable_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &throwable_ref);
    return result;
}
