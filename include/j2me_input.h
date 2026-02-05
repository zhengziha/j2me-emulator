#ifndef J2ME_INPUT_H
#define J2ME_INPUT_H

#include "j2me_types.h"
#include <SDL2/SDL.h>

/**
 * @file j2me_input.h
 * @brief J2ME输入系统
 * 
 * 处理键盘、游戏键和触摸输入事件
 */

// 前向声明
typedef struct j2me_input_manager j2me_input_manager_t;
typedef struct j2me_key_event j2me_key_event_t;

// MIDP键码定义 (javax.microedition.lcdui.Canvas)
#define KEY_NUM0        48
#define KEY_NUM1        49
#define KEY_NUM2        50
#define KEY_NUM3        51
#define KEY_NUM4        52
#define KEY_NUM5        53
#define KEY_NUM6        54
#define KEY_NUM7        55
#define KEY_NUM8        56
#define KEY_NUM9        57
#define KEY_STAR        42      // *
#define KEY_POUND       35      // #

// 游戏键码
#define KEY_UP          -1
#define KEY_DOWN        -2
#define KEY_LEFT        -3
#define KEY_RIGHT       -4
#define KEY_FIRE        -5
#define KEY_GAME_A      -6
#define KEY_GAME_B      -7
#define KEY_GAME_C      -8
#define KEY_GAME_D      -9

// 软键
#define KEY_SOFT_LEFT   -21
#define KEY_SOFT_RIGHT  -22

// 其他常用键
#define KEY_SELECT      -10
#define KEY_CLEAR       -12
#define KEY_SEND        -11
#define KEY_END         -13

// 事件类型
typedef enum {
    INPUT_EVENT_KEY_PRESSED,
    INPUT_EVENT_KEY_RELEASED,
    INPUT_EVENT_KEY_REPEATED,
    INPUT_EVENT_POINTER_PRESSED,
    INPUT_EVENT_POINTER_RELEASED,
    INPUT_EVENT_POINTER_DRAGGED
} j2me_input_event_type_t;

// 键盘事件
struct j2me_key_event {
    j2me_input_event_type_t type;
    int key_code;               // MIDP键码
    char key_char;              // 字符 (如果适用)
    uint32_t timestamp;         // 时间戳
    bool is_game_key;           // 是否为游戏键
};

// 指针事件
typedef struct {
    j2me_input_event_type_t type;
    int x, y;                   // 坐标
    uint32_t timestamp;         // 时间戳
} j2me_pointer_event_t;

// 输入事件联合
typedef union {
    j2me_input_event_type_t type;
    j2me_key_event_t key_event;
    j2me_pointer_event_t pointer_event;
} j2me_input_event_t;

// 事件回调函数类型
typedef void (*j2me_key_callback_t)(j2me_key_event_t* event, void* user_data);
typedef void (*j2me_pointer_callback_t)(j2me_pointer_event_t* event, void* user_data);

// 输入管理器
struct j2me_input_manager {
    // 键盘状态
    bool key_states[512];       // 键盘状态数组
    bool game_key_states[10];   // 游戏键状态
    
    // 指针状态
    int pointer_x, pointer_y;   // 当前指针位置
    bool pointer_pressed;       // 指针是否按下
    
    // 事件回调
    j2me_key_callback_t key_callback;
    j2me_pointer_callback_t pointer_callback;
    void* callback_user_data;
    
    // 键映射
    int sdl_to_midp_map[512];   // SDL键码到MIDP键码的映射
};

/**
 * @brief 创建输入管理器
 * @return 输入管理器指针
 */
j2me_input_manager_t* j2me_input_manager_create(void);

/**
 * @brief 销毁输入管理器
 * @param manager 输入管理器
 */
void j2me_input_manager_destroy(j2me_input_manager_t* manager);

/**
 * @brief 设置键盘事件回调
 * @param manager 输入管理器
 * @param callback 回调函数
 * @param user_data 用户数据
 */
void j2me_input_set_key_callback(j2me_input_manager_t* manager, j2me_key_callback_t callback, void* user_data);

/**
 * @brief 设置指针事件回调
 * @param manager 输入管理器
 * @param callback 回调函数
 * @param user_data 用户数据
 */
void j2me_input_set_pointer_callback(j2me_input_manager_t* manager, j2me_pointer_callback_t callback, void* user_data);

/**
 * @brief 处理SDL事件
 * @param manager 输入管理器
 * @param event SDL事件
 * @return 是否处理了事件
 */
bool j2me_input_handle_sdl_event(j2me_input_manager_t* manager, SDL_Event* event);

/**
 * @brief 检查键是否按下
 * @param manager 输入管理器
 * @param key_code MIDP键码
 * @return 是否按下
 */
bool j2me_input_is_key_pressed(j2me_input_manager_t* manager, int key_code);

/**
 * @brief 检查游戏键是否按下
 * @param manager 输入管理器
 * @param game_action 游戏动作
 * @return 是否按下
 */
bool j2me_input_is_game_action_pressed(j2me_input_manager_t* manager, int game_action);

/**
 * @brief 获取键的游戏动作
 * @param key_code MIDP键码
 * @return 游戏动作码
 */
int j2me_input_get_game_action(int key_code);

/**
 * @brief 获取游戏动作对应的键码
 * @param game_action 游戏动作
 * @return 键码
 */
int j2me_input_get_key_code(int game_action);

/**
 * @brief 获取键名称
 * @param key_code MIDP键码
 * @return 键名称字符串
 */
const char* j2me_input_get_key_name(int key_code);

/**
 * @brief 获取当前指针位置
 * @param manager 输入管理器
 * @param x 输出X坐标
 * @param y 输出Y坐标
 */
void j2me_input_get_pointer_position(j2me_input_manager_t* manager, int* x, int* y);

/**
 * @brief 检查指针是否按下
 * @param manager 输入管理器
 * @return 是否按下
 */
bool j2me_input_is_pointer_pressed(j2me_input_manager_t* manager);

/**
 * @brief 更新输入状态 (每帧调用)
 * @param manager 输入管理器
 */
void j2me_input_update(j2me_input_manager_t* manager);

/**
 * @brief 重置输入状态
 * @param manager 输入管理器
 */
void j2me_input_reset(j2me_input_manager_t* manager);

// 游戏动作常量 (javax.microedition.lcdui.game.GameCanvas)
#define GAME_UP_PRESSED     1
#define GAME_DOWN_PRESSED   2
#define GAME_LEFT_PRESSED   4
#define GAME_RIGHT_PRESSED  8
#define GAME_FIRE_PRESSED   16
#define GAME_A_PRESSED      32
#define GAME_B_PRESSED      64
#define GAME_C_PRESSED      128
#define GAME_D_PRESSED      256

/**
 * @brief 获取游戏键状态位掩码
 * @param manager 输入管理器
 * @return 游戏键状态位掩码
 */
int j2me_input_get_key_states(j2me_input_manager_t* manager);

#endif // J2ME_INPUT_H