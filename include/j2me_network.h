#ifndef J2ME_NETWORK_H
#define J2ME_NETWORK_H

#include "j2me_types.h"
#include "j2me_object.h"

/**
 * @file j2me_network.h
 * @brief J2ME网络系统
 * 
 * 实现GCF (Generic Connection Framework) 的网络功能
 */

// 前向声明
typedef struct j2me_network_manager j2me_network_manager_t;
typedef struct j2me_connection j2me_connection_t;

// 连接类型
typedef enum {
    CONNECTION_TYPE_UNKNOWN = 0,
    CONNECTION_TYPE_HTTP,
    CONNECTION_TYPE_HTTPS,
    CONNECTION_TYPE_SOCKET,
    CONNECTION_TYPE_DATAGRAM,
    CONNECTION_TYPE_SMS,
    CONNECTION_TYPE_FILE
} j2me_connection_type_t;

// 连接状态
typedef enum {
    CONNECTION_STATE_CLOSED = 0,
    CONNECTION_STATE_OPENING,
    CONNECTION_STATE_OPEN,
    CONNECTION_STATE_ERROR
} j2me_connection_state_t;

// HTTP方法
typedef enum {
    HTTP_METHOD_GET = 0,
    HTTP_METHOD_POST,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE
} j2me_http_method_t;

// HTTP响应码
typedef enum {
    HTTP_OK = 200,
    HTTP_CREATED = 201,
    HTTP_ACCEPTED = 202,
    HTTP_NOT_MODIFIED = 304,
    HTTP_BAD_REQUEST = 400,
    HTTP_UNAUTHORIZED = 401,
    HTTP_FORBIDDEN = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_METHOD_NOT_ALLOWED = 405,
    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_NOT_IMPLEMENTED = 501,
    HTTP_BAD_GATEWAY = 502,
    HTTP_SERVICE_UNAVAILABLE = 503
} j2me_http_response_code_t;

// 网络连接结构
struct j2me_connection {
    j2me_object_header_t header;        // 对象头
    j2me_connection_type_t type;        // 连接类型
    j2me_connection_state_t state;      // 连接状态
    char* url;                          // 连接URL
    char* host;                         // 主机名
    int port;                           // 端口号
    char* path;                         // 路径
    
    // 管理器引用
    j2me_network_manager_t* manager;    // 所属管理器
    int slot_index;                     // 在管理器中的槽位索引
    
    // HTTP特定字段
    j2me_http_method_t http_method;     // HTTP方法
    int response_code;                  // 响应码
    char* request_headers;              // 请求头
    char* response_headers;             // 响应头
    uint8_t* request_body;              // 请求体
    size_t request_body_size;           // 请求体大小
    uint8_t* response_body;             // 响应体
    size_t response_body_size;          // 响应体大小
    
    // Socket特定字段
    int socket_fd;                      // Socket文件描述符
    bool is_server;                     // 是否为服务器Socket
    
    // 回调函数
    void (*data_callback)(j2me_connection_t* conn, const uint8_t* data, size_t size, void* user_data);
    void (*error_callback)(j2me_connection_t* conn, j2me_error_t error, void* user_data);
    void* callback_user_data;
};

// 网络管理器
struct j2me_network_manager {
    bool initialized;                   // 是否已初始化
    int max_connections;                // 最大连接数
    j2me_connection_t** connections;    // 连接数组
    int active_connections;             // 活跃连接数
    
    // 网络设置
    int timeout_ms;                     // 超时时间 (毫秒)
    bool proxy_enabled;                 // 是否启用代理
    char* proxy_host;                   // 代理主机
    int proxy_port;                     // 代理端口
    
    // 统计信息
    size_t bytes_sent;                  // 发送字节数
    size_t bytes_received;              // 接收字节数
    int connections_opened;             // 打开的连接数
    int connections_closed;             // 关闭的连接数
};

/**
 * @brief 创建网络管理器
 * @param vm 虚拟机实例
 * @return 网络管理器指针
 */
j2me_network_manager_t* j2me_network_manager_create(j2me_vm_t* vm);

/**
 * @brief 销毁网络管理器
 * @param manager 网络管理器
 */
void j2me_network_manager_destroy(j2me_network_manager_t* manager);

/**
 * @brief 初始化网络系统
 * @param manager 网络管理器
 * @return 错误码
 */
j2me_error_t j2me_network_initialize(j2me_network_manager_t* manager);

/**
 * @brief 关闭网络系统
 * @param manager 网络管理器
 */
void j2me_network_shutdown(j2me_network_manager_t* manager);

/**
 * @brief 打开连接 (Connector.open)
 * @param vm 虚拟机实例
 * @param manager 网络管理器
 * @param url 连接URL
 * @param mode 访问模式 (READ, WRITE, READ_WRITE)
 * @param timeout 是否启用超时
 * @return 连接指针
 */
j2me_connection_t* j2me_connection_open(j2me_vm_t* vm, j2me_network_manager_t* manager, 
                                        const char* url, int mode, bool timeout);

/**
 * @brief 关闭连接
 * @param connection 连接
 */
void j2me_connection_close(j2me_connection_t* connection);

/**
 * @brief 获取连接状态
 * @param connection 连接
 * @return 连接状态
 */
j2me_connection_state_t j2me_connection_get_state(j2me_connection_t* connection);

// HTTP连接功能

/**
 * @brief 设置HTTP请求方法
 * @param connection HTTP连接
 * @param method HTTP方法
 * @return 错误码
 */
j2me_error_t j2me_http_set_request_method(j2me_connection_t* connection, j2me_http_method_t method);

/**
 * @brief 设置HTTP请求头
 * @param connection HTTP连接
 * @param key 头名称
 * @param value 头值
 * @return 错误码
 */
j2me_error_t j2me_http_set_request_property(j2me_connection_t* connection, const char* key, const char* value);

/**
 * @brief 获取HTTP响应头
 * @param connection HTTP连接
 * @param key 头名称
 * @return 头值 (需要释放)
 */
char* j2me_http_get_header_field(j2me_connection_t* connection, const char* key);

/**
 * @brief 获取HTTP响应码
 * @param connection HTTP连接
 * @return 响应码
 */
int j2me_http_get_response_code(j2me_connection_t* connection);

/**
 * @brief 获取HTTP响应消息
 * @param connection HTTP连接
 * @return 响应消息 (需要释放)
 */
char* j2me_http_get_response_message(j2me_connection_t* connection);

/**
 * @brief 发送HTTP请求
 * @param connection HTTP连接
 * @param data 请求数据
 * @param size 数据大小
 * @return 错误码
 */
j2me_error_t j2me_http_send_request(j2me_connection_t* connection, const uint8_t* data, size_t size);

/**
 * @brief 接收HTTP响应
 * @param connection HTTP连接
 * @param buffer 接收缓冲区
 * @param buffer_size 缓冲区大小
 * @param bytes_read 实际读取字节数
 * @return 错误码
 */
j2me_error_t j2me_http_receive_response(j2me_connection_t* connection, uint8_t* buffer, 
                                        size_t buffer_size, size_t* bytes_read);

// Socket连接功能

/**
 * @brief 创建Socket连接
 * @param vm 虚拟机实例
 * @param manager 网络管理器
 * @param host 主机名
 * @param port 端口号
 * @return 连接指针
 */
j2me_connection_t* j2me_socket_open(j2me_vm_t* vm, j2me_network_manager_t* manager, 
                                    const char* host, int port);

/**
 * @brief 创建服务器Socket
 * @param vm 虚拟机实例
 * @param manager 网络管理器
 * @param port 监听端口
 * @return 连接指针
 */
j2me_connection_t* j2me_server_socket_open(j2me_vm_t* vm, j2me_network_manager_t* manager, int port);

/**
 * @brief 接受客户端连接
 * @param server_socket 服务器Socket
 * @return 客户端连接
 */
j2me_connection_t* j2me_server_socket_accept(j2me_connection_t* server_socket);

/**
 * @brief 发送数据
 * @param connection Socket连接
 * @param data 数据
 * @param size 数据大小
 * @param bytes_sent 实际发送字节数
 * @return 错误码
 */
j2me_error_t j2me_socket_send(j2me_connection_t* connection, const uint8_t* data, 
                              size_t size, size_t* bytes_sent);

/**
 * @brief 接收数据
 * @param connection Socket连接
 * @param buffer 接收缓冲区
 * @param buffer_size 缓冲区大小
 * @param bytes_received 实际接收字节数
 * @return 错误码
 */
j2me_error_t j2me_socket_receive(j2me_connection_t* connection, uint8_t* buffer, 
                                 size_t buffer_size, size_t* bytes_received);

// 数据报连接功能

/**
 * @brief 创建数据报连接
 * @param vm 虚拟机实例
 * @param manager 网络管理器
 * @param url 连接URL
 * @return 连接指针
 */
j2me_connection_t* j2me_datagram_open(j2me_vm_t* vm, j2me_network_manager_t* manager, const char* url);

/**
 * @brief 发送数据报
 * @param connection 数据报连接
 * @param data 数据
 * @param size 数据大小
 * @param host 目标主机
 * @param port 目标端口
 * @return 错误码
 */
j2me_error_t j2me_datagram_send(j2me_connection_t* connection, const uint8_t* data, 
                                size_t size, const char* host, int port);

/**
 * @brief 接收数据报
 * @param connection 数据报连接
 * @param buffer 接收缓冲区
 * @param buffer_size 缓冲区大小
 * @param bytes_received 实际接收字节数
 * @param sender_host 发送方主机 (输出参数)
 * @param sender_port 发送方端口 (输出参数)
 * @return 错误码
 */
j2me_error_t j2me_datagram_receive(j2me_connection_t* connection, uint8_t* buffer, 
                                   size_t buffer_size, size_t* bytes_received,
                                   char** sender_host, int* sender_port);

// 网络工具函数

/**
 * @brief 解析URL
 * @param url URL字符串
 * @param type 连接类型 (输出参数)
 * @param host 主机名 (输出参数，需要释放)
 * @param port 端口号 (输出参数)
 * @param path 路径 (输出参数，需要释放)
 * @return 错误码
 */
j2me_error_t j2me_network_parse_url(const char* url, j2me_connection_type_t* type,
                                    char** host, int* port, char** path);

/**
 * @brief 获取连接类型名称
 * @param type 连接类型
 * @return 类型名称字符串
 */
const char* j2me_network_get_type_name(j2me_connection_type_t type);

/**
 * @brief 获取HTTP方法名称
 * @param method HTTP方法
 * @return 方法名称字符串
 */
const char* j2me_network_get_method_name(j2me_http_method_t method);

/**
 * @brief 设置网络超时
 * @param manager 网络管理器
 * @param timeout_ms 超时时间 (毫秒)
 */
void j2me_network_set_timeout(j2me_network_manager_t* manager, int timeout_ms);

/**
 * @brief 获取网络统计信息
 * @param manager 网络管理器
 * @param bytes_sent 发送字节数 (输出参数)
 * @param bytes_received 接收字节数 (输出参数)
 * @param connections_opened 打开连接数 (输出参数)
 * @param connections_closed 关闭连接数 (输出参数)
 */
void j2me_network_get_statistics(j2me_network_manager_t* manager, 
                                 size_t* bytes_sent, size_t* bytes_received,
                                 int* connections_opened, int* connections_closed);

/**
 * @brief 更新网络系统 (每帧调用)
 * @param manager 网络管理器
 */
void j2me_network_update(j2me_network_manager_t* manager);

/**
 * @brief 关闭所有连接
 * @param manager 网络管理器
 */
void j2me_network_close_all(j2me_network_manager_t* manager);

#endif // J2ME_NETWORK_H