#include "j2me_native_methods.h"
#include "j2me_class.h"
#include "j2me_heap.h"
#include "j2me_object.h"
#include "j2me_string.h"
#include <stdlib.h>
#include <string.h>
#include "j2me_log.h"
#include <stdio.h>

j2me_error_t java_class_for_name(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int name_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &name_ref);
    if (result != J2ME_SUCCESS) return result;

    j2me_int class_ref = J2ME_NULL_REF;
    if (vm->heap && name_ref != J2ME_NULL_REF && vm->class_loader) {
        const char* name = j2me_heap_string_get_chars(vm->heap, (j2me_ref_t)name_ref);
        if (name) {
            j2me_class_t* cls = j2me_class_loader_find_class(vm->class_loader, name);
            if (!cls) {
                cls = j2me_class_loader_load_class(vm->class_loader, name);
            }
            if (cls) {
                class_ref = (j2me_int)(uintptr_t)cls;
            }
        }
    }

    return j2me_operand_stack_push(&frame->operand_stack, class_ref);
}

j2me_error_t java_class_new_instance(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int class_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &class_ref);
    if (result != J2ME_SUCCESS) return result;

    j2me_int obj_ref = J2ME_NULL_REF;
    if (class_ref != J2ME_NULL_REF && vm->heap) {
        j2me_class_t* cls = (j2me_class_t*)(uintptr_t)class_ref;
        size_t obj_size = sizeof(j2me_class_t*) + (cls->instance_size > 0 ? cls->instance_size : 16);
        j2me_ref_t ref = j2me_heap_alloc(vm->heap, 0, obj_size);
        if (ref != J2ME_NULL_REF) {
            void* data = j2me_heap_get_object_data(vm->heap, ref);
            if (data) {
                *((j2me_class_t**)data) = cls;
            }
            obj_ref = (j2me_int)ref;
        }
    }

    return j2me_operand_stack_push(&frame->operand_stack, obj_ref);
}

j2me_error_t java_class_is_instance(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int obj_ref, class_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &obj_ref);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &class_ref);
    if (result != J2ME_SUCCESS) return result;

    j2me_int is_instance = 0;
    if (class_ref != J2ME_NULL_REF && obj_ref != J2ME_NULL_REF && vm->heap) {
        j2me_class_t* target_cls = (j2me_class_t*)(uintptr_t)class_ref;
        j2me_heap_object_header_t* obj = j2me_heap_get_object(vm->heap, (j2me_ref_t)obj_ref);
        if (obj && obj->size >= sizeof(j2me_class_t*)) {
            j2me_class_t* obj_cls = *((j2me_class_t**)obj->data);
            if (obj_cls && target_cls) {
                j2me_class_t* c = obj_cls;
                while (c) {
                    if (c == target_cls) { is_instance = 1; break; }
                    c = c->super_class_ptr;
                }
            }
        }
    }

    return j2me_operand_stack_push(&frame->operand_stack, is_instance);
}

j2me_error_t java_class_is_assignable_from(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int sub_class_ref, this_class_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &sub_class_ref);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &this_class_ref);
    if (result != J2ME_SUCCESS) return result;

    j2me_int assignable = 0;
    j2me_class_t* sub_cls = (j2me_class_t*)(uintptr_t)sub_class_ref;
    j2me_class_t* this_cls = (j2me_class_t*)(uintptr_t)this_class_ref;
    if (sub_cls && this_cls) {
        j2me_class_t* c = sub_cls;
        while (c) {
            if (c == this_cls) { assignable = 1; break; }
            c = c->super_class_ptr;
        }
    }

    return j2me_operand_stack_push(&frame->operand_stack, assignable);
}

j2me_error_t java_class_is_interface(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int class_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &class_ref);
    if (result != J2ME_SUCCESS) return result;

    j2me_int is_iface = 0;
    j2me_class_t* cls = (j2me_class_t*)(uintptr_t)class_ref;
    if (cls && (cls->access_flags & ACC_INTERFACE)) {
        is_iface = 1;
    }

    return j2me_operand_stack_push(&frame->operand_stack, is_iface);
}

j2me_error_t java_class_is_array(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int class_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &class_ref);
    if (result != J2ME_SUCCESS) return result;

    j2me_int is_arr = 0;
    j2me_class_t* cls = (j2me_class_t*)(uintptr_t)class_ref;
    if (cls && cls->name && cls->name[0] == '[') {
        is_arr = 1;
    }

    return j2me_operand_stack_push(&frame->operand_stack, is_arr);
}

j2me_error_t java_class_get_name(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int class_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &class_ref);
    if (result != J2ME_SUCCESS) return result;

    j2me_int name_ref = J2ME_NULL_REF;
    j2me_class_t* cls = (j2me_class_t*)(uintptr_t)class_ref;
    if (cls && cls->name && vm->heap) {
        name_ref = (j2me_int)j2me_heap_string_create(vm->heap, cls->name);
    }

    return j2me_operand_stack_push(&frame->operand_stack, name_ref);
}

j2me_error_t java_class_get_superclass(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int class_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &class_ref);
    if (result != J2ME_SUCCESS) return result;

    j2me_int super_ref = J2ME_NULL_REF;
    j2me_class_t* cls = (j2me_class_t*)(uintptr_t)class_ref;
    if (cls && cls->super_class_ptr) {
        super_ref = (j2me_int)(uintptr_t)cls->super_class_ptr;
    }

    return j2me_operand_stack_push(&frame->operand_stack, super_ref);
}

j2me_error_t java_class_invoke_clinit(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int class_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &class_ref);
    if (result != J2ME_SUCCESS) return result;

    j2me_class_t* cls = (j2me_class_t*)(uintptr_t)class_ref;
    if (cls && cls->clinit && cls->clinit->bytecode && cls->clinit->bytecode_length > 0) {
        j2me_interpreter_execute_method(vm, cls->clinit, NULL, NULL);
    }
    cls->state = CLASS_INITIALIZED;

    return J2ME_SUCCESS;
}

j2me_error_t java_class_init9(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int class_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &class_ref);
    if (result != J2ME_SUCCESS) return result;
    return J2ME_SUCCESS;
}

j2me_error_t java_class_invoke_verify(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int class_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &class_ref);
    if (result != J2ME_SUCCESS) return result;
    return J2ME_SUCCESS;
}
