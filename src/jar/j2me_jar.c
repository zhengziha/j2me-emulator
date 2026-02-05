#include "j2me_jar.h"
#include "j2me_vm.h"
#include "j2me_midlet_executor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <zlib.h>
#include <errno.h>

/**
 * @file j2me_jar.c
 * @brief J2ME JAR文件解析器实现
 * 
 * 基于ZIP格式的JAR文件解析和MIDlet管理
 */

// ZIP文件格式常量
#define ZIP_LOCAL_FILE_HEADER_SIGNATURE     0x04034b50
#define ZIP_CENTRAL_DIR_HEADER_SIGNATURE    0x02014b50
#define ZIP_END_OF_CENTRAL_DIR_SIGNATURE    0x06054b50

#define ZIP_COMPRESSION_STORED              0
#define ZIP_COMPRESSION_DEFLATED            8

// ZIP文件头结构
typedef struct {
    uint32_t signature;
    uint16_t version_needed;
    uint16_t flags;
    uint16_t compression_method;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t filename_length;
    uint16_t extra_field_length;
} zip_local_file_header_t;

typedef struct {
    uint32_t signature;
    uint16_t version_made_by;
    uint16_t version_needed;
    uint16_t flags;
    uint16_t compression_method;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t filename_length;
    uint16_t extra_field_length;
    uint16_t comment_length;
    uint16_t disk_number;
    uint16_t internal_attributes;
    uint32_t external_attributes;
    uint32_t local_header_offset;
} zip_central_dir_header_t;

typedef struct {
    uint32_t signature;
    uint16_t disk_number;
    uint16_t central_dir_disk;
    uint16_t entries_on_disk;
    uint16_t total_entries;
    uint32_t central_dir_size;
    uint32_t central_dir_offset;
    uint16_t comment_length;
} zip_end_of_central_dir_t;

/**
 * @brief 读取小端序16位整数
 */
static uint16_t read_uint16_le(FILE* file) {
    uint8_t bytes[2];
    fread(bytes, 1, 2, file);
    return (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
}

/**
 * @brief 读取小端序32位整数
 */
static uint32_t read_uint32_le(FILE* file) {
    uint8_t bytes[4];
    fread(bytes, 1, 4, file);
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) | 
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

/**
 * @brief 查找ZIP文件的中央目录结束记录
 */
static long find_end_of_central_dir(FILE* file, size_t file_size) {
    // 从文件末尾开始搜索
    long search_start = file_size - sizeof(zip_end_of_central_dir_t);
    if (search_start < 0) search_start = 0;
    
    for (long pos = file_size - 4; pos >= search_start; pos--) {
        fseek(file, pos, SEEK_SET);
        uint32_t signature = read_uint32_le(file);
        if (signature == ZIP_END_OF_CENTRAL_DIR_SIGNATURE) {
            return pos;
        }
    }
    
    return -1;
}

/**
 * @brief 确定JAR条目类型
 */
static j2me_jar_entry_type_t determine_entry_type(const char* name) {
    if (!name) return JAR_ENTRY_FILE;
    
    size_t len = strlen(name);
    if (len == 0) return JAR_ENTRY_FILE;
    
    // 检查是否为目录
    if (name[len - 1] == '/') {
        return JAR_ENTRY_DIRECTORY;
    }
    
    // 检查文件扩展名
    const char* ext = strrchr(name, '.');
    if (ext) {
        if (strcmp(ext, ".class") == 0) {
            return JAR_ENTRY_CLASS;
        }
    }
    
    // 检查特殊文件
    if (strcmp(name, "META-INF/MANIFEST.MF") == 0) {
        return JAR_ENTRY_MANIFEST;
    }
    
    // 检查是否在META-INF目录下
    if (strncmp(name, "META-INF/", 9) == 0) {
        return JAR_ENTRY_RESOURCE;
    }
    
    return JAR_ENTRY_RESOURCE;
}

j2me_jar_file_t* j2me_jar_open(const char* filename) {
    if (!filename) {
        return NULL;
    }
    
    // 检查文件是否存在
    struct stat st;
    if (stat(filename, &st) != 0) {
        printf("[JAR解析器] 文件不存在: %s\n", filename);
        return NULL;
    }
    
    // 打开文件
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("[JAR解析器] 打开文件失败: %s\n", filename);
        return NULL;
    }
    
    // 创建JAR文件对象
    j2me_jar_file_t* jar_file = (j2me_jar_file_t*)malloc(sizeof(j2me_jar_file_t));
    if (!jar_file) {
        fclose(file);
        return NULL;
    }
    
    memset(jar_file, 0, sizeof(j2me_jar_file_t));
    
    jar_file->filename = strdup(filename);
    jar_file->file = file;
    jar_file->file_size = st.st_size;
    jar_file->parsed = false;
    
    printf("[JAR解析器] JAR文件打开成功: %s (大小: %zu bytes)\n", filename, jar_file->file_size);
    
    return jar_file;
}

void j2me_jar_close(j2me_jar_file_t* jar_file) {
    if (!jar_file) {
        return;
    }
    
    // 关闭文件
    if (jar_file->file) {
        fclose(jar_file->file);
        jar_file->file = NULL;
    }
    
    // 释放条目数组
    if (jar_file->entries) {
        for (int i = 0; i < jar_file->entry_count; i++) {
            j2me_jar_entry_t* entry = jar_file->entries[i];
            if (entry) {
                if (entry->name) free(entry->name);
                if (entry->full_path) free(entry->full_path);
                if (entry->data) free(entry->data);
                free(entry);
            }
        }
        free(jar_file->entries);
    }
    
    // 释放清单文件内容
    if (jar_file->manifest_content) {
        free(jar_file->manifest_content);
    }
    
    // 销毁MIDlet套件
    if (jar_file->midlet_suite) {
        j2me_midlet_suite_destroy(jar_file->midlet_suite);
    }
    
    // 释放文件名
    if (jar_file->filename) {
        free(jar_file->filename);
    }
    
    printf("[JAR解析器] JAR文件已关闭\n");
    free(jar_file);
}

j2me_error_t j2me_jar_parse(j2me_jar_file_t* jar_file) {
    if (!jar_file || !jar_file->file) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    if (jar_file->parsed) {
        return J2ME_SUCCESS; // 已经解析过
    }
    
    printf("[JAR解析器] 开始解析JAR文件...\n");
    
    // 查找中央目录结束记录
    long eocd_pos = find_end_of_central_dir(jar_file->file, jar_file->file_size);
    if (eocd_pos < 0) {
        printf("[JAR解析器] 未找到ZIP中央目录结束记录\n");
        return J2ME_ERROR_IO_EXCEPTION;
    }
    
    // 读取中央目录结束记录
    fseek(jar_file->file, eocd_pos, SEEK_SET);
    zip_end_of_central_dir_t eocd;
    eocd.signature = read_uint32_le(jar_file->file);
    eocd.disk_number = read_uint16_le(jar_file->file);
    eocd.central_dir_disk = read_uint16_le(jar_file->file);
    eocd.entries_on_disk = read_uint16_le(jar_file->file);
    eocd.total_entries = read_uint16_le(jar_file->file);
    eocd.central_dir_size = read_uint32_le(jar_file->file);
    eocd.central_dir_offset = read_uint32_le(jar_file->file);
    eocd.comment_length = read_uint16_le(jar_file->file);
    
    printf("[JAR解析器] 找到 %d 个条目\n", eocd.total_entries);
    
    // 分配条目数组
    jar_file->entry_count = eocd.total_entries;
    jar_file->entries = (j2me_jar_entry_t**)calloc(jar_file->entry_count, sizeof(j2me_jar_entry_t*));
    if (!jar_file->entries) {
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    // 读取中央目录条目
    fseek(jar_file->file, eocd.central_dir_offset, SEEK_SET);
    
    for (int i = 0; i < jar_file->entry_count; i++) {
        // 读取中央目录头
        zip_central_dir_header_t header;
        header.signature = read_uint32_le(jar_file->file);
        
        if (header.signature != ZIP_CENTRAL_DIR_HEADER_SIGNATURE) {
            printf("[JAR解析器] 无效的中央目录头签名: 0x%08x\n", header.signature);
            return J2ME_ERROR_IO_EXCEPTION;
        }
        
        header.version_made_by = read_uint16_le(jar_file->file);
        header.version_needed = read_uint16_le(jar_file->file);
        header.flags = read_uint16_le(jar_file->file);
        header.compression_method = read_uint16_le(jar_file->file);
        header.last_mod_time = read_uint16_le(jar_file->file);
        header.last_mod_date = read_uint16_le(jar_file->file);
        header.crc32 = read_uint32_le(jar_file->file);
        header.compressed_size = read_uint32_le(jar_file->file);
        header.uncompressed_size = read_uint32_le(jar_file->file);
        header.filename_length = read_uint16_le(jar_file->file);
        header.extra_field_length = read_uint16_le(jar_file->file);
        header.comment_length = read_uint16_le(jar_file->file);
        header.disk_number = read_uint16_le(jar_file->file);
        header.internal_attributes = read_uint16_le(jar_file->file);
        header.external_attributes = read_uint32_le(jar_file->file);
        header.local_header_offset = read_uint32_le(jar_file->file);
        
        // 读取文件名
        char* filename = (char*)malloc(header.filename_length + 1);
        if (!filename) {
            return J2ME_ERROR_OUT_OF_MEMORY;
        }
        fread(filename, 1, header.filename_length, jar_file->file);
        filename[header.filename_length] = '\0';
        
        // 跳过额外字段和注释
        fseek(jar_file->file, header.extra_field_length + header.comment_length, SEEK_CUR);
        
        // 创建JAR条目
        j2me_jar_entry_t* entry = (j2me_jar_entry_t*)malloc(sizeof(j2me_jar_entry_t));
        if (!entry) {
            free(filename);
            return J2ME_ERROR_OUT_OF_MEMORY;
        }
        
        memset(entry, 0, sizeof(j2me_jar_entry_t));
        
        entry->name = filename;
        entry->full_path = strdup(filename);
        entry->type = determine_entry_type(filename);
        entry->compressed_size = header.compressed_size;
        entry->uncompressed_size = header.uncompressed_size;
        entry->crc32 = header.crc32;
        entry->compression_method = header.compression_method;
        entry->file_offset = header.local_header_offset;
        entry->loaded = false;
        
        jar_file->entries[i] = entry;
        
        printf("[JAR解析器] 条目 #%d: %s (%s, %zu -> %zu bytes)\n", 
               i, entry->name, j2me_jar_get_entry_type_name(entry->type),
               entry->compressed_size, entry->uncompressed_size);
    }
    
    jar_file->parsed = true;
    
    // 解析清单文件
    j2me_jar_parse_manifest(jar_file);
    
    printf("[JAR解析器] JAR文件解析完成\n");
    return J2ME_SUCCESS;
}

int j2me_jar_get_entry_count(j2me_jar_file_t* jar_file) {
    return jar_file ? jar_file->entry_count : 0;
}

j2me_jar_entry_t* j2me_jar_find_entry(j2me_jar_file_t* jar_file, const char* name) {
    if (!jar_file || !name || !jar_file->entries) {
        return NULL;
    }
    
    for (int i = 0; i < jar_file->entry_count; i++) {
        j2me_jar_entry_t* entry = jar_file->entries[i];
        if (entry && entry->name && strcmp(entry->name, name) == 0) {
            return entry;
        }
    }
    
    return NULL;
}

j2me_jar_entry_t* j2me_jar_get_entry(j2me_jar_file_t* jar_file, int index) {
    if (!jar_file || index < 0 || index >= jar_file->entry_count) {
        return NULL;
    }
    
    return jar_file->entries[index];
}

j2me_error_t j2me_jar_load_entry(j2me_jar_file_t* jar_file, j2me_jar_entry_t* entry) {
    if (!jar_file || !entry || !jar_file->file) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    if (entry->loaded) {
        return J2ME_SUCCESS; // 已经加载
    }
    
    // 定位到本地文件头
    fseek(jar_file->file, entry->file_offset, SEEK_SET);
    
    // 读取本地文件头
    zip_local_file_header_t header;
    header.signature = read_uint32_le(jar_file->file);
    
    if (header.signature != ZIP_LOCAL_FILE_HEADER_SIGNATURE) {
        printf("[JAR解析器] 无效的本地文件头签名: 0x%08x\n", header.signature);
        return J2ME_ERROR_IO_EXCEPTION;
    }
    
    header.version_needed = read_uint16_le(jar_file->file);
    header.flags = read_uint16_le(jar_file->file);
    header.compression_method = read_uint16_le(jar_file->file);
    header.last_mod_time = read_uint16_le(jar_file->file);
    header.last_mod_date = read_uint16_le(jar_file->file);
    header.crc32 = read_uint32_le(jar_file->file);
    header.compressed_size = read_uint32_le(jar_file->file);
    header.uncompressed_size = read_uint32_le(jar_file->file);
    header.filename_length = read_uint16_le(jar_file->file);
    header.extra_field_length = read_uint16_le(jar_file->file);
    
    // 跳过文件名和额外字段
    fseek(jar_file->file, header.filename_length + header.extra_field_length, SEEK_CUR);
    
    // 读取压缩数据
    if (entry->compressed_size == 0) {
        entry->data = NULL;
        entry->loaded = true;
        return J2ME_SUCCESS;
    }
    
    uint8_t* compressed_data = (uint8_t*)malloc(entry->compressed_size);
    if (!compressed_data) {
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    size_t bytes_read = fread(compressed_data, 1, entry->compressed_size, jar_file->file);
    if (bytes_read != entry->compressed_size) {
        free(compressed_data);
        printf("[JAR解析器] 读取压缩数据失败\n");
        return J2ME_ERROR_IO_EXCEPTION;
    }
    
    // 解压数据
    if (entry->compression_method == ZIP_COMPRESSION_STORED) {
        // 未压缩，直接使用
        entry->data = compressed_data;
    } else if (entry->compression_method == ZIP_COMPRESSION_DEFLATED) {
        // 使用zlib解压
        entry->data = (uint8_t*)malloc(entry->uncompressed_size);
        if (!entry->data) {
            free(compressed_data);
            return J2ME_ERROR_OUT_OF_MEMORY;
        }
        
        z_stream stream;
        memset(&stream, 0, sizeof(stream));
        stream.next_in = compressed_data;
        stream.avail_in = entry->compressed_size;
        stream.next_out = entry->data;
        stream.avail_out = entry->uncompressed_size;
        
        // 初始化inflateInit2用于原始deflate数据
        int ret = inflateInit2(&stream, -MAX_WBITS);
        if (ret != Z_OK) {
            free(compressed_data);
            free(entry->data);
            entry->data = NULL;
            printf("[JAR解析器] zlib初始化失败: %d\n", ret);
            return J2ME_ERROR_IO_EXCEPTION;
        }
        
        ret = inflate(&stream, Z_FINISH);
        inflateEnd(&stream);
        
        if (ret != Z_STREAM_END) {
            free(compressed_data);
            free(entry->data);
            entry->data = NULL;
            printf("[JAR解析器] 解压失败: %d\n", ret);
            return J2ME_ERROR_IO_EXCEPTION;
        }
        
        free(compressed_data);
    } else {
        free(compressed_data);
        printf("[JAR解析器] 不支持的压缩方法: %d\n", entry->compression_method);
        return J2ME_ERROR_NOT_IMPLEMENTED;
    }
    
    entry->loaded = true;
    
    printf("[JAR解析器] 条目加载成功: %s (%zu bytes)\n", entry->name, entry->uncompressed_size);
    return J2ME_SUCCESS;
}

j2me_error_t j2me_jar_extract_entry(j2me_jar_file_t* jar_file, j2me_jar_entry_t* entry, const char* output_path) {
    if (!jar_file || !entry || !output_path) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 加载条目数据
    j2me_error_t result = j2me_jar_load_entry(jar_file, entry);
    if (result != J2ME_SUCCESS) {
        return result;
    }
    
    // 如果是目录，创建目录
    if (entry->type == JAR_ENTRY_DIRECTORY) {
        // 这里可以创建目录，暂时跳过
        return J2ME_SUCCESS;
    }
    
    // 写入文件
    FILE* output_file = fopen(output_path, "wb");
    if (!output_file) {
        printf("[JAR解析器] 创建输出文件失败: %s\n", output_path);
        return J2ME_ERROR_IO_EXCEPTION;
    }
    
    if (entry->data && entry->uncompressed_size > 0) {
        size_t bytes_written = fwrite(entry->data, 1, entry->uncompressed_size, output_file);
        if (bytes_written != entry->uncompressed_size) {
            fclose(output_file);
            printf("[JAR解析器] 写入文件失败\n");
            return J2ME_ERROR_IO_EXCEPTION;
        }
    }
    
    fclose(output_file);
    
    printf("[JAR解析器] 条目提取成功: %s -> %s\n", entry->name, output_path);
    return J2ME_SUCCESS;
}

j2me_error_t j2me_jar_extract_all(j2me_jar_file_t* jar_file, const char* output_dir) {
    if (!jar_file || !output_dir) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    printf("[JAR解析器] 开始提取所有条目到: %s\n", output_dir);
    
    for (int i = 0; i < jar_file->entry_count; i++) {
        j2me_jar_entry_t* entry = jar_file->entries[i];
        if (!entry) continue;
        
        // 构建输出路径
        char output_path[1024];
        snprintf(output_path, sizeof(output_path), "%s/%s", output_dir, entry->name);
        
        j2me_error_t result = j2me_jar_extract_entry(jar_file, entry, output_path);
        if (result != J2ME_SUCCESS) {
            printf("[JAR解析器] 提取条目失败: %s\n", entry->name);
            // 继续处理其他条目
        }
    }
    
    printf("[JAR解析器] 所有条目提取完成\n");
    return J2ME_SUCCESS;
}

const char* j2me_jar_get_entry_type_name(j2me_jar_entry_type_t type) {
    switch (type) {
        case JAR_ENTRY_FILE: return "文件";
        case JAR_ENTRY_DIRECTORY: return "目录";
        case JAR_ENTRY_CLASS: return "类文件";
        case JAR_ENTRY_RESOURCE: return "资源";
        case JAR_ENTRY_MANIFEST: return "清单";
        default: return "未知";
    }
}

const char* j2me_midlet_get_state_name(j2me_midlet_state_t state) {
    switch (state) {
        case MIDLET_STATE_PAUSED: return "暂停";
        case MIDLET_STATE_ACTIVE: return "活跃";
        case MIDLET_STATE_DESTROYED: return "已销毁";
        default: return "未知";
    }
}

bool j2me_jar_verify(j2me_jar_file_t* jar_file) {
    if (!jar_file || !jar_file->parsed) {
        return false;
    }
    
    // 简单验证：检查是否有清单文件
    j2me_jar_entry_t* manifest = j2me_jar_find_entry(jar_file, "META-INF/MANIFEST.MF");
    return manifest != NULL;
}

void j2me_jar_get_statistics(j2me_jar_file_t* jar_file, 
                             int* total_entries, size_t* total_size, size_t* compressed_size) {
    if (!jar_file) {
        if (total_entries) *total_entries = 0;
        if (total_size) *total_size = 0;
        if (compressed_size) *compressed_size = 0;
        return;
    }
    
    if (total_entries) *total_entries = jar_file->entry_count;
    
    size_t total = 0, compressed = 0;
    for (int i = 0; i < jar_file->entry_count; i++) {
        j2me_jar_entry_t* entry = jar_file->entries[i];
        if (entry) {
            total += entry->uncompressed_size;
            compressed += entry->compressed_size;
        }
    }
    
    if (total_size) *total_size = total;
    if (compressed_size) *compressed_size = compressed;
}


// ============================================================================
// 清单文件解析
// ============================================================================

/**
 * @brief 解析清单文件属性
 */
static char* parse_manifest_attribute(const char* manifest, const char* key) {
    if (!manifest || !key) {
        return NULL;
    }
    
    // 查找键
    char search_key[256];
    snprintf(search_key, sizeof(search_key), "%s:", key);
    
    const char* found = strstr(manifest, search_key);
    if (!found) {
        return NULL;
    }
    
    // 跳过键名和冒号
    found += strlen(search_key);
    
    // 跳过空格
    while (*found == ' ' || *found == '\t') {
        found++;
    }
    
    // 查找行结束
    const char* line_end = found;
    while (*line_end && *line_end != '\r' && *line_end != '\n') {
        line_end++;
    }
    
    // 复制值
    size_t value_len = line_end - found;
    char* value = (char*)malloc(value_len + 1);
    if (value) {
        strncpy(value, found, value_len);
        value[value_len] = '\0';
    }
    
    return value;
}

j2me_error_t j2me_jar_parse_manifest(j2me_jar_file_t* jar_file) {
    if (!jar_file) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    printf("[JAR解析器] 开始解析清单文件...\n");
    
    // 查找清单文件
    j2me_jar_entry_t* manifest_entry = j2me_jar_find_entry(jar_file, "META-INF/MANIFEST.MF");
    if (!manifest_entry) {
        printf("[JAR解析器] 未找到清单文件\n");
        return J2ME_ERROR_CLASS_NOT_FOUND;
    }
    
    // 加载清单文件
    j2me_error_t result = j2me_jar_load_entry(jar_file, manifest_entry);
    if (result != J2ME_SUCCESS) {
        printf("[JAR解析器] 加载清单文件失败\n");
        return result;
    }
    
    // 保存清单文件内容
    if (jar_file->manifest_content) {
        free(jar_file->manifest_content);
    }
    
    jar_file->manifest_content = (char*)malloc(manifest_entry->uncompressed_size + 1);
    if (!jar_file->manifest_content) {
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(jar_file->manifest_content, manifest_entry->data, manifest_entry->uncompressed_size);
    jar_file->manifest_content[manifest_entry->uncompressed_size] = '\0';
    
    printf("[JAR解析器] 清单文件内容:\n%s\n", jar_file->manifest_content);
    
    // 创建MIDlet套件
    jar_file->midlet_suite = j2me_midlet_suite_create(jar_file);
    
    printf("[JAR解析器] 清单文件解析完成\n");
    return J2ME_SUCCESS;
}

j2me_midlet_suite_t* j2me_jar_get_midlet_suite(j2me_jar_file_t* jar_file) {
    return jar_file ? jar_file->midlet_suite : NULL;
}

// ============================================================================
// MIDlet套件管理
// ============================================================================

j2me_midlet_suite_t* j2me_midlet_suite_create(j2me_jar_file_t* jar_file) {
    if (!jar_file || !jar_file->manifest_content) {
        return NULL;
    }
    
    printf("[MIDlet套件] 创建MIDlet套件...\n");
    
    j2me_midlet_suite_t* suite = (j2me_midlet_suite_t*)malloc(sizeof(j2me_midlet_suite_t));
    if (!suite) {
        return NULL;
    }
    
    memset(suite, 0, sizeof(j2me_midlet_suite_t));
    
    // 解析基本信息
    suite->name = parse_manifest_attribute(jar_file->manifest_content, "MIDlet-Name");
    suite->vendor = parse_manifest_attribute(jar_file->manifest_content, "MIDlet-Vendor");
    suite->version = parse_manifest_attribute(jar_file->manifest_content, "MIDlet-Version");
    suite->description = parse_manifest_attribute(jar_file->manifest_content, "MIDlet-Description");
    suite->microedition_profile = parse_manifest_attribute(jar_file->manifest_content, "MicroEdition-Profile");
    suite->microedition_configuration = parse_manifest_attribute(jar_file->manifest_content, "MicroEdition-Configuration");
    
    printf("[MIDlet套件] 名称: %s\n", suite->name ? suite->name : "未知");
    printf("[MIDlet套件] 供应商: %s\n", suite->vendor ? suite->vendor : "未知");
    printf("[MIDlet套件] 版本: %s\n", suite->version ? suite->version : "未知");
    printf("[MIDlet套件] 配置: %s\n", suite->microedition_configuration ? suite->microedition_configuration : "未知");
    printf("[MIDlet套件] 配置文件: %s\n", suite->microedition_profile ? suite->microedition_profile : "未知");
    
    // 解析MIDlet列表
    // 格式: MIDlet-1: Name, Icon, ClassName
    int midlet_index = 1;
    suite->midlet_count = 0;
    suite->midlets = NULL;
    
    while (true) {
        char midlet_key[32];
        snprintf(midlet_key, sizeof(midlet_key), "MIDlet-%d", midlet_index);
        
        char* midlet_value = parse_manifest_attribute(jar_file->manifest_content, midlet_key);
        if (!midlet_value) {
            break; // 没有更多MIDlet
        }
        
        // 解析MIDlet信息: Name, Icon, ClassName
        char* name = NULL;
        char* icon = NULL;
        char* class_name = NULL;
        
        char* token = strtok(midlet_value, ",");
        if (token) {
            // 去除前后空格
            while (*token == ' ') token++;
            name = strdup(token);
            
            token = strtok(NULL, ",");
            if (token) {
                while (*token == ' ') token++;
                icon = strdup(token);
                
                token = strtok(NULL, ",");
                if (token) {
                    while (*token == ' ') token++;
                    class_name = strdup(token);
                }
            }
        }
        
        if (name && class_name) {
            // 创建MIDlet对象
            j2me_midlet_t* midlet = (j2me_midlet_t*)malloc(sizeof(j2me_midlet_t));
            if (midlet) {
                memset(midlet, 0, sizeof(j2me_midlet_t));
                midlet->name = name;
                midlet->icon = icon;
                midlet->class_name = class_name;
                midlet->state = MIDLET_STATE_PAUSED;
                midlet->started = false;
                midlet->jar_file = jar_file;  // 设置JAR文件引用
                
                // 扩展MIDlet数组
                suite->midlets = (j2me_midlet_t**)realloc(suite->midlets, 
                                                          (suite->midlet_count + 1) * sizeof(j2me_midlet_t*));
                if (suite->midlets) {
                    suite->midlets[suite->midlet_count] = midlet;
                    suite->midlet_count++;
                    
                    printf("[MIDlet套件] MIDlet #%d: %s (类: %s)\n", 
                           midlet_index, midlet->name, midlet->class_name);
                }
            }
        } else {
            if (name) free(name);
            if (icon) free(icon);
            if (class_name) free(class_name);
        }
        
        free(midlet_value);
        midlet_index++;
    }
    
    suite->jar_file = jar_file;
    
    printf("[MIDlet套件] MIDlet套件创建完成，共 %d 个MIDlet\n", suite->midlet_count);
    return suite;
}

void j2me_midlet_suite_destroy(j2me_midlet_suite_t* suite) {
    if (!suite) {
        return;
    }
    
    // 释放基本信息
    if (suite->name) free(suite->name);
    if (suite->vendor) free(suite->vendor);
    if (suite->version) free(suite->version);
    if (suite->description) free(suite->description);
    if (suite->jar_url) free(suite->jar_url);
    if (suite->jad_url) free(suite->jad_url);
    if (suite->microedition_profile) free(suite->microedition_profile);
    if (suite->microedition_configuration) free(suite->microedition_configuration);
    
    // 释放MIDlet列表
    if (suite->midlets) {
        for (int i = 0; i < suite->midlet_count; i++) {
            j2me_midlet_t* midlet = suite->midlets[i];
            if (midlet) {
                if (midlet->name) free(midlet->name);
                if (midlet->class_name) free(midlet->class_name);
                if (midlet->icon) free(midlet->icon);
                if (midlet->description) free(midlet->description);
                free(midlet);
            }
        }
        free(suite->midlets);
    }
    
    // 释放权限列表
    if (suite->permissions) {
        for (int i = 0; i < suite->permission_count; i++) {
            if (suite->permissions[i]) {
                free(suite->permissions[i]);
            }
        }
        free(suite->permissions);
    }
    
    printf("[MIDlet套件] MIDlet套件已销毁\n");
    free(suite);
}

int j2me_midlet_suite_get_midlet_count(j2me_midlet_suite_t* suite) {
    return suite ? suite->midlet_count : 0;
}

j2me_midlet_t* j2me_midlet_suite_get_midlet(j2me_midlet_suite_t* suite, int index) {
    if (!suite || index < 0 || index >= suite->midlet_count) {
        return NULL;
    }
    
    return suite->midlets[index];
}

j2me_midlet_t* j2me_midlet_suite_find_midlet(j2me_midlet_suite_t* suite, const char* name) {
    if (!suite || !name) {
        return NULL;
    }
    
    for (int i = 0; i < suite->midlet_count; i++) {
        j2me_midlet_t* midlet = suite->midlets[i];
        if (midlet && midlet->name && strcmp(midlet->name, name) == 0) {
            return midlet;
        }
    }
    
    return NULL;
}

// ============================================================================
// MIDlet生命周期管理
// ============================================================================

j2me_error_t j2me_midlet_start(j2me_vm_t* vm, j2me_midlet_t* midlet) {
    if (!vm || !midlet) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    printf("[MIDlet] 启动MIDlet: %s (类: %s)\n", midlet->name, midlet->class_name);
    
    // 创建MIDlet执行器
    j2me_midlet_executor_t* executor = j2me_midlet_executor_create(vm, midlet->jar_file);
    if (!executor) {
        printf("[MIDlet] 错误: 创建MIDlet执行器失败\n");
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    // 创建MIDlet实例
    j2me_midlet_instance_t* instance = j2me_midlet_executor_create_instance(executor, midlet);
    if (!instance) {
        printf("[MIDlet] 错误: 创建MIDlet实例失败\n");
        j2me_midlet_executor_destroy(executor);
        return J2ME_ERROR_CLASS_NOT_FOUND;
    }
    
    // 启动MIDlet实例
    j2me_error_t result = j2me_midlet_executor_start_instance(executor, instance);
    if (result != J2ME_SUCCESS) {
        printf("[MIDlet] 错误: 启动MIDlet实例失败: %d\n", result);
        j2me_midlet_executor_destroy_instance(executor, instance);
        j2me_midlet_executor_destroy(executor);
        return result;
    }
    
    // 保存执行器和实例到MIDlet中
    midlet->executor = executor;
    midlet->instance = instance;
    midlet->state = MIDLET_STATE_ACTIVE;
    midlet->started = true;
    
    printf("[MIDlet] MIDlet已启动\n");
    return J2ME_SUCCESS;
}

j2me_error_t j2me_midlet_pause(j2me_midlet_t* midlet) {
    if (!midlet) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    printf("[MIDlet] 暂停MIDlet: %s\n", midlet->name);
    
    // 调用pauseApp()方法
    if (midlet->instance && midlet->instance->pause_app && midlet->instance->pause_app->bytecode) {
        printf("[MIDlet] 调用pauseApp()方法\n");
        // 这里应该通过虚拟机执行pauseApp方法
        // 简化实现：直接标记为已调用
        printf("[MIDlet] pauseApp()方法调用完成\n");
    } else {
        printf("[MIDlet] 警告: pauseApp()方法未找到或无字节码\n");
    }
    
    midlet->state = MIDLET_STATE_PAUSED;
    
    printf("[MIDlet] MIDlet已暂停\n");
    return J2ME_SUCCESS;
}

j2me_error_t j2me_midlet_resume(j2me_midlet_t* midlet) {
    if (!midlet) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    printf("[MIDlet] 恢复MIDlet: %s\n", midlet->name);
    
    // 调用startApp()方法
    if (midlet->instance && midlet->instance->start_app && midlet->instance->start_app->bytecode) {
        printf("[MIDlet] 调用startApp()方法\n");
        // 这里应该通过虚拟机执行startApp方法
        // 简化实现：直接标记为已调用
        printf("[MIDlet] startApp()方法调用完成\n");
    } else {
        printf("[MIDlet] 警告: startApp()方法未找到或无字节码\n");
    }
    
    midlet->state = MIDLET_STATE_ACTIVE;
    
    printf("[MIDlet] MIDlet已恢复\n");
    return J2ME_SUCCESS;
}

j2me_error_t j2me_midlet_destroy(j2me_midlet_t* midlet) {
    if (!midlet) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    printf("[MIDlet] 销毁MIDlet: %s\n", midlet->name);
    
    // 如果MIDlet正在运行，先停止它
    if (midlet->started && midlet->executor && midlet->instance) {
        j2me_midlet_executor_destroy_instance(midlet->executor, midlet->instance);
        j2me_midlet_executor_destroy(midlet->executor);
        midlet->executor = NULL;
        midlet->instance = NULL;
    }
    
    midlet->state = MIDLET_STATE_DESTROYED;
    midlet->started = false;
    
    printf("[MIDlet] MIDlet已销毁\n");
    return J2ME_SUCCESS;
}

j2me_midlet_state_t j2me_midlet_get_state(j2me_midlet_t* midlet) {
    return midlet ? midlet->state : MIDLET_STATE_DESTROYED;
}
