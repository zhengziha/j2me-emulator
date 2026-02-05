#include "j2me_network.h"
#include "j2me_vm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <curl/curl.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>

/**
 * @file j2me_network.c
 * @brief J2ME网络系统实现
 * 
 * 基于libcurl和BSD Socket的真实网络连接系统
 * 支持HTTP/HTTPS请求、TCP/UDP Socket通信和异步I/O
 */

#define MAX_CONNECTIONS     32
#define DEFAULT_TIMEOUT_MS  30000
#define MAX_URL_LENGTH      1024
#define MAX_HEADER_LENGTH   4096
#define MAX_RESPONSE_SIZE   (1024 * 1024)  // 1MB最大响应

// HTTP响应数据结构
typedef struct {
    uint8_t* data;
    size_t size;
    size_t capacity;
} http_response_data_t;

// 异步操作结构
typedef struct {
    j2me_connection_t* connection;
    pthread_t thread;
    bool active;
    j2me_error_t result;
} async_operation_t;

j2me_network_manager_t* j2me_network_manager_create(j2me_vm_t* vm) {
    if (!vm) {
        return NULL;
    }
    
    j2me_network_manager_t* manager = (j2me_network_manager_t*)malloc(sizeof(j2me_network_manager_t));
    if (!manager) {
        return NULL;
    }
    
    memset(manager, 0, sizeof(j2me_network_manager_t));
    
    manager->max_connections = MAX_CONNECTIONS;
    manager->connections = (j2me_connection_t**)calloc(MAX_CONNECTIONS, sizeof(j2me_connection_t*));
    if (!manager->connections) {
        free(manager);
        return NULL;
    }
    
    manager->timeout_ms = DEFAULT_TIMEOUT_MS;
    manager->proxy_enabled = false;
    
    printf("[网络系统] 网络管理器创建成功 (真实网络实现)\n");
    return manager;
}

void j2me_network_manager_destroy(j2me_network_manager_t* manager) {
    if (!manager) {
        return;
    }
    
    // 关闭所有连接
    for (int i = 0; i < manager->max_connections; i++) {
        if (manager->connections[i]) {
            j2me_connection_close(manager->connections[i]);
            manager->connections[i] = NULL; // 清空指针避免重复释放
        }
    }
    
    if (manager->initialized) {
        j2me_network_shutdown(manager);
    }
    
    if (manager->proxy_host) {
        free(manager->proxy_host);
        manager->proxy_host = NULL;
    }
    
    if (manager->connections) {
        free(manager->connections);
        manager->connections = NULL;
    }
    
    printf("[网络系统] 网络管理器已销毁\n");
    free(manager);
}

j2me_error_t j2me_network_initialize(j2me_network_manager_t* manager) {
    if (!manager || manager->initialized) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 初始化libcurl
    CURLcode curl_result = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (curl_result != CURLE_OK) {
        printf("[网络系统] libcurl初始化失败: %s\n", curl_easy_strerror(curl_result));
        return J2ME_ERROR_INITIALIZATION_FAILED;
    }
    
    manager->initialized = true;
    
    printf("[网络系统] 网络系统初始化成功 (libcurl + BSD Socket)\n");
    return J2ME_SUCCESS;
}

void j2me_network_shutdown(j2me_network_manager_t* manager) {
    if (!manager || !manager->initialized) {
        return;
    }
    
    // 关闭所有连接
    j2me_network_close_all(manager);
    
    // 清理libcurl
    curl_global_cleanup();
    
    manager->initialized = false;
    
    printf("[网络系统] 网络系统已关闭 (libcurl已清理)\n");
}

/**
 * @brief 在连接数组中找到空位
 */
static int find_free_connection_slot(j2me_network_manager_t* manager) {
    for (int i = 0; i < manager->max_connections; i++) {
        if (manager->connections[i] == NULL) {
            return i;
        }
    }
    return -1;
}

j2me_error_t j2me_network_parse_url(const char* url, j2me_connection_type_t* type,
                                    char** host, int* port, char** path) {
    if (!url || !type || !host || !port || !path) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    *type = CONNECTION_TYPE_UNKNOWN;
    *host = NULL;
    *port = 0;
    *path = NULL;
    
    // 解析协议
    if (strncmp(url, "http://", 7) == 0) {
        *type = CONNECTION_TYPE_HTTP;
        *port = 80;
        url += 7;
    } else if (strncmp(url, "https://", 8) == 0) {
        *type = CONNECTION_TYPE_HTTPS;
        *port = 443;
        url += 8;
    } else if (strncmp(url, "socket://", 9) == 0) {
        *type = CONNECTION_TYPE_SOCKET;
        url += 9;
    } else if (strncmp(url, "datagram://", 11) == 0) {
        *type = CONNECTION_TYPE_DATAGRAM;
        url += 11;
    } else if (strncmp(url, "file://", 7) == 0) {
        *type = CONNECTION_TYPE_FILE;
        url += 7;
    } else {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 查找路径分隔符
    const char* path_start = strchr(url, '/');
    const char* port_start = strchr(url, ':');
    
    // 确定主机名结束位置
    const char* host_end = url + strlen(url);
    if (path_start && (!port_start || path_start < port_start)) {
        host_end = path_start;
    } else if (port_start) {
        host_end = port_start;
    }
    
    // 提取主机名
    size_t host_len = host_end - url;
    *host = (char*)malloc(host_len + 1);
    if (!*host) {
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    strncpy(*host, url, host_len);
    (*host)[host_len] = '\0';
    
    // 提取端口号
    if (port_start && port_start < (path_start ? path_start : url + strlen(url))) {
        *port = atoi(port_start + 1);
    }
    
    // 提取路径
    if (path_start) {
        *path = strdup(path_start);
    } else {
        *path = strdup("/");
    }
    
    if (!*path) {
        free(*host);
        *host = NULL;
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    return J2ME_SUCCESS;
}

j2me_connection_t* j2me_connection_open(j2me_vm_t* vm, j2me_network_manager_t* manager, 
                                        const char* url, int mode, bool timeout) {
    if (!vm || !manager || !url) {
        return NULL;
    }
    
    int slot = find_free_connection_slot(manager);
    if (slot < 0) {
        printf("[网络系统] 连接数量已达上限\n");
        return NULL;
    }
    
    j2me_connection_t* connection = (j2me_connection_t*)malloc(sizeof(j2me_connection_t));
    if (!connection) {
        return NULL;
    }
    
    memset(connection, 0, sizeof(j2me_connection_t));
    
    // 解析URL
    char* host = NULL;
    char* path = NULL;
    int port = 0;
    j2me_error_t result = j2me_network_parse_url(url, &connection->type, &host, &port, &path);
    if (result != J2ME_SUCCESS) {
        free(connection);
        return NULL;
    }
    
    connection->url = strdup(url);
    connection->host = host;
    connection->port = port;
    connection->path = path;
    connection->state = CONNECTION_STATE_CLOSED;
    connection->socket_fd = -1;
    connection->http_method = HTTP_METHOD_GET;
    
    // 存储管理器引用和槽位索引
    connection->manager = manager;
    connection->slot_index = slot;
    
    manager->connections[slot] = connection;
    manager->active_connections++;
    manager->connections_opened++;
    
    printf("[网络系统] 创建连接 #%d: %s\n", slot, url);
    
    return connection;
}

void j2me_connection_close(j2me_connection_t* connection) {
    if (!connection) {
        return;
    }
    
    // 从管理器中移除连接
    if (connection->manager && connection->slot_index >= 0) {
        connection->manager->connections[connection->slot_index] = NULL;
        connection->manager->active_connections--;
        connection->manager->connections_closed++;
    }
    
    // 关闭Socket
    if (connection->socket_fd >= 0) {
        close(connection->socket_fd);
        connection->socket_fd = -1;
    }
    
    // 释放内存 (检查指针避免重复释放)
    if (connection->url) {
        free(connection->url);
        connection->url = NULL;
    }
    if (connection->host) {
        free(connection->host);
        connection->host = NULL;
    }
    if (connection->path) {
        free(connection->path);
        connection->path = NULL;
    }
    if (connection->request_headers) {
        free(connection->request_headers);
        connection->request_headers = NULL;
    }
    if (connection->response_headers) {
        free(connection->response_headers);
        connection->response_headers = NULL;
    }
    if (connection->request_body) {
        free(connection->request_body);
        connection->request_body = NULL;
    }
    if (connection->response_body) {
        free(connection->response_body);
        connection->response_body = NULL;
    }
    
    connection->state = CONNECTION_STATE_CLOSED;
    
    printf("[网络系统] 连接已关闭\n");
    free(connection);
}

j2me_connection_state_t j2me_connection_get_state(j2me_connection_t* connection) {
    return connection ? connection->state : CONNECTION_STATE_CLOSED;
}

j2me_error_t j2me_http_set_request_method(j2me_connection_t* connection, j2me_http_method_t method) {
    if (!connection || connection->type != CONNECTION_TYPE_HTTP) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    connection->http_method = method;
    
    printf("[网络系统] 设置HTTP方法: %s\n", j2me_network_get_method_name(method));
    return J2ME_SUCCESS;
}

j2me_error_t j2me_http_set_request_property(j2me_connection_t* connection, const char* key, const char* value) {
    if (!connection || !key || !value || connection->type != CONNECTION_TYPE_HTTP) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 简化实现：将头信息添加到字符串中
    char header_line[512];
    snprintf(header_line, sizeof(header_line), "%s: %s\r\n", key, value);
    
    if (!connection->request_headers) {
        connection->request_headers = strdup(header_line);
    } else {
        size_t old_len = strlen(connection->request_headers);
        size_t new_len = old_len + strlen(header_line) + 1;
        connection->request_headers = (char*)realloc(connection->request_headers, new_len);
        if (connection->request_headers) {
            strcat(connection->request_headers, header_line);
        }
    }
    
    printf("[网络系统] 设置HTTP头: %s = %s\n", key, value);
    return J2ME_SUCCESS;
}

char* j2me_http_get_header_field(j2me_connection_t* connection, const char* key) {
    if (!connection || !key || !connection->response_headers) {
        return NULL;
    }
    
    // 简化实现：在响应头中查找指定键
    char search_key[256];
    snprintf(search_key, sizeof(search_key), "%s:", key);
    
    char* found = strstr(connection->response_headers, search_key);
    if (!found) {
        return NULL;
    }
    
    // 跳过键名和冒号
    found += strlen(search_key);
    while (*found == ' ' || *found == '\t') {
        found++;
    }
    
    // 查找行结束
    char* line_end = strstr(found, "\r\n");
    if (!line_end) {
        line_end = found + strlen(found);
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

int j2me_http_get_response_code(j2me_connection_t* connection) {
    return connection ? connection->response_code : 0;
}

char* j2me_http_get_response_message(j2me_connection_t* connection) {
    if (!connection) {
        return NULL;
    }
    
    // 根据响应码返回标准消息
    switch (connection->response_code) {
        case HTTP_OK: return strdup("OK");
        case HTTP_CREATED: return strdup("Created");
        case HTTP_ACCEPTED: return strdup("Accepted");
        case HTTP_NOT_MODIFIED: return strdup("Not Modified");
        case HTTP_BAD_REQUEST: return strdup("Bad Request");
        case HTTP_UNAUTHORIZED: return strdup("Unauthorized");
        case HTTP_FORBIDDEN: return strdup("Forbidden");
        case HTTP_NOT_FOUND: return strdup("Not Found");
        case HTTP_METHOD_NOT_ALLOWED: return strdup("Method Not Allowed");
        case HTTP_INTERNAL_SERVER_ERROR: return strdup("Internal Server Error");
        case HTTP_NOT_IMPLEMENTED: return strdup("Not Implemented");
        case HTTP_BAD_GATEWAY: return strdup("Bad Gateway");
        case HTTP_SERVICE_UNAVAILABLE: return strdup("Service Unavailable");
        default: return strdup("Unknown");
    }
}

/**
 * @brief libcurl写入回调函数
 */
static size_t j2me_curl_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    http_response_data_t* response = (http_response_data_t*)userp;
    
    // 检查是否需要扩展缓冲区
    if (response->size + realsize + 1 > response->capacity) {
        size_t new_capacity = response->capacity * 2;
        if (new_capacity < response->size + realsize + 1) {
            new_capacity = response->size + realsize + 1;
        }
        
        uint8_t* new_data = (uint8_t*)realloc(response->data, new_capacity);
        if (!new_data) {
            return 0; // 内存分配失败
        }
        
        response->data = new_data;
        response->capacity = new_capacity;
    }
    
    // 复制数据
    memcpy(response->data + response->size, contents, realsize);
    response->size += realsize;
    response->data[response->size] = 0; // 添加null终止符
    
    return realsize;
}

/**
 * @brief libcurl头信息回调函数
 */
static size_t j2me_curl_header_callback(char* buffer, size_t size, size_t nitems, void* userdata) {
    size_t realsize = size * nitems;
    j2me_connection_t* connection = (j2me_connection_t*)userdata;
    
    // 添加头信息到响应头字符串
    if (!connection->response_headers) {
        connection->response_headers = (char*)malloc(realsize + 1);
        if (connection->response_headers) {
            memcpy(connection->response_headers, buffer, realsize);
            connection->response_headers[realsize] = '\0';
        }
    } else {
        size_t old_len = strlen(connection->response_headers);
        char* new_headers = (char*)realloc(connection->response_headers, old_len + realsize + 1);
        if (new_headers) {
            connection->response_headers = new_headers;
            memcpy(connection->response_headers + old_len, buffer, realsize);
            connection->response_headers[old_len + realsize] = '\0';
        }
    }
    
    return realsize;
}

j2me_error_t j2me_http_send_request(j2me_connection_t* connection, const uint8_t* data, size_t size) {
    if (!connection || connection->type != CONNECTION_TYPE_HTTP) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    printf("[网络系统] 发送真实HTTP请求: %s %s://%s:%d%s\n", 
           j2me_network_get_method_name(connection->http_method),
           connection->type == CONNECTION_TYPE_HTTPS ? "https" : "http",
           connection->host, connection->port, connection->path);
    
    // 创建CURL句柄
    CURL* curl = curl_easy_init();
    if (!curl) {
        printf("[网络系统] 创建CURL句柄失败\n");
        return J2ME_ERROR_INITIALIZATION_FAILED;
    }
    
    // 构建完整URL
    char full_url[MAX_URL_LENGTH];
    snprintf(full_url, sizeof(full_url), "%s://%s:%d%s",
             connection->type == CONNECTION_TYPE_HTTPS ? "https" : "http",
             connection->host, connection->port, connection->path);
    
    // 设置URL
    curl_easy_setopt(curl, CURLOPT_URL, full_url);
    
    // 设置HTTP方法
    switch (connection->http_method) {
        case HTTP_METHOD_GET:
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
            break;
        case HTTP_METHOD_POST:
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            if (data && size > 0) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, size);
            }
            break;
        case HTTP_METHOD_HEAD:
            curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
            break;
        case HTTP_METHOD_PUT:
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
            break;
        case HTTP_METHOD_DELETE:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
            break;
    }
    
    // 设置请求头
    struct curl_slist* headers = NULL;
    if (connection->request_headers) {
        // 解析请求头字符串并添加到curl
        char* headers_copy = strdup(connection->request_headers);
        char* line = strtok(headers_copy, "\r\n");
        while (line) {
            headers = curl_slist_append(headers, line);
            line = strtok(NULL, "\r\n");
        }
        free(headers_copy);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    
    // 设置响应数据回调
    http_response_data_t response_data = {0};
    response_data.capacity = 4096; // 初始4KB
    response_data.data = (uint8_t*)malloc(response_data.capacity);
    if (!response_data.data) {
        curl_easy_cleanup(curl);
        if (headers) curl_slist_free_all(headers);
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, j2me_curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
    
    // 设置头信息回调
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, j2me_curl_header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, connection);
    
    // 设置超时
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, DEFAULT_TIMEOUT_MS);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 10000); // 10秒连接超时
    
    // 设置SSL验证 (HTTPS) - 测试环境使用宽松设置
    if (connection->type == CONNECTION_TYPE_HTTPS) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // 测试时关闭peer验证
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); // 测试时关闭host验证
    }
    
    // 设置User-Agent
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "J2ME-Emulator/1.0");
    
    // 执行请求
    connection->state = CONNECTION_STATE_OPENING;
    CURLcode res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        printf("[网络系统] HTTP请求失败: %s\n", curl_easy_strerror(res));
        free(response_data.data);
        curl_easy_cleanup(curl);
        if (headers) curl_slist_free_all(headers);
        connection->state = CONNECTION_STATE_ERROR;
        return J2ME_ERROR_IO_EXCEPTION;
    }
    
    // 获取响应码
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    connection->response_code = (int)response_code;
    
    // 保存响应体
    if (connection->response_body) {
        free(connection->response_body);
    }
    connection->response_body = response_data.data;
    connection->response_body_size = response_data.size;
    
    // 清理
    curl_easy_cleanup(curl);
    if (headers) curl_slist_free_all(headers);
    
    connection->state = CONNECTION_STATE_OPEN;
    
    printf("[网络系统] HTTP请求成功: 响应码=%d, 数据大小=%zu bytes\n", 
           connection->response_code, connection->response_body_size);
    
    return J2ME_SUCCESS;
}

j2me_error_t j2me_http_receive_response(j2me_connection_t* connection, uint8_t* buffer, 
                                        size_t buffer_size, size_t* bytes_read) {
    if (!connection || !buffer || !bytes_read || connection->type != CONNECTION_TYPE_HTTP) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    *bytes_read = 0;
    
    if (!connection->response_body || connection->response_body_size == 0) {
        return J2ME_SUCCESS; // 没有更多数据
    }
    
    size_t copy_size = (buffer_size < connection->response_body_size) ? buffer_size : connection->response_body_size;
    memcpy(buffer, connection->response_body, copy_size);
    *bytes_read = copy_size;
    
    printf("[网络系统] 接收HTTP响应: %zu bytes\n", copy_size);
    
    return J2ME_SUCCESS;
}

j2me_connection_t* j2me_socket_open(j2me_vm_t* vm, j2me_network_manager_t* manager, 
                                    const char* host, int port) {
    if (!vm || !manager || !host || port <= 0) {
        return NULL;
    }
    
    char url[256];
    snprintf(url, sizeof(url), "socket://%s:%d", host, port);
    
    j2me_connection_t* connection = j2me_connection_open(vm, manager, url, 0, false);
    if (!connection) {
        return NULL;
    }
    
    printf("[网络系统] 创建真实Socket连接: %s:%d\n", host, port);
    
    // 创建Socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("[网络系统] 创建Socket失败: %s\n", strerror(errno));
        j2me_connection_close(connection);
        return NULL;
    }
    
    // 设置非阻塞模式
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    
    // 解析主机名
    struct hostent* server = gethostbyname(host);
    if (!server) {
        printf("[网络系统] 解析主机名失败: %s\n", host);
        close(sockfd);
        j2me_connection_close(connection);
        return NULL;
    }
    
    // 设置服务器地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    
    // 尝试连接
    connection->state = CONNECTION_STATE_OPENING;
    int result = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    if (result < 0 && errno != EINPROGRESS) {
        printf("[网络系统] Socket连接失败: %s\n", strerror(errno));
        close(sockfd);
        j2me_connection_close(connection);
        return NULL;
    }
    
    // 等待连接完成 (使用select)
    if (errno == EINPROGRESS) {
        fd_set write_fds;
        struct timeval timeout;
        
        FD_ZERO(&write_fds);
        FD_SET(sockfd, &write_fds);
        timeout.tv_sec = 10; // 10秒超时
        timeout.tv_usec = 0;
        
        result = select(sockfd + 1, NULL, &write_fds, NULL, &timeout);
        if (result <= 0) {
            printf("[网络系统] Socket连接超时\n");
            close(sockfd);
            j2me_connection_close(connection);
            return NULL;
        }
        
        // 检查连接是否成功
        int error;
        socklen_t len = sizeof(error);
        if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0) {
            printf("[网络系统] Socket连接失败: %s\n", strerror(error));
            close(sockfd);
            j2me_connection_close(connection);
            return NULL;
        }
    }
    
    connection->socket_fd = sockfd;
    connection->state = CONNECTION_STATE_OPEN;
    
    printf("[网络系统] Socket连接成功: fd=%d\n", sockfd);
    
    return connection;
}

j2me_connection_t* j2me_server_socket_open(j2me_vm_t* vm, j2me_network_manager_t* manager, int port) {
    if (!vm || !manager || port <= 0) {
        return NULL;
    }
    
    char url[256];
    snprintf(url, sizeof(url), "socket://:%d", port);
    
    j2me_connection_t* connection = j2me_connection_open(vm, manager, url, 0, false);
    if (!connection) {
        return NULL;
    }
    
    connection->is_server = true;
    
    printf("[网络系统] 创建真实服务器Socket: 端口%d\n", port);
    
    // 创建服务器Socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("[网络系统] 创建服务器Socket失败: %s\n", strerror(errno));
        j2me_connection_close(connection);
        return NULL;
    }
    
    // 设置Socket选项
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        printf("[网络系统] 设置Socket选项失败: %s\n", strerror(errno));
        close(sockfd);
        j2me_connection_close(connection);
        return NULL;
    }
    
    // 绑定地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("[网络系统] 绑定Socket失败: %s\n", strerror(errno));
        close(sockfd);
        j2me_connection_close(connection);
        return NULL;
    }
    
    // 开始监听
    if (listen(sockfd, 5) < 0) {
        printf("[网络系统] 监听Socket失败: %s\n", strerror(errno));
        close(sockfd);
        j2me_connection_close(connection);
        return NULL;
    }
    
    connection->socket_fd = sockfd;
    connection->state = CONNECTION_STATE_OPEN;
    
    printf("[网络系统] 服务器Socket创建成功: fd=%d, 端口=%d\n", sockfd, port);
    
    return connection;
}

j2me_connection_t* j2me_server_socket_accept(j2me_connection_t* server_socket) {
    if (!server_socket || !server_socket->is_server || server_socket->socket_fd < 0) {
        return NULL;
    }
    
    printf("[网络系统] 等待客户端连接...\n");
    
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    // 设置非阻塞模式检查是否有连接
    fd_set read_fds;
    struct timeval timeout;
    
    FD_ZERO(&read_fds);
    FD_SET(server_socket->socket_fd, &read_fds);
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000; // 100ms超时
    
    int result = select(server_socket->socket_fd + 1, &read_fds, NULL, NULL, &timeout);
    if (result <= 0) {
        return NULL; // 没有连接或超时
    }
    
    // 接受连接
    int client_fd = accept(server_socket->socket_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        printf("[网络系统] 接受连接失败: %s\n", strerror(errno));
        return NULL;
    }
    
    // 创建客户端连接对象
    char client_url[256];
    snprintf(client_url, sizeof(client_url), "socket://%s:%d", 
             inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    
    // 这里需要访问manager，但我们没有直接引用，简化处理
    j2me_connection_t* client_connection = (j2me_connection_t*)malloc(sizeof(j2me_connection_t));
    if (!client_connection) {
        close(client_fd);
        return NULL;
    }
    
    memset(client_connection, 0, sizeof(j2me_connection_t));
    client_connection->type = CONNECTION_TYPE_SOCKET;
    client_connection->state = CONNECTION_STATE_OPEN;
    client_connection->socket_fd = client_fd;
    client_connection->url = strdup(client_url);
    client_connection->host = strdup(inet_ntoa(client_addr.sin_addr));
    client_connection->port = ntohs(client_addr.sin_port);
    
    printf("[网络系统] 接受客户端连接: %s:%d (fd=%d)\n", 
           client_connection->host, client_connection->port, client_fd);
    
    return client_connection;
}

j2me_error_t j2me_socket_send(j2me_connection_t* connection, const uint8_t* data, 
                              size_t size, size_t* bytes_sent) {
    if (!connection || !data || !bytes_sent || connection->type != CONNECTION_TYPE_SOCKET) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    if (connection->socket_fd < 0) {
        return J2ME_ERROR_IO_EXCEPTION;
    }
    
    *bytes_sent = 0;
    
    // 使用非阻塞发送
    ssize_t result = send(connection->socket_fd, data, size, MSG_NOSIGNAL);
    
    if (result < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 缓冲区满，稍后重试
            return J2ME_SUCCESS;
        } else {
            printf("[网络系统] Socket发送失败: %s\n", strerror(errno));
            return J2ME_ERROR_IO_EXCEPTION;
        }
    }
    
    *bytes_sent = (size_t)result;
    
    printf("[网络系统] Socket发送数据: %zu bytes (实际发送: %zu)\n", size, *bytes_sent);
    
    return J2ME_SUCCESS;
}

j2me_error_t j2me_socket_receive(j2me_connection_t* connection, uint8_t* buffer, 
                                 size_t buffer_size, size_t* bytes_received) {
    if (!connection || !buffer || !bytes_received || connection->type != CONNECTION_TYPE_SOCKET) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    if (connection->socket_fd < 0) {
        return J2ME_ERROR_IO_EXCEPTION;
    }
    
    *bytes_received = 0;
    
    // 检查是否有数据可读
    fd_set read_fds;
    struct timeval timeout;
    
    FD_ZERO(&read_fds);
    FD_SET(connection->socket_fd, &read_fds);
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000; // 10ms超时
    
    int result = select(connection->socket_fd + 1, &read_fds, NULL, NULL, &timeout);
    if (result <= 0) {
        return J2ME_SUCCESS; // 没有数据或超时
    }
    
    // 接收数据
    ssize_t recv_result = recv(connection->socket_fd, buffer, buffer_size, MSG_DONTWAIT);
    
    if (recv_result < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 没有数据可读
            return J2ME_SUCCESS;
        } else {
            printf("[网络系统] Socket接收失败: %s\n", strerror(errno));
            return J2ME_ERROR_IO_EXCEPTION;
        }
    } else if (recv_result == 0) {
        // 连接已关闭
        printf("[网络系统] Socket连接已关闭\n");
        connection->state = CONNECTION_STATE_CLOSED;
        return J2ME_ERROR_IO_EXCEPTION;
    }
    
    *bytes_received = (size_t)recv_result;
    
    printf("[网络系统] Socket接收数据: %zu bytes\n", *bytes_received);
    
    return J2ME_SUCCESS;
}

j2me_connection_t* j2me_datagram_open(j2me_vm_t* vm, j2me_network_manager_t* manager, const char* url) {
    if (!vm || !manager || !url) {
        return NULL;
    }
    
    j2me_connection_t* connection = j2me_connection_open(vm, manager, url, 0, false);
    if (!connection) {
        return NULL;
    }
    
    printf("[网络系统] 创建真实数据报连接: %s\n", url);
    
    // 创建UDP Socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        printf("[网络系统] 创建UDP Socket失败: %s\n", strerror(errno));
        j2me_connection_close(connection);
        return NULL;
    }
    
    // 设置非阻塞模式
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    
    // 如果指定了端口，绑定本地地址
    if (connection->port > 0) {
        struct sockaddr_in local_addr;
        memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = INADDR_ANY;
        local_addr.sin_port = htons(connection->port);
        
        if (bind(sockfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
            printf("[网络系统] 绑定UDP Socket失败: %s\n", strerror(errno));
            close(sockfd);
            j2me_connection_close(connection);
            return NULL;
        }
        
        printf("[网络系统] UDP Socket绑定到端口: %d\n", connection->port);
    }
    
    connection->socket_fd = sockfd;
    connection->state = CONNECTION_STATE_OPEN;
    
    printf("[网络系统] 数据报连接创建成功: fd=%d\n", sockfd);
    
    return connection;
}

j2me_error_t j2me_datagram_send(j2me_connection_t* connection, const uint8_t* data, 
                                size_t size, const char* host, int port) {
    if (!connection || !data || !host || connection->type != CONNECTION_TYPE_DATAGRAM) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    if (connection->socket_fd < 0) {
        return J2ME_ERROR_IO_EXCEPTION;
    }
    
    // 解析目标主机
    struct hostent* server = gethostbyname(host);
    if (!server) {
        printf("[网络系统] 解析主机名失败: %s\n", host);
        return J2ME_ERROR_IO_EXCEPTION;
    }
    
    // 设置目标地址
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    memcpy(&dest_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    
    // 发送数据报
    ssize_t result = sendto(connection->socket_fd, data, size, 0,
                           (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    
    if (result < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("[网络系统] UDP发送缓冲区满，稍后重试\n");
            return J2ME_SUCCESS;
        } else {
            printf("[网络系统] UDP发送失败: %s\n", strerror(errno));
            return J2ME_ERROR_IO_EXCEPTION;
        }
    }
    
    printf("[网络系统] 发送数据报: %zu bytes 到 %s:%d (实际发送: %zd)\n", 
           size, host, port, result);
    
    return J2ME_SUCCESS;
}

j2me_error_t j2me_datagram_receive(j2me_connection_t* connection, uint8_t* buffer, 
                                   size_t buffer_size, size_t* bytes_received,
                                   char** sender_host, int* sender_port) {
    if (!connection || !buffer || !bytes_received || connection->type != CONNECTION_TYPE_DATAGRAM) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    if (connection->socket_fd < 0) {
        return J2ME_ERROR_IO_EXCEPTION;
    }
    
    *bytes_received = 0;
    if (sender_host) *sender_host = NULL;
    if (sender_port) *sender_port = 0;
    
    // 检查是否有数据可读
    fd_set read_fds;
    struct timeval timeout;
    
    FD_ZERO(&read_fds);
    FD_SET(connection->socket_fd, &read_fds);
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000; // 10ms超时
    
    int result = select(connection->socket_fd + 1, &read_fds, NULL, NULL, &timeout);
    if (result <= 0) {
        return J2ME_SUCCESS; // 没有数据或超时
    }
    
    // 接收数据报
    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);
    
    ssize_t recv_result = recvfrom(connection->socket_fd, buffer, buffer_size, MSG_DONTWAIT,
                                  (struct sockaddr*)&sender_addr, &sender_len);
    
    if (recv_result < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return J2ME_SUCCESS; // 没有数据
        } else {
            printf("[网络系统] UDP接收失败: %s\n", strerror(errno));
            return J2ME_ERROR_IO_EXCEPTION;
        }
    }
    
    *bytes_received = (size_t)recv_result;
    
    // 设置发送方信息
    if (sender_host) {
        *sender_host = strdup(inet_ntoa(sender_addr.sin_addr));
    }
    if (sender_port) {
        *sender_port = ntohs(sender_addr.sin_port);
    }
    
    printf("[网络系统] 接收数据报: %zu bytes 来自 %s:%d\n", 
           *bytes_received, inet_ntoa(sender_addr.sin_addr), ntohs(sender_addr.sin_port));
    
    return J2ME_SUCCESS;
}

const char* j2me_network_get_type_name(j2me_connection_type_t type) {
    switch (type) {
        case CONNECTION_TYPE_HTTP: return "HTTP";
        case CONNECTION_TYPE_HTTPS: return "HTTPS";
        case CONNECTION_TYPE_SOCKET: return "SOCKET";
        case CONNECTION_TYPE_DATAGRAM: return "DATAGRAM";
        case CONNECTION_TYPE_SMS: return "SMS";
        case CONNECTION_TYPE_FILE: return "FILE";
        default: return "UNKNOWN";
    }
}

const char* j2me_network_get_method_name(j2me_http_method_t method) {
    switch (method) {
        case HTTP_METHOD_GET: return "GET";
        case HTTP_METHOD_POST: return "POST";
        case HTTP_METHOD_HEAD: return "HEAD";
        case HTTP_METHOD_PUT: return "PUT";
        case HTTP_METHOD_DELETE: return "DELETE";
        default: return "UNKNOWN";
    }
}

void j2me_network_set_timeout(j2me_network_manager_t* manager, int timeout_ms) {
    if (manager) {
        manager->timeout_ms = timeout_ms;
        printf("[网络系统] 设置超时时间: %d ms\n", timeout_ms);
    }
}

void j2me_network_get_statistics(j2me_network_manager_t* manager, 
                                 size_t* bytes_sent, size_t* bytes_received,
                                 int* connections_opened, int* connections_closed) {
    if (!manager) {
        return;
    }
    
    if (bytes_sent) *bytes_sent = manager->bytes_sent;
    if (bytes_received) *bytes_received = manager->bytes_received;
    if (connections_opened) *connections_opened = manager->connections_opened;
    if (connections_closed) *connections_closed = manager->connections_closed;
}

void j2me_network_update(j2me_network_manager_t* manager) {
    if (!manager) {
        return;
    }
    
    // 检查连接状态，处理超时等
    for (int i = 0; i < manager->max_connections; i++) {
        j2me_connection_t* connection = manager->connections[i];
        if (connection && connection->state == CONNECTION_STATE_OPENING) {
            // 简化实现：立即标记为打开
            connection->state = CONNECTION_STATE_OPEN;
        }
    }
}

void j2me_network_close_all(j2me_network_manager_t* manager) {
    if (!manager) {
        return;
    }
    
    for (int i = 0; i < manager->max_connections; i++) {
        if (manager->connections[i]) {
            j2me_connection_close(manager->connections[i]);
            manager->connections[i] = NULL;
            manager->connections_closed++;
        }
    }
    
    manager->active_connections = 0;
    
    printf("[网络系统] 所有连接已关闭\n");
}