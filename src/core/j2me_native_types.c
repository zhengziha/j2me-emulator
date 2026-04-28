#include "j2me_native_methods.h"
#include "j2me_heap.h"
#include <stdlib.h>
#include <string.h>
#include "j2me_log.h"
#include <stdio.h>

j2me_error_t java_float_to_int_bits(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int val_low, val_high;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &val_low);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &val_high);
    if (result != J2ME_SUCCESS) return result;
    float f;
    int32_t bits;
    int64_t combined = ((int64_t)val_high << 32) | ((int64_t)val_low & 0xFFFFFFFF);
    double d;
    memcpy(&d, &combined, sizeof(double));
    f = (float)d;
    memcpy(&bits, &f, sizeof(float));
    return j2me_operand_stack_push(&frame->operand_stack, (j2me_int)bits);
}

j2me_error_t java_float_to_raw_int_bits(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    return java_float_to_int_bits(vm, frame, args);
}

j2me_error_t java_float_int_bits_to_float(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int bits;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &bits);
    if (result != J2ME_SUCCESS) return result;
    float f;
    int32_t ibits = (int32_t)bits;
    memcpy(&f, &ibits, sizeof(float));
    double d = (double)f;
    int64_t ret_bits;
    memcpy(&ret_bits, &d, sizeof(double));
    result = j2me_operand_stack_push(&frame->operand_stack, (j2me_int)((ret_bits >> 32) & 0xFFFFFFFF));
    if (result != J2ME_SUCCESS) return result;
    return j2me_operand_stack_push(&frame->operand_stack, (j2me_int)(ret_bits & 0xFFFFFFFF));
}

j2me_error_t java_double_to_long_bits(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int val_low, val_high;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &val_low);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &val_high);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_push(&frame->operand_stack, val_high);
    if (result != J2ME_SUCCESS) return result;
    return j2me_operand_stack_push(&frame->operand_stack, val_low);
}

j2me_error_t java_double_to_raw_long_bits(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    return java_double_to_long_bits(vm, frame, args);
}

j2me_error_t java_double_long_bits_to_double(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    if (!vm || !frame) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_int val_low, val_high;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &val_low);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &val_high);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_push(&frame->operand_stack, val_high);
    if (result != J2ME_SUCCESS) return result;
    return j2me_operand_stack_push(&frame->operand_stack, val_low);
}
