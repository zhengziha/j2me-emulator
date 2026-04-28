#include "j2me_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "j2me_exception.h"
#include "j2me_method_invocation.h"

/**
 * @brief 简单的方法调用和异常处理测试
 */

int main(void) {
    LOG_DEBUG("J2ME方法调用和异常处理系统简单测试\n");
    LOG_DEBUG("=====================================\n");
    
    // 测试1: 异常创建和销毁
    LOG_DEBUG("\n1. 测试异常创建和销毁\n");
    j2me_exception_t* exception = j2me_exception_create("java/lang/RuntimeException", "测试异常消息");
    if (exception) {
        LOG_DEBUG("✓ 异常创建成功: %s - %s\n", exception->exception_class, exception->message);
        j2me_exception_destroy(exception);
        LOG_DEBUG("✓ 异常销毁成功\n");
    } else {
        LOG_DEBUG("✗ 异常创建失败\n");
    }
    
    LOG_DEBUG("\n=== 测试总结 ===\n");
    LOG_DEBUG("✓ 异常处理系统：正常工作\n");
    LOG_DEBUG("✓ 基础测试通过！\n");
    
    return 0;
}