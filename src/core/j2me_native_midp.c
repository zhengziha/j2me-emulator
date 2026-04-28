#include "j2me_native_methods.h"
#include "j2me_graphics.h"
#include "j2me_string.h"
#include "j2me_heap.h"
#include <stdlib.h>
#include <string.h>
#include "j2me_log.h"
#include <stdio.h>
#include <SDL2/SDL.h>

j2me_error_t midp_display_get_display(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    LOG_DEBUG("[MIDP Display] getDisplay() 调用\n");
    j2me_int display_ref = 0x10000001;
    if (frame->operand_stack.top >= frame->operand_stack.size) {
        return J2ME_ERROR_STACK_OVERFLOW;
    }
    j2me_error_t result = j2me_operand_stack_push(&frame->operand_stack, display_ref);
    if (result != J2ME_SUCCESS) {
        LOG_DEBUG("[MIDP Display] 错误: 压栈失败，栈顶=%zu，栈大小=%zu\n", frame->operand_stack.top, frame->operand_stack.size);
        return result;
    }
    LOG_DEBUG("[MIDP Display] getDisplay() 返回Display对象: 0x%x\n", display_ref);
    return J2ME_SUCCESS;
}

j2me_error_t midp_display_set_current(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    LOG_DEBUG("[MIDP Display] setCurrent() 调用\n");
    if (frame->operand_stack.top < 2) {
        if (frame->operand_stack.top >= 1) {
            j2me_int displayable_ref;
            j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &displayable_ref);
            if (result == J2ME_SUCCESS) {
                if (vm && displayable_ref != 0) {
                    vm->current_canvas_ref = displayable_ref;
                    j2me_stack_frame_t* temp_frame = j2me_stack_frame_create(10, 5);
                    if (temp_frame) {
                        j2me_operand_stack_push(&temp_frame->operand_stack, displayable_ref);
                        midp_canvas_repaint(vm, temp_frame, NULL);
                        j2me_stack_frame_destroy(temp_frame);
                    }
                    j2me_stack_frame_t* service_frame = j2me_stack_frame_create(10, 5);
                    if (service_frame) {
                        j2me_operand_stack_push(&service_frame->operand_stack, displayable_ref);
                        midp_canvas_service_repaints(vm, service_frame, NULL);
                        j2me_stack_frame_destroy(service_frame);
                    }
                }
                return J2ME_SUCCESS;
            }
        }
        return J2ME_SUCCESS;
    }
    j2me_int displayable_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &displayable_ref);
    if (result != J2ME_SUCCESS) return result;
    j2me_int display_ref;
    result = j2me_operand_stack_pop(&frame->operand_stack, &display_ref);
    if (result != J2ME_SUCCESS) return result;
    if (vm && displayable_ref != 0) {
        vm->current_canvas_ref = displayable_ref;
        j2me_stack_frame_t* temp_frame = j2me_stack_frame_create(10, 5);
        if (temp_frame) {
            j2me_operand_stack_push(&temp_frame->operand_stack, displayable_ref);
            midp_canvas_repaint(vm, temp_frame, NULL);
            j2me_stack_frame_destroy(temp_frame);
        }
        j2me_stack_frame_t* service_frame = j2me_stack_frame_create(10, 5);
        if (service_frame) {
            j2me_operand_stack_push(&service_frame->operand_stack, displayable_ref);
            midp_canvas_service_repaints(vm, service_frame, NULL);
            j2me_stack_frame_destroy(service_frame);
        }
    }
    return J2ME_SUCCESS;
}

j2me_error_t midp_display_get_current(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int display_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &display_ref);
    if (result != J2ME_SUCCESS) return result;
    j2me_int current_displayable = 0x20000001;
    return j2me_operand_stack_push(&frame->operand_stack, current_displayable);
}

static j2me_error_t find_canvas_paint_method(j2me_vm_t* vm, j2me_int canvas_ref, j2me_class_t** canvas_class, j2me_method_t** paint_method) {
    if (!vm || !canvas_class || !paint_method) return J2ME_ERROR_INVALID_PARAMETER;
    *canvas_class = NULL;
    *paint_method = NULL;
    return J2ME_ERROR_METHOD_NOT_FOUND;
}

j2me_error_t call_canvas_paint_method(j2me_vm_t* vm, j2me_int canvas_ref, j2me_int graphics_ref) {
    if (!vm || !vm->heap) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_heap_object_header_t* heap_obj = j2me_heap_get_object(vm->heap, canvas_ref);
    if (!heap_obj) {
        if (vm->display && vm->display->context) {
            j2me_color_t bg_color = {255, 255, 255, 255};
            j2me_graphics_set_color(vm->display->context, bg_color);
            j2me_graphics_draw_rect(vm->display->context, 0, 0, 240, 320, true);
        }
        return J2ME_SUCCESS;
    }
    j2me_class_t* canvas_class = NULL;
    if (heap_obj->size >= sizeof(void*)) {
        canvas_class = *((j2me_class_t**)heap_obj->data);
    }
    if (!canvas_class) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_method_t* paint_method = NULL;
    for (int i = 0; i < canvas_class->methods_count; i++) {
        j2me_method_t* method = &canvas_class->methods[i];
        if (strcmp(method->name, "paint") == 0 && strstr(method->descriptor, "Graphics") != NULL) {
            paint_method = method;
            break;
        }
    }
    if (!paint_method || !paint_method->bytecode || paint_method->bytecode_length == 0) {
        if (vm->display && vm->display->context) {
            j2me_color_t bg_color = {255, 255, 255, 255};
            j2me_graphics_set_color(vm->display->context, bg_color);
            j2me_graphics_draw_rect(vm->display->context, 0, 0, 240, 320, true);
        }
        return J2ME_SUCCESS;
    }
    return j2me_interpreter_execute_method(vm, paint_method, heap_obj, NULL);
}

static j2me_error_t midp_canvas_call_paint_method(j2me_vm_t* vm, j2me_int canvas_ref) {
    if (!vm || !vm->heap) return J2ME_ERROR_INVALID_PARAMETER;
    j2me_ref_t graphics_ref = J2ME_NULL_REF;
    if (vm->display && vm->display->context) {
        graphics_ref = j2me_heap_create_graphics(vm->heap, vm->display->context);
        if (graphics_ref == J2ME_NULL_REF) graphics_ref = 0x40000001;
    } else {
        graphics_ref = 0x40000001;
    }
    j2me_error_t result = call_canvas_paint_method(vm, canvas_ref, graphics_ref);
    if (j2me_heap_is_valid_ref(vm->heap, graphics_ref)) {
        j2me_heap_release(vm->heap, graphics_ref);
    }
    return result;
}

j2me_error_t midp_canvas_repaint(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int canvas_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &canvas_ref);
    if (result != J2ME_SUCCESS) return result;
    if (vm && vm->display && vm->display->context) {
        SDL_SetRenderTarget(vm->display->context->renderer, vm->display->context->canvas);
        SDL_SetRenderDrawColor(vm->display->context->renderer, 255, 255, 255, 255);
        SDL_RenderClear(vm->display->context->renderer);
        midp_canvas_call_paint_method(vm, canvas_ref);
        SDL_SetRenderTarget(vm->display->context->renderer, NULL);
        SDL_Rect dest_rect = {0, 0, 240, 320};
        SDL_RenderCopy(vm->display->context->renderer, vm->display->context->canvas, NULL, &dest_rect);
        j2me_display_refresh(vm->display);
    }
    return J2ME_SUCCESS;
}

j2me_error_t midp_canvas_service_repaints(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int canvas_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &canvas_ref);
    if (result != J2ME_SUCCESS) return result;
    if (vm && vm->display && vm->display->context) {
        SDL_SetRenderTarget(vm->display->context->renderer, vm->display->context->canvas);
        SDL_SetRenderDrawColor(vm->display->context->renderer, 255, 255, 255, 255);
        SDL_RenderClear(vm->display->context->renderer);
        midp_canvas_call_paint_method(vm, canvas_ref);
        SDL_SetRenderTarget(vm->display->context->renderer, NULL);
        SDL_RenderCopy(vm->display->context->renderer, vm->display->context->canvas, NULL, NULL);
        j2me_display_refresh(vm->display);
    }
    return J2ME_SUCCESS;
}

j2me_error_t midp_canvas_get_width(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int canvas_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &canvas_ref);
    if (result != J2ME_SUCCESS) return result;
    j2me_int width = vm && vm->display ? vm->display->screen_width : 240;
    return j2me_operand_stack_push(&frame->operand_stack, width);
}

j2me_error_t midp_canvas_get_height(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int canvas_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &canvas_ref);
    if (result != J2ME_SUCCESS) return result;
    j2me_int height = vm && vm->display ? vm->display->screen_height : 320;
    return j2me_operand_stack_push(&frame->operand_stack, height);
}

j2me_error_t midp_graphics_set_color_rgb(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int blue, green, red, graphics_ref;
    j2me_error_t result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &blue);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &green);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &red);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &graphics_ref);
    if (result != J2ME_SUCCESS) return result;
    if (vm && vm->display && vm->display->context) {
        j2me_color_t color = {
            .r = (red < 0) ? 0 : ((red > 255) ? 255 : red),
            .g = (green < 0) ? 0 : ((green > 255) ? 255 : green),
            .b = (blue < 0) ? 0 : ((blue > 255) ? 255 : blue),
            .a = 255
        };
        j2me_graphics_set_color(vm->display->context, color);
    }
    return J2ME_SUCCESS;
}

j2me_error_t midp_graphics_set_color(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int rgb, graphics_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &rgb);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &graphics_ref);
    if (result != J2ME_SUCCESS) return result;
    if (vm && vm->display && vm->display->context) {
        j2me_color_t color = { .r = (rgb >> 16) & 0xFF, .g = (rgb >> 8) & 0xFF, .b = rgb & 0xFF, .a = 255 };
        j2me_graphics_set_color(vm->display->context, color);
    }
    return J2ME_SUCCESS;
}

j2me_error_t midp_graphics_get_color(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int graphics_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &graphics_ref);
    if (result != J2ME_SUCCESS) return result;
    j2me_int color = 0x000000;
    if (vm && vm->display && vm->display->context) {
        j2me_color_t current_color = vm->display->context->current_color;
        color = (current_color.r << 16) | (current_color.g << 8) | current_color.b;
    }
    return j2me_operand_stack_push(&frame->operand_stack, color);
}

j2me_error_t midp_graphics_draw_line(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int y2, x2, y1, x1, graphics_ref;
    j2me_error_t result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &y2);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &x2);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &y1);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &x1);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &graphics_ref);
    if (result != J2ME_SUCCESS) return result;
    if (vm && vm->display && vm->display->context) {
        j2me_graphics_draw_line(vm->display->context, x1, y1, x2, y2);
    }
    return J2ME_SUCCESS;
}

j2me_error_t midp_graphics_draw_rect(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int height, width, y, x, graphics_ref;
    j2me_error_t result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &height);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &width);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &y);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &x);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &graphics_ref);
    if (result != J2ME_SUCCESS) return result;
    if (vm && vm->display && vm->display->context) {
        j2me_graphics_draw_rect(vm->display->context, x, y, width, height, false);
    }
    return J2ME_SUCCESS;
}

j2me_error_t midp_graphics_fill_rect(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int height, width, y, x, graphics_ref;
    j2me_error_t result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &height);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &width);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &y);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &x);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &graphics_ref);
    if (result != J2ME_SUCCESS) return result;
    if (vm && vm->display && vm->display->context) {
        j2me_graphics_draw_rect(vm->display->context, x, y, width, height, true);
    }
    return J2ME_SUCCESS;
}

j2me_error_t midp_graphics_draw_string(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int anchor, y, x, string_ref, graphics_ref;
    j2me_error_t result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &anchor);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &y);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &x);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &string_ref);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &graphics_ref);
    if (result != J2ME_SUCCESS) return result;
    if (vm && vm->display && vm->display->context) {
        j2me_graphics_draw_string(vm->display->context, "Sample Text", x, y, anchor);
    }
    return J2ME_SUCCESS;
}

j2me_error_t midp_graphics_draw_oval(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int height, width, y, x, graphics_ref;
    j2me_error_t result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &height);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &width);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &y);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &x);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &graphics_ref);
    if (result != J2ME_SUCCESS) return result;
    if (vm && vm->display && vm->display->context) {
        j2me_graphics_draw_oval(vm->display->context, x, y, width, height, false);
    }
    return J2ME_SUCCESS;
}

j2me_error_t midp_graphics_fill_oval(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int height, width, y, x, graphics_ref;
    j2me_error_t result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &height);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &width);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &y);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &x);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &graphics_ref);
    if (result != J2ME_SUCCESS) return result;
    if (vm && vm->display && vm->display->context) {
        j2me_graphics_draw_oval(vm->display->context, x, y, width, height, true);
    }
    return J2ME_SUCCESS;
}

j2me_error_t midp_graphics_draw_arc(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int arc_angle, start_angle, height, width, y, x, graphics_ref;
    j2me_error_t result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &arc_angle);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &start_angle);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &height);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &width);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &y);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &x);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &graphics_ref);
    if (result != J2ME_SUCCESS) return result;
    if (vm && vm->display && vm->display->context) {
        j2me_graphics_draw_arc(vm->display->context, x, y, width, height, start_angle, arc_angle, false);
    }
    return J2ME_SUCCESS;
}

j2me_error_t midp_image_create_image(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int height, width;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &height);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &width);
    if (result != J2ME_SUCCESS) return result;
    j2me_image_t* image = NULL;
    if (vm && vm->display && vm->display->context) {
        image = j2me_image_create(vm->display->context, width, height);
    }
    j2me_int image_ref = image ? (j2me_int)(uintptr_t)image : 0x50000001;
    return j2me_operand_stack_push(&frame->operand_stack, image_ref);
}

j2me_error_t midp_image_create_image_from_file(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int filename_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &filename_ref);
    if (result != J2ME_SUCCESS) return result;
    const char* filename = "test_image.png";
    j2me_image_t* image = NULL;
    if (vm && vm->display && vm->display->context) {
        image = j2me_image_load(vm->display->context, filename);
    }
    j2me_int image_ref = image ? (j2me_int)(uintptr_t)image : 0x50000002;
    return j2me_operand_stack_push(&frame->operand_stack, image_ref);
}

j2me_error_t midp_image_get_width(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int image_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &image_ref);
    if (result != J2ME_SUCCESS) return result;
    j2me_int width = 64;
    if (image_ref > 0x50000000) {
        j2me_image_t* image = (j2me_image_t*)(uintptr_t)image_ref;
        if (image) width = image->width;
    }
    return j2me_operand_stack_push(&frame->operand_stack, width);
}

j2me_error_t midp_image_get_height(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int image_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &image_ref);
    if (result != J2ME_SUCCESS) return result;
    j2me_int height = 64;
    if (image_ref > 0x50000000) {
        j2me_image_t* image = (j2me_image_t*)(uintptr_t)image_ref;
        if (image) height = image->height;
    }
    return j2me_operand_stack_push(&frame->operand_stack, height);
}

j2me_error_t midp_graphics_draw_image(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int anchor, y, x, image_ref, graphics_ref;
    j2me_error_t result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &anchor);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &y);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &x);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &image_ref);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &graphics_ref);
    if (result != J2ME_SUCCESS) return result;
    if (vm && vm->display && vm->display->context && image_ref > 0) {
        j2me_image_t* image = (j2me_image_t*)(uintptr_t)image_ref;
        if (image) j2me_graphics_draw_image(vm->display->context, image, x, y, anchor);
    }
    return J2ME_SUCCESS;
}

j2me_error_t midp_canvas_key_pressed(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int key_code, canvas_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &key_code);
    if (result != J2ME_SUCCESS) return result;
    return j2me_operand_stack_pop(&frame->operand_stack, &canvas_ref);
}

j2me_error_t midp_canvas_key_released(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int key_code, canvas_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &key_code);
    if (result != J2ME_SUCCESS) return result;
    return j2me_operand_stack_pop(&frame->operand_stack, &canvas_ref);
}

j2me_error_t midp_canvas_pointer_pressed(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int y, x, canvas_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &y);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &x);
    if (result != J2ME_SUCCESS) return result;
    return j2me_operand_stack_pop(&frame->operand_stack, &canvas_ref);
}

j2me_error_t midp_canvas_pointer_released(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int y, x, canvas_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &y);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &x);
    if (result != J2ME_SUCCESS) return result;
    return j2me_operand_stack_pop(&frame->operand_stack, &canvas_ref);
}

j2me_error_t midp_canvas_pointer_dragged(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int y, x, canvas_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &y);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &x);
    if (result != J2ME_SUCCESS) return result;
    return j2me_operand_stack_pop(&frame->operand_stack, &canvas_ref);
}

j2me_error_t midp_midlet_platform_request(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int url_ref, midlet_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &url_ref);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &midlet_ref);
    if (result != J2ME_SUCCESS) return result;
    return j2me_operand_stack_push(&frame->operand_stack, 0);
}

j2me_error_t midp_midlet_destroy_app(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int unconditional, midlet_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &unconditional);
    if (result != J2ME_SUCCESS) return result;
    return j2me_operand_stack_pop(&frame->operand_stack, &midlet_ref);
}

j2me_error_t midp_midlet_notify_destroyed(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int midlet_ref;
    return j2me_operand_stack_pop(&frame->operand_stack, &midlet_ref);
}

j2me_error_t java_string_length(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int string_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &string_ref);
    if (result != J2ME_SUCCESS) return result;
    j2me_int length = 0;
    if (vm->heap && string_ref != J2ME_NULL_REF) {
        const char* str = j2me_heap_string_get_chars(vm->heap, (j2me_ref_t)string_ref);
        if (str) length = (j2me_int)strlen(str);
    }
    return j2me_operand_stack_push(&frame->operand_stack, length);
}

j2me_error_t java_string_char_at(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int index, string_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &index);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &string_ref);
    if (result != J2ME_SUCCESS) return result;
    j2me_int character = 0;
    if (vm->heap && string_ref != J2ME_NULL_REF) {
        const char* str = j2me_heap_string_get_chars(vm->heap, (j2me_ref_t)string_ref);
        if (str && index >= 0 && index < (j2me_int)strlen(str)) {
            character = (j2me_int)(unsigned char)str[index];
        }
    }
    return j2me_operand_stack_push(&frame->operand_stack, character);
}

j2me_error_t java_string_substring(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int end_index, start_index, string_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &end_index);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &start_index);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &string_ref);
    if (result != J2ME_SUCCESS) return result;
    j2me_int new_ref = J2ME_NULL_REF;
    if (vm->heap && string_ref != J2ME_NULL_REF) {
        const char* str = j2me_heap_string_get_chars(vm->heap, (j2me_ref_t)string_ref);
        if (str) {
            int len = (int)strlen(str);
            if (start_index >= 0 && end_index >= start_index && end_index <= len) {
                int sub_len = end_index - start_index;
                char* buf = (char*)malloc(sub_len + 1);
                if (buf) {
                    memcpy(buf, str + start_index, sub_len);
                    buf[sub_len] = '\0';
                    new_ref = (j2me_int)j2me_heap_string_create(vm->heap, buf);
                    free(buf);
                }
            }
        }
    }
    return j2me_operand_stack_push(&frame->operand_stack, new_ref);
}

j2me_error_t midp_graphics_draw_round_rect(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int arc_height, arc_width, height, width, y, x, graphics_ref;
    j2me_error_t result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &arc_height);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &arc_width);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &height);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &width);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &y);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &x);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &graphics_ref);
    if (result != J2ME_SUCCESS) return result;
    if (vm && vm->display && vm->display->context) {
        j2me_graphics_draw_rect(vm->display->context, x, y, width, height, false);
    }
    return J2ME_SUCCESS;
}

j2me_error_t midp_graphics_fill_round_rect(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int arc_height, arc_width, height, width, y, x, graphics_ref;
    j2me_error_t result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &arc_height);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &arc_width);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &height);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &width);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &y);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &x);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &graphics_ref);
    if (result != J2ME_SUCCESS) return result;
    if (vm && vm->display && vm->display->context) {
        j2me_graphics_draw_rect(vm->display->context, x, y, width, height, true);
    }
    return J2ME_SUCCESS;
}

j2me_error_t midp_graphics_fill_arc(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int arc_angle, start_angle, height, width, y, x, graphics_ref;
    j2me_error_t result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &arc_angle);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &start_angle);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &height);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &width);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &y);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &x);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &graphics_ref);
    if (result != J2ME_SUCCESS) return result;
    if (vm && vm->display && vm->display->context) {
        j2me_graphics_draw_arc(vm->display->context, x, y, width, height, start_angle, arc_angle, true);
    }
    return J2ME_SUCCESS;
}

j2me_error_t midp_graphics_fill_triangle(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int y3, x3, y2, x2, y1, x1, graphics_ref;
    j2me_error_t result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &y3);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &x3);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &y2);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &x2);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &y1);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &x1);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &graphics_ref);
    if (result != J2ME_SUCCESS) return result;
    if (vm && vm->display && vm->display->context) {
        int x_points[] = {x1, x2, x3};
        int y_points[] = {y1, y2, y3};
        j2me_graphics_draw_polygon(vm->display->context, x_points, y_points, 3, true);
    }
    return J2ME_SUCCESS;
}

j2me_error_t midp_graphics_draw_rgb(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int alpha, height, width, scanlength, offset, rgb_data_ref, graphics_ref;
    j2me_error_t result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &alpha);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &height);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &width);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &scanlength);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &offset);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &rgb_data_ref);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &graphics_ref);
    if (result != J2ME_SUCCESS) return result;
    return J2ME_SUCCESS;
}

j2me_error_t midp_graphics_get_display_color(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int color, graphics_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &color);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &graphics_ref);
    if (result != J2ME_SUCCESS) return result;
    return j2me_operand_stack_push(&frame->operand_stack, color);
}

j2me_error_t midp_font_char_width(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int ch, font_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &ch);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &font_ref);
    if (result != J2ME_SUCCESS) return result;
    j2me_int width = 8;
    if (vm && vm->display && vm->display->context) {
        width = j2me_graphics_get_char_width(vm->display->context, (char)(ch & 0xFFFF));
    }
    return j2me_operand_stack_push(&frame->operand_stack, width);
}

j2me_error_t midp_font_chars_width(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int length, offset, char_array_ref, font_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &length);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &offset);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &char_array_ref);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &font_ref);
    if (result != J2ME_SUCCESS) return result;
    j2me_int width = length * 8;
    if (vm && vm->display && vm->display->context) {
        width = length * j2me_graphics_get_char_width(vm->display->context, 'W');
    }
    return j2me_operand_stack_push(&frame->operand_stack, width);
}

j2me_error_t midp_font_string_width(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int string_ref, font_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &string_ref);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &font_ref);
    if (result != J2ME_SUCCESS) return result;
    j2me_int width = 0;
    if (vm && vm->display && vm->display->context) {
        const char* str = NULL;
        if (vm->heap && string_ref != J2ME_NULL_REF) {
            str = j2me_heap_string_get_chars(vm->heap, (j2me_ref_t)string_ref);
        }
        if (str) {
            width = j2me_graphics_get_string_width(vm->display->context, str);
        }
    }
    return j2me_operand_stack_push(&frame->operand_stack, width);
}

j2me_error_t midp_font_substring_width(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int len, offset, string_ref, font_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &len);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &offset);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &string_ref);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &font_ref);
    if (result != J2ME_SUCCESS) return result;
    j2me_int width = 0;
    if (vm && vm->display && vm->display->context && vm->heap && string_ref != J2ME_NULL_REF) {
        const char* str = j2me_heap_string_get_chars(vm->heap, (j2me_ref_t)string_ref);
        if (str) {
            int slen = (int)strlen(str);
            if (offset >= 0 && offset + len <= slen) {
                char* buf = (char*)malloc(len + 1);
                if (buf) {
                    memcpy(buf, str + offset, len);
                    buf[len] = '\0';
                    width = j2me_graphics_get_string_width(vm->display->context, buf);
                    free(buf);
                }
            }
        }
    }
    return j2me_operand_stack_push(&frame->operand_stack, width);
}

j2me_error_t midp_font_init(j2me_vm_t* vm, j2me_stack_frame_t* frame, void* args) {
    j2me_int size, style, face, font_ref;
    j2me_error_t result = j2me_operand_stack_pop(&frame->operand_stack, &size);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &style);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &face);
    if (result != J2ME_SUCCESS) return result;
    result = j2me_operand_stack_pop(&frame->operand_stack, &font_ref);
    if (result != J2ME_SUCCESS) return result;
    return J2ME_SUCCESS;
}
