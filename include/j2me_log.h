#ifndef J2ME_LOG_H
#define J2ME_LOG_H

#include <stdio.h>

// 日志级别
typedef enum {
    J2ME_LOG_LEVEL_NONE = 0,
    J2ME_LOG_LEVEL_ERROR = 1,
    J2ME_LOG_LEVEL_WARN = 2,
    J2ME_LOG_LEVEL_INFO = 3,
    J2ME_LOG_LEVEL_DEBUG = 4,
} j2me_log_level_t;

// 全局日志级别
extern j2me_log_level_t g_j2me_log_level;

// 日志宏
#define LOG_ERROR(fmt, ...) \
    do { if (g_j2me_log_level >= J2ME_LOG_LEVEL_ERROR) \
        printf("[错误] " fmt "\n", ##__VA_ARGS__); } while(0)

#define LOG_WARN(fmt, ...) \
    do { if (g_j2me_log_level >= J2ME_LOG_LEVEL_WARN) \
        printf("[警告] " fmt "\n", ##__VA_ARGS__); } while(0)

#define LOG_INFO(fmt, ...) \
    do { if (g_j2me_log_level >= J2ME_LOG_LEVEL_INFO) \
        printf(fmt "\n", ##__VA_ARGS__); } while(0)

#define LOG_DEBUG(fmt, ...) \
    do { if (g_j2me_log_level >= J2ME_LOG_LEVEL_DEBUG) \
        printf(fmt, ##__VA_ARGS__); } while(0)

static inline void j2me_log_set_level(j2me_log_level_t level) {
    g_j2me_log_level = level;
}

static inline j2me_log_level_t j2me_log_get_level(void) {
    return g_j2me_log_level;
}

#endif // J2ME_LOG_H
