#include "j2me_native_methods.h"
#include "j2me_heap.h"
#include "j2me_object.h"
#include <stdlib.h>
#include <string.h>
#include "j2me_log.h"
#include <stdio.h>

j2me_error_t java_thread_start0(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int thread_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &thread_ref);
    if (result != J2ME_SUCCESS) return result;

    void* runnable_obj = NULL;
    if (vm->current_runnable_ref != 0) {
        runnable_obj = j2me_heap_get_object(vm->heap, vm->current_runnable_ref);
    }
    void* target_obj = runnable_obj ? runnable_obj : j2me_heap_get_object(vm->heap, (j2me_ref_t)thread_ref);
    if (!target_obj) return J2ME_ERROR_INVALID_PARAMETER;

    j2me_thread_t* vm_thread = j2me_vm_create_thread(vm, target_obj, runnable_obj);
    if (!vm_thread) return J2ME_ERROR_OUT_OF_MEMORY;

    return j2me_vm_start_thread(vm, vm_thread);
}

j2me_error_t java_thread_is_alive(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int thread_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &thread_ref);
    if (result != J2ME_SUCCESS) return result;

    j2me_int alive = 0;
    if (vm->thread_list && thread_ref != J2ME_NULL_REF) {
        j2me_thread_t* t = vm->thread_list;
        while (t) {
            if (t->thread_object && (j2me_int)(uintptr_t)t->thread_object == thread_ref) {
                alive = t->is_running ? 1 : 0;
                break;
            }
            t = t->next;
        }
    }

    return j2me_operand_stack_push(&frame->operand_stack, alive);
}

j2me_error_t java_thread_active_count(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    return j2me_operand_stack_push(&frame->operand_stack, (j2me_int)vm->thread_count);
}

j2me_error_t java_thread_set_priority0(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int new_priority, old_priority, thread_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &new_priority);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &old_priority);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &thread_ref);
    if (result != J2ME_SUCCESS) return result;
    return J2ME_SUCCESS;
}

j2me_error_t java_thread_interrupt0(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int thread_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &thread_ref);
    return result;
}

j2me_error_t java_thread_internal_exit(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int thread_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &thread_ref);
    if (result != J2ME_SUCCESS) return result;

    if (vm->current_thread) {
        vm->current_thread->is_running = false;
    }

    return J2ME_SUCCESS;
}

j2me_error_t java_socket_open0(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int port, ip_bytes_ref, protocol_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &port);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &ip_bytes_ref);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &protocol_ref);
    if (result != J2ME_SUCCESS) return result;
    return J2ME_SUCCESS;
}

j2me_error_t java_socket_read0(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int len, off, data_ref, protocol_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &len);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &off);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &data_ref);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &protocol_ref);
    if (result != J2ME_SUCCESS) return result;
    return j2me_operand_stack_push(&frame->operand_stack, -1);
}

j2me_error_t java_socket_write0(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int len, off, data_ref, protocol_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &len);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &off);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &data_ref);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &protocol_ref);
    if (result != J2ME_SUCCESS) return result;
    return j2me_operand_stack_push(&frame->operand_stack, len);
}

j2me_error_t java_socket_available0(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int protocol_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &protocol_ref);
    if (result != J2ME_SUCCESS) return result;
    return j2me_operand_stack_push(&frame->operand_stack, 0);
}

j2me_error_t java_socket_close0(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int protocol_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &protocol_ref);
    return result;
}

j2me_error_t java_socket_finalize(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int protocol_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &protocol_ref);
    return result;
}
