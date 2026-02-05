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

/**
 * @file j2me_network.c
 * @brief J2ME网络系统实现
 * 
 * 基于BSD Socket的网络连接系统
 */

#define MAX_CONNECTIONS     32
#define DEFAULT_TIMEOUT_MS  30000
#define MAX_URL_LENGTH      1024
#define MAX_HEADER_LENGTH   4096

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
    
    printf("[网络系统] 网络管理器创建成功\n");
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
    }
    
    free(manager->connections);
    free(manager);
    
    printf("[网络系统] 网络管理器已销毁\n");
}

j2me_error_t j2me_network_initialize(j2me_network_manager_t* manager) {
    if (!manager || manager->initialized) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 网络系统初始化 (BSD Socket不需要特殊初始化)
    manager->initialized = true;
    
    printf("[网络系统] 网络系统初始化成功\n");
    return J2ME_SUCCESS;
}

void j2me_network_shutdown(j2me_network_manager_t* manager) {
    if (!manager || !manager->initialized) {
        return;
    }
    
    // 关闭所有连接
    j2me_network_close_all(manager);
    
    manager->initialized = false;
    
    printf("[网络系统] 网络系统已关闭\n");
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
    
    // 关闭Socket
    if (connection->socket_fd >= 0) {
        close(connection->socket_fd);
        connection->socket_fd = -1;
    }
    
    // 释放内存
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

j2me_error_t j2me_http_send_request(j2me_connection_t* connection, const uint8_t* data, size_t size) {
    if (!connection || connection->type != CONNECTION_TYPE_HTTP) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    printf("[网络系统] 发送HTTP请求: %s %s (简化实现)\n", 
           j2me_network_get_method_name(connection->http_method), connection->path);
    
    // 简化实现：模拟HTTP请求
    connection->state = CONNECTION_STATE_OPENING;
    
    // 模拟成功响应
    connection->response_code = HTTP_OK;
    connection->response_headers = strdup("Content-Type: text/html\r\nContent-Length: 13\r\n\r\n");
    
    const char* response_body = "Hello, World!";
    connection->response_body_size = strlen(response_body);
    connection->response_body = (uint8_t*)malloc(connection->response_body_size);
    if (connection->response_body) {
        memcpy(connection->response_body, response_body, connection->response_body_size);
    }
    
    connection->state = CONNECTION_STATE_OPEN;
    
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
    
    printf("[网络系统] 创建Socket连接: %s:%d (简化实现)\n", host, port);
    
    // 简化实现：不实际创建Socket
    connection->socket_fd = -1; // 模拟Socket
    connection->state = CONNECTION_STATE_CLOSED;
    
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
    
    printf("[网络系统] 创建服务器Socket: 端口%d (简化实现)\n", port);
    
    return connection;
}

j2me_connection_t* j2me_server_socket_accept(j2me_connection_t* server_socket) {
    if (!server_socket || !server_socket->is_server) {
        return NULL;
    }
    
    printf("[网络系统] 接受客户端连接 (简化实现)\n");
    
    // 简化实现：返回NULL表示没有连接
    return NULL;
}

j2me_error_t j2me_socket_send(j2me_connection_t* connection, const uint8_t* data, 
                              size_t size, size_t* bytes_sent) {
    if (!connection || !data || !bytes_sent || connection->type != CONNECTION_TYPE_SOCKET) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    printf("[网络系统] Socket发送数据: %zu bytes (简化实现)\n", size);
    
    // 简化实现：模拟发送成功
    *bytes_sent = size;
    
    return J2ME_SUCCESS;
}

j2me_error_t j2me_socket_receive(j2me_connection_t* connection, uint8_t* buffer, 
                                 size_t buffer_size, size_t* bytes_received) {
    if (!connection || !buffer || !bytes_received || connection->type != CONNECTION_TYPE_SOCKET) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    printf("[网络系统] Socket接收数据 (简化实现)\n");
    
    // 简化实现：没有数据可接收
    *bytes_received = 0;
    
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
    
    printf("[网络系统] 创建数据报连接: %s (简化实现)\n", url);
    
    return connection;
}

j2me_error_t j2me_datagram_send(j2me_connection_t* connection, const uint8_t* data, 
                                size_t size, const char* host, int port) {
    if (!connection || !data || !host || connection->type != CONNECTION_TYPE_DATAGRAM) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    printf("[网络系统] 发送数据报: %zu bytes 到 %s:%d (简化实现)\n", size, host, port);
    
    return J2ME_SUCCESS;
}

j2me_error_t j2me_datagram_receive(j2me_connection_t* connection, uint8_t* buffer, 
                                   size_t buffer_size, size_t* bytes_received,
                                   char** sender_host, int* sender_port) {
    if (!connection || !buffer || !bytes_received || connection->type != CONNECTION_TYPE_DATAGRAM) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    printf("[网络系统] 接收数据报 (简化实现)\n");
    
    // 简化实现：没有数据可接收
    *bytes_received = 0;
    if (sender_host) *sender_host = NULL;
    if (sender_port) *sender_port = 0;
    
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