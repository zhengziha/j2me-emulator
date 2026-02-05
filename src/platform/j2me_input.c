#include "j2me_input.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @file j2me_input.c
 * @brief J2ME输入系统实现
 * 
 * 处理SDL事件并转换为MIDP输入事件
 */

/**
 * @brief 初始化SDL到MIDP键码映射
 * @param manager 输入管理器
 */
static void init_key_mapping(j2me_input_manager_t* manager) {
    // 清空映射表
    for (int i = 0; i < 512; i++) {
        manager->sdl_to_midp_map[i] = 0;
    }
    
    // 数字键映射
    manager->sdl_to_midp_map[SDLK_0] = KEY_NUM0;
    manager->sdl_to_midp_map[SDLK_1] = KEY_NUM1;
    manager->sdl_to_midp_map[SDLK_2] = KEY_NUM2;
    manager->sdl_to_midp_map[SDLK_3] = KEY_NUM3;
    manager->sdl_to_midp_map[SDLK_4] = KEY_NUM4;
    manager->sdl_to_midp_map[SDLK_5] = KEY_NUM5;
    manager->sdl_to_midp_map[SDLK_6] = KEY_NUM6;
    manager->sdl_to_midp_map[SDLK_7] = KEY_NUM7;
    manager->sdl_to_midp_map[SDLK_8] = KEY_NUM8;
    manager->sdl_to_midp_map[SDLK_9] = KEY_NUM9;
    
    // 特殊键映射
    manager->sdl_to_midp_map[SDLK_ASTERISK] = KEY_STAR;
    manager->sdl_to_midp_map[SDLK_HASH] = KEY_POUND;
    
    // 方向键映射
    manager->sdl_to_midp_map[SDLK_UP] = KEY_UP;
    manager->sdl_to_midp_map[SDLK_DOWN] = KEY_DOWN;
    manager->sdl_to_midp_map[SDLK_LEFT] = KEY_LEFT;
    manager->sdl_to_midp_map[SDLK_RIGHT] = KEY_RIGHT;
    
    // 游戏键映射
    manager->sdl_to_midp_map[SDLK_SPACE] = KEY_FIRE;
    manager->sdl_to_midp_map[SDLK_RETURN] = KEY_FIRE;
    manager->sdl_to_midp_map[SDLK_z] = KEY_GAME_A;
    manager->sdl_to_midp_map[SDLK_x] = KEY_GAME_B;
    manager->sdl_to_midp_map[SDLK_c] = KEY_GAME_C;
    manager->sdl_to_midp_map[SDLK_v] = KEY_GAME_D;
    
    // 软键映射
    manager->sdl_to_midp_map[SDLK_F1] = KEY_SOFT_LEFT;
    manager->sdl_to_midp_map[SDLK_F2] = KEY_SOFT_RIGHT;
    
    // 其他键映射
    manager->sdl_to_midp_map[SDLK_ESCAPE] = KEY_END;
    manager->sdl_to_midp_map[SDLK_BACKSPACE] = KEY_CLEAR;
    manager->sdl_to_midp_map[SDLK_TAB] = KEY_SELECT;
}

j2me_input_manager_t* j2me_input_manager_create(void) {
    j2me_input_manager_t* manager = (j2me_input_manager_t*)malloc(sizeof(j2me_input_manager_t));
    if (!manager) {
        return NULL;
    }
    
    memset(manager, 0, sizeof(j2me_input_manager_t));
    
    // 初始化键码映射
    init_key_mapping(manager);
    
    printf("[输入系统] 输入管理器创建成功\n");
    return manager;
}

void j2me_input_manager_destroy(j2me_input_manager_t* manager) {
    if (manager) {
        free(manager);
        printf("[输入系统] 输入管理器已销毁\n");
    }
}

void j2me_input_set_key_callback(j2me_input_manager_t* manager, j2me_key_callback_t callback, void* user_data) {
    if (manager) {
        manager->key_callback = callback;
        manager->callback_user_data = user_data;
    }
}

void j2me_input_set_pointer_callback(j2me_input_manager_t* manager, j2me_pointer_callback_t callback, void* user_data) {
    if (manager) {
        manager->pointer_callback = callback;
        manager->callback_user_data = user_data;
    }
}

/**
 * @brief 获取SDL键码对应的MIDP键码
 * @param manager 输入管理器
 * @param sdl_key SDL键码
 * @return MIDP键码
 */
static int get_midp_key_code(j2me_input_manager_t* manager, SDL_Keycode sdl_key) {
    if (sdl_key >= 0 && sdl_key < 512) {
        return manager->sdl_to_midp_map[sdl_key];
    }
    return 0;
}

/**
 * @brief 检查是否为游戏键
 * @param key_code MIDP键码
 * @return 是否为游戏键
 */
static bool is_game_key(int key_code) {
    return (key_code >= KEY_GAME_D && key_code <= KEY_UP) || key_code == KEY_FIRE;
}

/**
 * @brief 触发键盘事件
 * @param manager 输入管理器
 * @param type 事件类型
 * @param key_code MIDP键码
 * @param key_char 字符
 */
static void trigger_key_event(j2me_input_manager_t* manager, j2me_input_event_type_t type, int key_code, char key_char) {
    if (!manager->key_callback || key_code == 0) {
        return;
    }
    
    j2me_key_event_t event;
    event.type = type;
    event.key_code = key_code;
    event.key_char = key_char;
    event.timestamp = SDL_GetTicks();
    event.is_game_key = is_game_key(key_code);
    
    manager->key_callback(&event, manager->callback_user_data);
}

/**
 * @brief 触发指针事件
 * @param manager 输入管理器
 * @param type 事件类型
 * @param x X坐标
 * @param y Y坐标
 */
static void trigger_pointer_event(j2me_input_manager_t* manager, j2me_input_event_type_t type, int x, int y) {
    if (!manager->pointer_callback) {
        return;
    }
    
    j2me_pointer_event_t event;
    event.type = type;
    event.x = x;
    event.y = y;
    event.timestamp = SDL_GetTicks();
    
    manager->pointer_callback(&event, manager->callback_user_data);
}

bool j2me_input_handle_sdl_event(j2me_input_manager_t* manager, SDL_Event* event) {
    if (!manager || !event) {
        return false;
    }
    
    switch (event->type) {
        case SDL_KEYDOWN: {
            int midp_key = get_midp_key_code(manager, event->key.keysym.sym);
            if (midp_key != 0) {
                // 更新键状态
                if (midp_key > 0 && midp_key < 512) {
                    manager->key_states[midp_key] = true;
                } else if (is_game_key(midp_key)) {
                    int game_index = -midp_key - 1;
                    if (game_index >= 0 && game_index < 10) {
                        manager->game_key_states[game_index] = true;
                    }
                }
                
                // 触发事件
                char key_char = (event->key.keysym.sym >= 32 && event->key.keysym.sym < 127) ? 
                               (char)event->key.keysym.sym : 0;
                trigger_key_event(manager, INPUT_EVENT_KEY_PRESSED, midp_key, key_char);
                
                printf("[输入系统] 键按下: SDL=%d, MIDP=%d, 字符='%c'\n", 
                       event->key.keysym.sym, midp_key, key_char ? key_char : '?');
                return true;
            }
            break;
        }
        
        case SDL_KEYUP: {
            int midp_key = get_midp_key_code(manager, event->key.keysym.sym);
            if (midp_key != 0) {
                // 更新键状态
                if (midp_key > 0 && midp_key < 512) {
                    manager->key_states[midp_key] = false;
                } else if (is_game_key(midp_key)) {
                    int game_index = -midp_key - 1;
                    if (game_index >= 0 && game_index < 10) {
                        manager->game_key_states[game_index] = false;
                    }
                }
                
                // 触发事件
                trigger_key_event(manager, INPUT_EVENT_KEY_RELEASED, midp_key, 0);
                
                printf("[输入系统] 键释放: SDL=%d, MIDP=%d\n", 
                       event->key.keysym.sym, midp_key);
                return true;
            }
            break;
        }
        
        case SDL_MOUSEBUTTONDOWN: {
            if (event->button.button == SDL_BUTTON_LEFT) {
                manager->pointer_x = event->button.x;
                manager->pointer_y = event->button.y;
                manager->pointer_pressed = true;
                
                trigger_pointer_event(manager, INPUT_EVENT_POINTER_PRESSED, 
                                     event->button.x, event->button.y);
                
                printf("[输入系统] 指针按下: (%d,%d)\n", event->button.x, event->button.y);
                return true;
            }
            break;
        }
        
        case SDL_MOUSEBUTTONUP: {
            if (event->button.button == SDL_BUTTON_LEFT) {
                manager->pointer_x = event->button.x;
                manager->pointer_y = event->button.y;
                manager->pointer_pressed = false;
                
                trigger_pointer_event(manager, INPUT_EVENT_POINTER_RELEASED, 
                                     event->button.x, event->button.y);
                
                printf("[输入系统] 指针释放: (%d,%d)\n", event->button.x, event->button.y);
                return true;
            }
            break;
        }
        
        case SDL_MOUSEMOTION: {
            manager->pointer_x = event->motion.x;
            manager->pointer_y = event->motion.y;
            
            if (manager->pointer_pressed) {
                trigger_pointer_event(manager, INPUT_EVENT_POINTER_DRAGGED, 
                                     event->motion.x, event->motion.y);
            }
            return true;
        }
    }
    
    return false;
}

bool j2me_input_is_key_pressed(j2me_input_manager_t* manager, int key_code) {
    if (!manager) {
        return false;
    }
    
    if (key_code > 0 && key_code < 512) {
        return manager->key_states[key_code];
    } else if (is_game_key(key_code)) {
        int game_index = -key_code - 1;
        if (game_index >= 0 && game_index < 10) {
            return manager->game_key_states[game_index];
        }
    }
    
    return false;
}

bool j2me_input_is_game_action_pressed(j2me_input_manager_t* manager, int game_action) {
    if (!manager) {
        return false;
    }
    
    switch (game_action) {
        case KEY_UP: return manager->game_key_states[0];
        case KEY_DOWN: return manager->game_key_states[1];
        case KEY_LEFT: return manager->game_key_states[2];
        case KEY_RIGHT: return manager->game_key_states[3];
        case KEY_FIRE: return manager->game_key_states[4];
        case KEY_GAME_A: return manager->game_key_states[5];
        case KEY_GAME_B: return manager->game_key_states[6];
        case KEY_GAME_C: return manager->game_key_states[7];
        case KEY_GAME_D: return manager->game_key_states[8];
        default: return false;
    }
}

int j2me_input_get_game_action(int key_code) {
    switch (key_code) {
        case KEY_UP: return KEY_UP;
        case KEY_DOWN: return KEY_DOWN;
        case KEY_LEFT: return KEY_LEFT;
        case KEY_RIGHT: return KEY_RIGHT;
        case KEY_FIRE: return KEY_FIRE;
        case KEY_GAME_A: return KEY_GAME_A;
        case KEY_GAME_B: return KEY_GAME_B;
        case KEY_GAME_C: return KEY_GAME_C;
        case KEY_GAME_D: return KEY_GAME_D;
        case KEY_NUM2: return KEY_UP;    // 数字键2通常映射为上
        case KEY_NUM8: return KEY_DOWN;  // 数字键8通常映射为下
        case KEY_NUM4: return KEY_LEFT;  // 数字键4通常映射为左
        case KEY_NUM6: return KEY_RIGHT; // 数字键6通常映射为右
        case KEY_NUM5: return KEY_FIRE;  // 数字键5通常映射为确认
        default: return 0;
    }
}

int j2me_input_get_key_code(int game_action) {
    switch (game_action) {
        case KEY_UP: return KEY_UP;
        case KEY_DOWN: return KEY_DOWN;
        case KEY_LEFT: return KEY_LEFT;
        case KEY_RIGHT: return KEY_RIGHT;
        case KEY_FIRE: return KEY_FIRE;
        case KEY_GAME_A: return KEY_GAME_A;
        case KEY_GAME_B: return KEY_GAME_B;
        case KEY_GAME_C: return KEY_GAME_C;
        case KEY_GAME_D: return KEY_GAME_D;
        default: return 0;
    }
}

const char* j2me_input_get_key_name(int key_code) {
    switch (key_code) {
        case KEY_NUM0: return "0";
        case KEY_NUM1: return "1";
        case KEY_NUM2: return "2";
        case KEY_NUM3: return "3";
        case KEY_NUM4: return "4";
        case KEY_NUM5: return "5";
        case KEY_NUM6: return "6";
        case KEY_NUM7: return "7";
        case KEY_NUM8: return "8";
        case KEY_NUM9: return "9";
        case KEY_STAR: return "*";
        case KEY_POUND: return "#";
        case KEY_UP: return "UP";
        case KEY_DOWN: return "DOWN";
        case KEY_LEFT: return "LEFT";
        case KEY_RIGHT: return "RIGHT";
        case KEY_FIRE: return "FIRE";
        case KEY_GAME_A: return "GAME_A";
        case KEY_GAME_B: return "GAME_B";
        case KEY_GAME_C: return "GAME_C";
        case KEY_GAME_D: return "GAME_D";
        case KEY_SOFT_LEFT: return "SOFT_LEFT";
        case KEY_SOFT_RIGHT: return "SOFT_RIGHT";
        case KEY_SELECT: return "SELECT";
        case KEY_CLEAR: return "CLEAR";
        case KEY_END: return "END";
        default: return "UNKNOWN";
    }
}

void j2me_input_get_pointer_position(j2me_input_manager_t* manager, int* x, int* y) {
    if (manager && x && y) {
        *x = manager->pointer_x;
        *y = manager->pointer_y;
    }
}

bool j2me_input_is_pointer_pressed(j2me_input_manager_t* manager) {
    return manager ? manager->pointer_pressed : false;
}

void j2me_input_update(j2me_input_manager_t* manager) {
    // 这里可以添加每帧更新的逻辑
    // 例如处理按键重复、手势识别等
    if (!manager) {
        return;
    }
    
    // 目前不需要特殊处理
}

void j2me_input_reset(j2me_input_manager_t* manager) {
    if (!manager) {
        return;
    }
    
    // 重置所有键状态
    memset(manager->key_states, 0, sizeof(manager->key_states));
    memset(manager->game_key_states, 0, sizeof(manager->game_key_states));
    
    // 重置指针状态
    manager->pointer_x = 0;
    manager->pointer_y = 0;
    manager->pointer_pressed = false;
    
    printf("[输入系统] 输入状态已重置\n");
}

int j2me_input_get_key_states(j2me_input_manager_t* manager) {
    if (!manager) {
        return 0;
    }
    
    int states = 0;
    
    if (manager->game_key_states[0]) states |= GAME_UP_PRESSED;     // UP
    if (manager->game_key_states[1]) states |= GAME_DOWN_PRESSED;   // DOWN
    if (manager->game_key_states[2]) states |= GAME_LEFT_PRESSED;   // LEFT
    if (manager->game_key_states[3]) states |= GAME_RIGHT_PRESSED;  // RIGHT
    if (manager->game_key_states[4]) states |= GAME_FIRE_PRESSED;   // FIRE
    if (manager->game_key_states[5]) states |= GAME_A_PRESSED;      // GAME_A
    if (manager->game_key_states[6]) states |= GAME_B_PRESSED;      // GAME_B
    if (manager->game_key_states[7]) states |= GAME_C_PRESSED;      // GAME_C
    if (manager->game_key_states[8]) states |= GAME_D_PRESSED;      // GAME_D
    
    return states;
}