#include "j2me_native_methods.h"
#include "j2me_graphics.h"
#include "j2me_string.h"
#include "j2me_object.h"
#include "j2me_heap.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "j2me_log.h"
#include <stdio.h>

static j2me_native_method_registry_t* g_native_registry = NULL;

j2me_native_method_registry_t* j2me_native_method_registry_create(void) {
    j2me_native_method_registry_t* registry = 
        (j2me_native_method_registry_t*)malloc(sizeof(j2me_native_method_registry_t));
    if (!registry) return NULL;
    registry->entries = NULL;
    registry->count = 0;
    registry->capacity = 0;
    return registry;
}

void j2me_native_method_registry_destroy(j2me_native_method_registry_t* registry) {
    if (registry) {
        if (registry->entries) free(registry->entries);
        free(registry);
    }
}

j2me_error_t j2me_native_method_register(j2me_native_method_registry_t* registry,
                                         const char* class_name,
                                         const char* method_name,
                                         const char* signature,
                                         j2me_native_method_func_t func) {
    if (!registry || !class_name || !method_name || !signature || !func)
        return J2ME_ERROR_INVALID_PARAMETER;
    if (registry->count >= registry->capacity) {
        size_t new_capacity = registry->capacity == 0 ? 16 : registry->capacity * 2;
        j2me_native_method_entry_t* new_entries = 
            (j2me_native_method_entry_t*)realloc(registry->entries, 
                                                 new_capacity * sizeof(j2me_native_method_entry_t));
        if (!new_entries) return J2ME_ERROR_OUT_OF_MEMORY;
        registry->entries = new_entries;
        registry->capacity = new_capacity;
    }
    j2me_native_method_entry_t* entry = &registry->entries[registry->count];
    entry->class_name = class_name;
    entry->method_name = method_name;
    entry->signature = signature;
    entry->func = func;
    registry->count++;
    return J2ME_SUCCESS;
}

j2me_native_method_func_t j2me_native_method_find(j2me_native_method_registry_t* registry,
                                                  const char* class_name,
                                                  const char* method_name,
                                                  const char* signature) {
    if (!registry || !class_name || !method_name || !signature) return NULL;
    for (size_t i = 0; i < registry->count; i++) {
        j2me_native_method_entry_t* entry = &registry->entries[i];
        if (strcmp(entry->class_name, class_name) == 0 &&
            strcmp(entry->method_name, method_name) == 0 &&
            strcmp(entry->signature, signature) == 0) {
            return entry->func;
        }
    }
    return NULL;
}

j2me_error_t j2me_native_method_invoke(j2me_vm_t* vm,
                                       j2me_stack_frame_t* frame,
                                       const char* class_name,
                                       const char* method_name,
                                       const char* signature,
                                       void* args) {
    if (!g_native_registry) return J2ME_ERROR_INVALID_STATE;
    j2me_native_method_func_t func = j2me_native_method_find(g_native_registry, class_name, method_name, signature);
    if (!func) {
        LOG_DEBUG("[本地方法] 未找到: %s.%s%s\n", class_name, method_name, signature);
        return J2ME_ERROR_METHOD_NOT_FOUND;
    }
    return func(vm, frame, args);
}

j2me_error_t j2me_midp_native_methods_init(j2me_vm_t* vm) {
    if (!vm) return J2ME_ERROR_INVALID_PARAMETER;
    if (!g_native_registry) {
        g_native_registry = j2me_native_method_registry_create();
        if (!g_native_registry) return J2ME_ERROR_OUT_OF_MEMORY;
    }

    j2me_native_method_register(g_native_registry, "javax/microedition/midlet/MIDlet", "platformRequest", "(Ljava/lang/String;)Z", midp_midlet_platform_request);
    j2me_native_method_register(g_native_registry, "javax/microedition/midlet/MIDlet", "destroyApp", "(Z)V", midp_midlet_destroy_app);
    j2me_native_method_register(g_native_registry, "javax/microedition/midlet/MIDlet", "notifyDestroyed", "()V", midp_midlet_notify_destroyed);
    j2me_native_method_register(g_native_registry, "XMIDlet", "destroyApp", "(Z)V", midp_midlet_destroy_app);
    j2me_native_method_register(g_native_registry, "XMIDlet", "platformRequest", "(Ljava/lang/String;)Z", midp_midlet_platform_request);

    j2me_native_method_register(g_native_registry, "java/lang/String", "length", "()I", java_string_length);
    j2me_native_method_register(g_native_registry, "java/lang/String", "charAt", "(I)C", java_string_char_at);
    j2me_native_method_register(g_native_registry, "java/lang/String", "substring", "(II)Ljava/lang/String;", java_string_substring);
    j2me_native_method_register(g_native_registry, "java/lang/String", "equals", "(Ljava/lang/Object;)Z", java_string_equals);
    j2me_native_method_register(g_native_registry, "java/lang/String", "hashCode", "()I", java_string_hash_code);
    j2me_native_method_register(g_native_registry, "java/lang/String", "indexOf", "(I)I", java_string_index_of);
    j2me_native_method_register(g_native_registry, "java/lang/String", "indexOf", "(II)I", java_string_index_of_from);
    j2me_native_method_register(g_native_registry, "java/lang/String", "lastIndexOf", "(I)I", java_string_last_index_of);
    j2me_native_method_register(g_native_registry, "java/lang/String", "lastIndexOf", "(II)I", java_string_last_index_of_from);
    j2me_native_method_register(g_native_registry, "java/lang/String", "intern", "()Ljava/lang/String;", java_string_intern);

    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Display", "getDisplay", "()Ljavax/microedition/lcdui/Display;", midp_display_get_display);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Display", "setCurrent", "(Ljavax/microedition/lcdui/Displayable;)V", midp_display_set_current);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Display", "getCurrent", "()Ljavax/microedition/lcdui/Displayable;", midp_display_get_current);

    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Canvas", "repaint", "()V", midp_canvas_repaint);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Canvas", "serviceRepaints", "()V", midp_canvas_service_repaints);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Canvas", "getWidth", "()I", midp_canvas_get_width);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Canvas", "getHeight", "()I", midp_canvas_get_height);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Canvas", "keyPressed", "(I)V", midp_canvas_key_pressed);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Canvas", "keyReleased", "(I)V", midp_canvas_key_released);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Canvas", "pointerPressed", "(II)V", midp_canvas_pointer_pressed);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Canvas", "pointerReleased", "(II)V", midp_canvas_pointer_released);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Canvas", "pointerDragged", "(II)V", midp_canvas_pointer_dragged);

    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Graphics", "setColor", "(III)V", midp_graphics_set_color_rgb);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Graphics", "setColor", "(I)V", midp_graphics_set_color);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Graphics", "getColor", "()I", midp_graphics_get_color);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Graphics", "drawLine", "(IIII)V", midp_graphics_draw_line);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Graphics", "drawRect", "(IIII)V", midp_graphics_draw_rect);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Graphics", "fillRect", "(IIII)V", midp_graphics_fill_rect);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Graphics", "drawString", "(Ljava/lang/String;III)V", midp_graphics_draw_string);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Graphics", "drawOval", "(IIII)V", midp_graphics_draw_oval);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Graphics", "fillOval", "(IIII)V", midp_graphics_fill_oval);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Graphics", "drawArc", "(IIIIII)V", midp_graphics_draw_arc);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Graphics", "drawImage", "(Ljavax/microedition/lcdui/Image;III)V", midp_graphics_draw_image);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Graphics", "drawRoundRect", "(IIIIII)V", midp_graphics_draw_round_rect);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Graphics", "fillRoundRect", "(IIIIII)V", midp_graphics_fill_round_rect);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Graphics", "fillArc", "(IIIIII)V", midp_graphics_fill_arc);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Graphics", "fillTriangle", "(IIIIII)V", midp_graphics_fill_triangle);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Graphics", "drawRGB", "([IIIIII)V", midp_graphics_draw_rgb);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Graphics", "getDisplayColor", "(I)I", midp_graphics_get_display_color);

    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Font", "charWidth", "(C)I", midp_font_char_width);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Font", "charsWidth", "([CII)I", midp_font_chars_width);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Font", "stringWidth", "(Ljava/lang/String;)I", midp_font_string_width);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Font", "substringWidth", "(Ljava/lang/String;II)I", midp_font_substring_width);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Font", "init", "(III)V", midp_font_init);

    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Image", "createImage", "(II)Ljavax/microedition/lcdui/Image;", midp_image_create_image);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Image", "createImage", "(Ljava/lang/String;)Ljavax/microedition/lcdui/Image;", midp_image_create_image_from_file);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Image", "getWidth", "()I", midp_image_get_width);
    j2me_native_method_register(g_native_registry, "javax/microedition/lcdui/Image", "getHeight", "()I", midp_image_get_height);

    j2me_native_method_register(g_native_registry, "java/io/PrintStream", "println", "(Ljava/lang/String;)V", java_system_out_println);
    j2me_native_method_register(g_native_registry, "java/io/PrintStream", "print", "(Ljava/lang/String;)V", java_system_out_print);

    j2me_native_method_register(g_native_registry, "java/lang/Thread", "start", "()V", java_thread_start);
    j2me_native_method_register(g_native_registry, "java/lang/Thread", "sleep", "(J)V", java_thread_sleep);
    j2me_native_method_register(g_native_registry, "java/lang/Thread", "yield", "()V", java_thread_yield);
    j2me_native_method_register(g_native_registry, "java/lang/Thread", "currentThread", "()Ljava/lang/Thread;", java_thread_current_thread);

    j2me_native_method_register(g_native_registry, "java/lang/Object", "getClass", "()Ljava/lang/Class;", java_object_get_class);
    j2me_native_method_register(g_native_registry, "java/lang/Object", "hashCode", "()I", java_object_hash_code);
    j2me_native_method_register(g_native_registry, "java/lang/Object", "notify", "()V", java_object_notify);
    j2me_native_method_register(g_native_registry, "java/lang/Object", "notifyAll", "()V", java_object_notify_all);
    j2me_native_method_register(g_native_registry, "java/lang/Object", "wait", "(J)V", java_object_wait);

    j2me_native_method_register(g_native_registry, "java/lang/System", "currentTimeMillis", "()J", java_system_current_time_millis);
    j2me_native_method_register(g_native_registry, "java/lang/System", "arraycopy", "(Ljava/lang/Object;ILjava/lang/Object;II)V", java_system_arraycopy);
    j2me_native_method_register(g_native_registry, "java/lang/System", "identityHashCode", "(Ljava/lang/Object;)I", java_system_identity_hash_code);
    j2me_native_method_register(g_native_registry, "java/lang/System", "getProperty0", "(Ljava/lang/String;)Ljava/lang/String;", java_system_get_property0);

    j2me_native_method_register(g_native_registry, "java/lang/Math", "sin", "(D)D", java_math_sin);
    j2me_native_method_register(g_native_registry, "java/lang/Math", "cos", "(D)D", java_math_cos);
    j2me_native_method_register(g_native_registry, "java/lang/Math", "tan", "(D)D", java_math_tan);
    j2me_native_method_register(g_native_registry, "java/lang/Math", "sqrt", "(D)D", java_math_sqrt);
    j2me_native_method_register(g_native_registry, "java/lang/Math", "ceil", "(D)D", java_math_ceil);
    j2me_native_method_register(g_native_registry, "java/lang/Math", "floor", "(D)D", java_math_floor);

    j2me_native_method_register(g_native_registry, "java/lang/Runtime", "gc", "()V", java_runtime_gc);
    j2me_native_method_register(g_native_registry, "java/lang/Runtime", "freeMemory", "()J", java_runtime_free_memory);
    j2me_native_method_register(g_native_registry, "java/lang/Runtime", "totalMemory", "()J", java_runtime_total_memory);
    j2me_native_method_register(g_native_registry, "java/lang/Runtime", "exitInternal", "(I)V", java_runtime_exit_internal);

    j2me_native_method_register(g_native_registry, "java/lang/Throwable", "printStackTrace", "()V", java_throwable_print_stack_trace);
    j2me_native_method_register(g_native_registry, "java/lang/Throwable", "fillInStackTrace", "()V", java_throwable_fill_in_stack_trace);

    j2me_native_method_register(g_native_registry, "java/lang/Float", "floatToIntBits", "(F)I", java_float_to_int_bits);
    j2me_native_method_register(g_native_registry, "java/lang/Float", "floatToRawIntBits", "(F)I", java_float_to_raw_int_bits);
    j2me_native_method_register(g_native_registry, "java/lang/Float", "intBitsToFloat", "(I)F", java_float_int_bits_to_float);
    j2me_native_method_register(g_native_registry, "java/lang/Double", "doubleToLongBits", "(D)J", java_double_to_long_bits);
    j2me_native_method_register(g_native_registry, "java/lang/Double", "doubleToRawLongBits", "(D)J", java_double_to_raw_long_bits);
    j2me_native_method_register(g_native_registry, "java/lang/Double", "longBitsToDouble", "(J)D", java_double_long_bits_to_double);

    j2me_native_method_register(g_native_registry, "java/lang/Class", "forName", "(Ljava/lang/String;)Ljava/lang/Class;", java_class_for_name);
    j2me_native_method_register(g_native_registry, "java/lang/Class", "newInstance", "()Ljava/lang/Object;", java_class_new_instance);
    j2me_native_method_register(g_native_registry, "java/lang/Class", "isInstance", "(Ljava/lang/Object;)Z", java_class_is_instance);
    j2me_native_method_register(g_native_registry, "java/lang/Class", "isAssignableFrom", "(Ljava/lang/Class;)Z", java_class_is_assignable_from);
    j2me_native_method_register(g_native_registry, "java/lang/Class", "isInterface", "()Z", java_class_is_interface);
    j2me_native_method_register(g_native_registry, "java/lang/Class", "isArray", "()Z", java_class_is_array);
    j2me_native_method_register(g_native_registry, "java/lang/Class", "getName", "()Ljava/lang/String;", java_class_get_name);
    j2me_native_method_register(g_native_registry, "java/lang/Class", "getSuperclass", "()Ljava/lang/Class;", java_class_get_superclass);
    j2me_native_method_register(g_native_registry, "java/lang/Class", "invoke_clinit", "()V", java_class_invoke_clinit);
    j2me_native_method_register(g_native_registry, "java/lang/Class", "init9", "()V", java_class_init9);
    j2me_native_method_register(g_native_registry, "java/lang/Class", "invoke_verify", "()V", java_class_invoke_verify);

    vm->native_method_registry = g_native_registry;
    LOG_DEBUG("[本地方法] MIDP本地方法初始化完成，注册了 %zu 个方法\n", g_native_registry->count);
    return J2ME_SUCCESS;
}
