#include "j2me_class.h"
#include "j2me_log.h"
#include "j2me_vm.h"
#include "j2me_jar.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @file j2me_class_loader.c
 * @brief J2ME类加载器实现
 * 
 * 实现Java类的加载、链接和初始化功能
 */

j2me_class_loader_t* j2me_class_loader_create(j2me_vm_t* vm, const char* classpath) {
    if (!vm) {
        return NULL;
    }
    
    j2me_class_loader_t* loader = (j2me_class_loader_t*)malloc(sizeof(j2me_class_loader_t));
    if (!loader) {
        return NULL;
    }
    
    memset(loader, 0, sizeof(j2me_class_loader_t));
    loader->vm = vm;
    
    if (classpath) {
        loader->classpath = (char*)malloc(strlen(classpath) + 1);
        if (loader->classpath) {
            strcpy(loader->classpath, classpath);
        }
    }
    
    LOG_DEBUG("[类加载器] 创建成功，类路径: %s\n", classpath ? classpath : "(默认)");
    return loader;
}

j2me_error_t j2me_class_loader_set_jar_file(j2me_class_loader_t* loader, void* jar_file) {
    if (!loader) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    loader->jar_file = jar_file;
    LOG_DEBUG("[类加载器] 设置JAR文件: %p\n", jar_file);
    return J2ME_SUCCESS;
}

void j2me_class_loader_destroy(j2me_class_loader_t* loader) {
    if (!loader) {
        return;
    }
    
    // 销毁所有已加载的类
    j2me_class_t* current = loader->loaded_classes;
    while (current) {
        j2me_class_t* next = current->next;
        j2me_class_destroy(current);
        current = next;
    }
    
    if (loader->classpath) {
        free(loader->classpath);
    }
    
    free(loader);
    LOG_DEBUG("[类加载器] 已销毁\n");
}

j2me_class_t* j2me_class_loader_find_class(j2me_class_loader_t* loader, const char* class_name) {
    if (!loader || !class_name) {
        return NULL;
    }
    
    j2me_class_t* current = loader->loaded_classes;
    while (current) {
        if (current->name && strcmp(current->name, class_name) == 0) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

/**
 * @brief 从JAR文件加载Class数据
 * @param jar_file JAR文件对象
 * @param class_name 类名
 * @param size 输出数据大小
 * @return Class文件数据，失败返回NULL
 */
static uint8_t* load_class_from_jar(j2me_jar_file_t* jar_file, const char* class_name, size_t* size) {
    if (!jar_file || !class_name || !size) {
        return NULL;
    }
    
    // 构建类文件路径 (将包名中的.替换为/，但保留.class扩展名)
    char class_path[256];
    int len = strlen(class_name);
    
    // 复制类名并转换包分隔符
    for (int i = 0; i < len && i < sizeof(class_path) - 7; i++) {
        if (class_name[i] == '.') {
            class_path[i] = '/';
        } else {
            class_path[i] = class_name[i];
        }
    }
    
    // 添加.class扩展名
    strcpy(class_path + len, ".class");
    
    LOG_DEBUG("[类加载器] 在JAR中查找类文件: %s\n", class_path);
    // 在JAR文件中查找类文件
    j2me_jar_entry_t* entry = j2me_jar_find_entry(jar_file, class_path);
    if (!entry) {
        LOG_DEBUG("[类加载器] 未在JAR中找到类文件: %s\n", class_path);
        return NULL;
    }
    
    // 加载类文件数据
    j2me_error_t result = j2me_jar_load_entry(jar_file, entry);
    if (result != J2ME_SUCCESS) {
        LOG_ERROR("[类加载器] 加载JAR条目失败: %s", class_path);
        return NULL;
    }
    
    // 复制数据
    *size = entry->uncompressed_size;
    uint8_t* data = (uint8_t*)malloc(*size);
    if (data && entry->data) {
        memcpy(data, entry->data, *size);
        LOG_DEBUG("[类加载器] 从JAR加载类文件成功: %s (%zu bytes)\n", class_path, *size);
    }
    
    return data;
}

/**
 * @brief 从文件加载Class数据 (简化实现)
 * @param class_name 类名
 * @param size 输出数据大小
 * @return Class文件数据，失败返回NULL
 */
static uint8_t* load_class_file(const char* class_name, size_t* size) {
    // 简化实现：创建一个最小的Class文件结构
    // 实际实现应该从文件系统或JAR文件中读取
    
    // 为演示目的，创建一个简单的"Hello"类
    if (strcmp(class_name, "Hello") == 0) {
        // 最小的Class文件结构 (简化版本)
        static uint8_t hello_class[] = {
            // Magic number
            0xCA, 0xFE, 0xBA, 0xBE,
            // Minor version
            0x00, 0x00,
            // Major version (Java 8)
            0x00, 0x34,
            // Constant pool count
            0x00, 0x05,
            // Constant pool entries (简化)
            // #1: Class info
            0x07, 0x00, 0x02,
            // #2: UTF-8 "Hello"
            0x01, 0x00, 0x05, 'H', 'e', 'l', 'l', 'o',
            // #3: Class info (Object)
            0x07, 0x00, 0x04,
            // #4: UTF-8 "java/lang/Object"
            0x01, 0x00, 0x10, 'j', 'a', 'v', 'a', '/', 'l', 'a', 'n', 'g', '/', 'O', 'b', 'j', 'e', 'c', 't',
            // Access flags (public)
            0x00, 0x21,
            // This class
            0x00, 0x01,
            // Super class
            0x00, 0x03,
            // Interfaces count
            0x00, 0x00,
            // Fields count
            0x00, 0x00,
            // Methods count
            0x00, 0x00,
            // Attributes count
            0x00, 0x00
        };
        
        *size = sizeof(hello_class);
        uint8_t* data = (uint8_t*)malloc(*size);
        if (data) {
            memcpy(data, hello_class, *size);
        }
        return data;
    }
    
    *size = 0;
    return NULL;
}

j2me_class_t* j2me_class_loader_load_class(j2me_class_loader_t* loader, const char* class_name) {
    if (!loader || !class_name) {
        return NULL;
    }
    
    // 首先检查是否已经加载
    j2me_class_t* existing = j2me_class_loader_find_class(loader, class_name);
    if (existing) {
        return existing;
    }
    
    // printf("[类加载器] 加载类: %s\n", class_name);
    
    // 加载Class文件数据
    size_t data_size;
    uint8_t* class_data = NULL;
    
    // 首先尝试从JAR文件加载 (如果有的话)
    if (loader->jar_file) {
        class_data = load_class_from_jar(loader->jar_file, class_name, &data_size);
    }
    
    // 如果JAR文件中没有找到，尝试从文件系统加载
    if (!class_data) {
        class_data = load_class_file(class_name, &data_size);
    }
    
    if (!class_data) {
        LOG_ERROR("[类加载器] 错误: 无法找到类文件 %s", class_name);
        return NULL;
    }
    
    // 解析Class文件
    j2me_class_t* class_ptr = j2me_class_parse(class_data, data_size);
    free(class_data);
    
    if (!class_ptr) {
        LOG_ERROR("[类加载器] 错误: 解析类文件失败 %s", class_name);
        return NULL;
    }
    
    // 设置类加载器
    class_ptr->loader = loader;
    class_ptr->state = CLASS_LOADED;
    
    // 添加到已加载类列表
    class_ptr->next = loader->loaded_classes;
    loader->loaded_classes = class_ptr;
    
    LOG_DEBUG("[类加载器] 类加载成功: %s (方法数: %d, 字段数: %d)\n", class_name, class_ptr->methods_count, class_ptr->fields_count);
    
    // 打印方法信息以便调试
    for (uint16_t i = 0; i < class_ptr->methods_count && i < 5; i++) {
        j2me_method_t* method = &class_ptr->methods[i];
        LOG_DEBUG("[类加载器]   方法 %d: %s%s (字节码长度: %d)\n", i, method->name ? method->name : "unknown",
               method->descriptor ? method->descriptor : "",
               method->bytecode_length);
    }
    
    // 解析父类指针
    if (class_ptr->super_name && loader) {
        j2me_class_t* super_class = j2me_class_loader_find_class(loader, class_ptr->super_name);
        if (!super_class) {
            // 父类尚未加载，递归加载
            super_class = j2me_class_loader_load_class(loader, class_ptr->super_name);
        }
        if (super_class) {
            class_ptr->super_class_ptr = super_class;
            LOG_DEBUG("[类加载器] 父类链接: %s -> %s\n", class_name, class_ptr->super_name);
        } else {
            LOG_DEBUG("[类加载器] 警告: 无法加载父类 %s (当前类: %s)\n", class_ptr->super_name, class_name);
        }
    }

    // 自动链接类
    j2me_error_t link_result = j2me_class_link(class_ptr);
    if (link_result != J2ME_SUCCESS) {
        LOG_WARN("[类加载器] 警告: 类链接失败: %s (错误: %d)", class_name, link_result);
    }
    
    // 自动初始化类（执行<clinit>）
    j2me_error_t init_result = j2me_class_initialize(class_ptr);
    if (init_result != J2ME_SUCCESS) {
        LOG_WARN("[类加载器] 警告: 类初始化失败: %s (错误: %d)", class_name, init_result);
    }
    
    return class_ptr;
}

j2me_error_t j2me_class_link(j2me_class_t* class_ptr) {
    if (!class_ptr) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 如果已经链接或初始化，直接返回成功
    if (class_ptr->state == CLASS_LINKED || class_ptr->state == CLASS_INITIALIZED) {
        return J2ME_SUCCESS;
    }
    
    // 现在应该是CLASS_LOADED状态
    if (class_ptr->state != CLASS_LOADED) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    LOG_DEBUG("[类加载器] 链接类: %s\n", class_ptr->name);
    // 验证阶段 (简化)
    // 实际实现应该进行字节码验证
    
    // 准备阶段：为静态字段分配内存并设置默认值
    for (uint16_t i = 0; i < class_ptr->fields_count; i++) {
        j2me_field_t* field = &class_ptr->fields[i];
        if (field->access_flags & ACC_STATIC) {
            // 为静态字段分配内存 (简化实现)
            LOG_DEBUG("[类加载器] 准备静态字段: %s\n", field->name ? field->name : "unknown");
        }
    }
    
    // 解析阶段：解析符号引用
    // 实际实现应该解析常量池中的符号引用
    
    class_ptr->state = CLASS_LINKED;
    LOG_DEBUG("[类加载器] 类链接完成: %s\n", class_ptr->name);
    return J2ME_SUCCESS;
}

j2me_error_t j2me_class_initialize(j2me_class_t* class_ptr) {
    if (!class_ptr) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 如果已经初始化，直接返回成功
    if (class_ptr->state == CLASS_INITIALIZED) {
        return J2ME_SUCCESS;
    }
    
    // 如果还没有链接，先链接
    if (class_ptr->state == CLASS_LOADED) {
        j2me_error_t link_result = j2me_class_link(class_ptr);
        if (link_result != J2ME_SUCCESS) {
            return link_result;
        }
    }
    
    // 现在应该是CLASS_LINKED状态
    if (class_ptr->state != CLASS_LINKED) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    LOG_DEBUG("[类加载器] 初始化类: %s\n", class_ptr->name);
    // 标记类为已初始化（在执行<clinit>之前，防止递归初始化）
    class_ptr->state = CLASS_INITIALIZED;
    
    // 初始化父类 (如果有)
    if (class_ptr->super_class_ptr && class_ptr->super_class_ptr->state != CLASS_INITIALIZED) {
        j2me_error_t result = j2me_class_initialize(class_ptr->super_class_ptr);
        if (result != J2ME_SUCCESS) {
            return result;
        }
    }
    
    // 执行类初始化方法 <clinit>
    if (class_ptr->clinit) {
        LOG_DEBUG("[类加载器] 执行类初始化方法: %s.<clinit> (字节码长度: %d)\n", class_ptr->name, class_ptr->clinit->bytecode_length);
        
        // 执行<clinit>方法（静态初始化）
        if (class_ptr->loader && class_ptr->loader->vm) {
            j2me_error_t result = j2me_interpreter_execute_method(
                class_ptr->loader->vm, 
                class_ptr->clinit, 
                NULL,  // 静态方法没有this
                NULL   // 没有参数
            );
            
            if (result != J2ME_SUCCESS) {
                LOG_WARN("[类加载器] 警告: 类初始化方法执行失败: %s.<clinit> (错误: %d)", class_ptr->name, result);
                // 继续执行，不中断程序
            } else {
                LOG_DEBUG("[类加载器] 类初始化方法执行成功: %s.<clinit>\n", class_ptr->name);
            }
        }
    }
    
    LOG_DEBUG("[类加载器] 类初始化完成: %s\n", class_ptr->name);
    return J2ME_SUCCESS;
}

bool j2me_class_is_subclass_of(j2me_class_t* class_ptr, const char* parent_class_name) {
    if (!class_ptr || !parent_class_name) {
        return false;
    }
    
    // 检查当前类是否就是目标类
    if (class_ptr->name && strcmp(class_ptr->name, parent_class_name) == 0) {
        return true;
    }
    
    // 递归检查父类
    if (class_ptr->super_name) {
        // 直接比较父类名
        if (strcmp(class_ptr->super_name, parent_class_name) == 0) {
            return true;
        }
        
        // 如果有父类指针，递归检查
        if (class_ptr->super_class_ptr) {
            return j2me_class_is_subclass_of(class_ptr->super_class_ptr, parent_class_name);
        }
        
        // 如果没有父类指针但有父类名，尝试加载父类
        if (class_ptr->loader) {
            j2me_class_t* super_class = j2me_class_loader_find_class(class_ptr->loader, class_ptr->super_name);
            if (super_class) {
                class_ptr->super_class_ptr = super_class;
                return j2me_class_is_subclass_of(super_class, parent_class_name);
            }
        }
    }
    
    return false;
}

j2me_method_t* j2me_class_find_method(j2me_class_t* class_ptr, const char* name, const char* descriptor) {
    if (!class_ptr || !name) {
        return NULL;
    }
    
    // 在当前类中查找
    for (uint16_t i = 0; i < class_ptr->methods_count; i++) {
        j2me_method_t* method = &class_ptr->methods[i];
        if (method->name && strcmp(method->name, name) == 0) {
            if (!descriptor || (method->descriptor && strcmp(method->descriptor, descriptor) == 0)) {
                return method;
            }
        }
    }
    
    // 在父类中查找 (如果不是静态方法)
    if (class_ptr->super_class_ptr) {
        return j2me_class_find_method(class_ptr->super_class_ptr, name, descriptor);
    }
    
    return NULL;
}

j2me_field_t* j2me_class_find_field(j2me_class_t* class_ptr, const char* name, const char* descriptor) {
    if (!class_ptr || !name) {
        return NULL;
    }
    
    // 在当前类中查找
    for (uint16_t i = 0; i < class_ptr->fields_count; i++) {
        j2me_field_t* field = &class_ptr->fields[i];
        if (field->name && strcmp(field->name, name) == 0) {
            if (!descriptor || (field->descriptor && strcmp(field->descriptor, descriptor) == 0)) {
                return field;
            }
        }
    }
    
    // 在父类中查找
    if (class_ptr->super_class_ptr) {
        return j2me_class_find_field(class_ptr->super_class_ptr, name, descriptor);
    }
    
    return NULL;
}

const char* j2me_constant_pool_get_utf8(j2me_constant_pool_t* pool, uint16_t index) {
    if (!pool || index == 0 || index >= pool->count) {
        return NULL;
    }
    
    j2me_constant_pool_entry_t* entry = &pool->entries[index - 1];
    if (entry->tag == J2ME_CONSTANT_UTF8) {
        return entry->info.utf8.bytes;
    }
    
    return NULL;
}

const char* j2me_constant_pool_get_class_name(j2me_constant_pool_t* pool, uint16_t index) {
    if (!pool || index == 0 || index >= pool->count) {
        return NULL;
    }
    
    j2me_constant_pool_entry_t* entry = &pool->entries[index - 1];
    if (entry->tag == J2ME_CONSTANT_CLASS) {
        return j2me_constant_pool_get_utf8(pool, entry->info.class_info.name_index);
    }
    
    return NULL;
}

j2me_error_t j2me_class_loader_load_all_classes(j2me_class_loader_t* loader) {
    if (!loader || !loader->jar_file) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    LOG_DEBUG("[类加载器] 开始加载JAR文件中的所有类...\n");
    j2me_jar_file_t* jar_file = (j2me_jar_file_t*)loader->jar_file;
    int loaded_count = 0;
    
    // 第一阶段：加载和链接所有类
    for (uint16_t i = 0; i < jar_file->entry_count; i++) {
        j2me_jar_entry_t* entry = jar_file->entries[i];
        if (!entry || !entry->name) {
            continue;
        }
        
        // 检查是否是.class文件
        if (strstr(entry->name, ".class")) {
            // 提取类名（去掉.class扩展名，并将/替换为.）
            char class_name[256];
            int len = strlen(entry->name);
            if (len > 6 && strcmp(entry->name + len - 6, ".class") == 0) {
                // 复制类名并转换包分隔符
                for (int i = 0; i < len - 6 && i < sizeof(class_name) - 1; i++) {
                    if (entry->name[i] == '/') {
                        class_name[i] = '.';
                    } else {
                        class_name[i] = entry->name[i];
                    }
                }
                class_name[len - 6] = '\0';
                
                // 加载类（这会自动链接类）
                j2me_class_t* class_ptr = j2me_class_loader_load_class(loader, class_name);
                if (class_ptr) {
                    loaded_count++;
                }
            }
        }
    }
    
    LOG_DEBUG("[类加载器] 第一阶段完成：加载了 %d 个类\n", loaded_count);
    // 第二阶段：初始化所有已加载的类（执行<clinit>）
    LOG_DEBUG("[类加载器] 第二阶段：开始初始化所有类...\n");
    int initialized_count = 0;
    
    j2me_class_t* current = loader->loaded_classes;
    while (current) {
        // 只初始化已链接但未初始化的类
        if (current->state == CLASS_LINKED) {
            j2me_error_t result = j2me_class_initialize(current);
            if (result == J2ME_SUCCESS) {
                initialized_count++;
            }
        }
        current = current->next;
    }
    
    LOG_DEBUG("[类加载器] 第二阶段完成：初始化了 %d 个类\n", initialized_count);
    LOG_DEBUG("[类加载器] 所有类加载和初始化完成\n");
    return J2ME_SUCCESS;
}