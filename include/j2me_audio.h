#ifndef J2ME_AUDIO_H
#define J2ME_AUDIO_H

#include "j2me_types.h"
#include "j2me_object.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

/**
 * @file j2me_audio.h
 * @brief J2ME音频系统
 * 
 * 实现MMAPI (Mobile Media API) 的音频功能
 */

// 前向声明
typedef struct j2me_audio_manager j2me_audio_manager_t;
typedef struct j2me_player j2me_player_t;
typedef struct j2me_audio_clip j2me_audio_clip_t;

// 播放器状态 (javax.microedition.media.Player)
typedef enum {
    PLAYER_UNREALIZED = 100,
    PLAYER_REALIZED = 200,
    PLAYER_PREFETCHED = 300,
    PLAYER_STARTED = 400,
    PLAYER_CLOSED = 0
} j2me_player_state_t;

// 音频格式
typedef enum {
    AUDIO_FORMAT_UNKNOWN = 0,
    AUDIO_FORMAT_WAV,
    AUDIO_FORMAT_MIDI,
    AUDIO_FORMAT_MP3,
    AUDIO_FORMAT_AAC,
    AUDIO_FORMAT_TONE_SEQUENCE
} j2me_audio_format_t;

// 音频剪辑结构
struct j2me_audio_clip {
    j2me_object_header_t header;    // 对象头
    j2me_audio_format_t format;     // 音频格式
    uint8_t* data;                  // 音频数据
    size_t data_size;               // 数据大小
    int sample_rate;                // 采样率
    int channels;                   // 声道数
    int bits_per_sample;            // 位深度
    Mix_Chunk* sdl_chunk;           // SDL音频块
    Mix_Music* sdl_music;           // SDL音乐 (用于MIDI等)
};

// 播放器结构
struct j2me_player {
    j2me_object_header_t header;    // 对象头
    j2me_player_state_t state;      // 播放器状态
    j2me_audio_clip_t* clip;        // 音频剪辑
    int volume;                     // 音量 (0-100)
    bool looping;                   // 是否循环
    bool muted;                     // 是否静音
    int64_t media_time;             // 媒体时间 (微秒)
    int64_t duration;               // 总时长 (微秒)
    int channel;                    // SDL混音器通道
    
    // 回调函数
    void (*end_of_media_callback)(j2me_player_t* player, void* user_data);
    void* callback_user_data;
};

// 音频管理器
struct j2me_audio_manager {
    bool initialized;               // 是否已初始化
    int max_players;                // 最大播放器数量
    j2me_player_t** players;        // 播放器数组
    int active_players;             // 活跃播放器数量
    
    // 全局音频设置
    int master_volume;              // 主音量 (0-100)
    bool master_muted;              // 主静音
    
    // SDL Mixer设置
    int frequency;                  // 音频频率
    Uint16 format;                  // 音频格式
    int channels;                   // 声道数
    int chunk_size;                 // 缓冲区大小
};

/**
 * @brief 创建音频管理器
 * @param vm 虚拟机实例
 * @return 音频管理器指针
 */
j2me_audio_manager_t* j2me_audio_manager_create(j2me_vm_t* vm);

/**
 * @brief 销毁音频管理器
 * @param manager 音频管理器
 */
void j2me_audio_manager_destroy(j2me_audio_manager_t* manager);

/**
 * @brief 初始化音频系统
 * @param manager 音频管理器
 * @return 错误码
 */
j2me_error_t j2me_audio_initialize(j2me_audio_manager_t* manager);

/**
 * @brief 关闭音频系统
 * @param manager 音频管理器
 */
void j2me_audio_shutdown(j2me_audio_manager_t* manager);

/**
 * @brief 创建音频剪辑
 * @param vm 虚拟机实例
 * @param data 音频数据
 * @param size 数据大小
 * @param format 音频格式
 * @return 音频剪辑指针
 */
j2me_audio_clip_t* j2me_audio_clip_create(j2me_vm_t* vm, const uint8_t* data, size_t size, j2me_audio_format_t format);

/**
 * @brief 从文件创建音频剪辑
 * @param vm 虚拟机实例
 * @param filename 文件名
 * @return 音频剪辑指针
 */
j2me_audio_clip_t* j2me_audio_clip_create_from_file(j2me_vm_t* vm, const char* filename);

/**
 * @brief 销毁音频剪辑
 * @param clip 音频剪辑
 */
void j2me_audio_clip_destroy(j2me_audio_clip_t* clip);

/**
 * @brief 创建播放器
 * @param vm 虚拟机实例
 * @param manager 音频管理器
 * @param clip 音频剪辑
 * @return 播放器指针
 */
j2me_player_t* j2me_player_create(j2me_vm_t* vm, j2me_audio_manager_t* manager, j2me_audio_clip_t* clip);

/**
 * @brief 从URL创建播放器
 * @param vm 虚拟机实例
 * @param manager 音频管理器
 * @param url 音频URL
 * @return 播放器指针
 */
j2me_player_t* j2me_player_create_from_url(j2me_vm_t* vm, j2me_audio_manager_t* manager, const char* url);

/**
 * @brief 销毁播放器
 * @param player 播放器
 */
void j2me_player_destroy(j2me_player_t* player);

/**
 * @brief 实现播放器 (realize)
 * @param player 播放器
 * @return 错误码
 */
j2me_error_t j2me_player_realize(j2me_player_t* player);

/**
 * @brief 预取播放器 (prefetch)
 * @param player 播放器
 * @return 错误码
 */
j2me_error_t j2me_player_prefetch(j2me_player_t* player);

/**
 * @brief 开始播放
 * @param player 播放器
 * @return 错误码
 */
j2me_error_t j2me_player_start(j2me_player_t* player);

/**
 * @brief 停止播放
 * @param player 播放器
 * @return 错误码
 */
j2me_error_t j2me_player_stop(j2me_player_t* player);

/**
 * @brief 关闭播放器
 * @param player 播放器
 */
void j2me_player_close(j2me_player_t* player);

/**
 * @brief 获取播放器状态
 * @param player 播放器
 * @return 播放器状态
 */
j2me_player_state_t j2me_player_get_state(j2me_player_t* player);

/**
 * @brief 设置音量
 * @param player 播放器
 * @param volume 音量 (0-100)
 */
void j2me_player_set_volume(j2me_player_t* player, int volume);

/**
 * @brief 获取音量
 * @param player 播放器
 * @return 音量 (0-100)
 */
int j2me_player_get_volume(j2me_player_t* player);

/**
 * @brief 设置循环播放
 * @param player 播放器
 * @param looping 是否循环
 */
void j2me_player_set_looping(j2me_player_t* player, bool looping);

/**
 * @brief 检查是否循环播放
 * @param player 播放器
 * @return 是否循环
 */
bool j2me_player_is_looping(j2me_player_t* player);

/**
 * @brief 设置静音
 * @param player 播放器
 * @param muted 是否静音
 */
void j2me_player_set_muted(j2me_player_t* player, bool muted);

/**
 * @brief 检查是否静音
 * @param player 播放器
 * @return 是否静音
 */
bool j2me_player_is_muted(j2me_player_t* player);

/**
 * @brief 获取媒体时间
 * @param player 播放器
 * @return 媒体时间 (微秒)
 */
int64_t j2me_player_get_media_time(j2me_player_t* player);

/**
 * @brief 设置媒体时间 (seek)
 * @param player 播放器
 * @param time 目标时间 (微秒)
 * @return 实际设置的时间
 */
int64_t j2me_player_set_media_time(j2me_player_t* player, int64_t time);

/**
 * @brief 获取媒体总时长
 * @param player 播放器
 * @return 总时长 (微秒)
 */
int64_t j2me_player_get_duration(j2me_player_t* player);

/**
 * @brief 设置播放结束回调
 * @param player 播放器
 * @param callback 回调函数
 * @param user_data 用户数据
 */
void j2me_player_set_end_callback(j2me_player_t* player, 
                                  void (*callback)(j2me_player_t* player, void* user_data),
                                  void* user_data);

/**
 * @brief 设置主音量
 * @param manager 音频管理器
 * @param volume 音量 (0-100)
 */
void j2me_audio_set_master_volume(j2me_audio_manager_t* manager, int volume);

/**
 * @brief 获取主音量
 * @param manager 音频管理器
 * @return 音量 (0-100)
 */
int j2me_audio_get_master_volume(j2me_audio_manager_t* manager);

/**
 * @brief 设置主静音
 * @param manager 音频管理器
 * @param muted 是否静音
 */
void j2me_audio_set_master_muted(j2me_audio_manager_t* manager, bool muted);

/**
 * @brief 检查主静音状态
 * @param manager 音频管理器
 * @return 是否静音
 */
bool j2me_audio_is_master_muted(j2me_audio_manager_t* manager);

/**
 * @brief 更新音频系统 (每帧调用)
 * @param manager 音频管理器
 */
void j2me_audio_update(j2me_audio_manager_t* manager);

/**
 * @brief 暂停所有播放器
 * @param manager 音频管理器
 */
void j2me_audio_pause_all(j2me_audio_manager_t* manager);

/**
 * @brief 恢复所有播放器
 * @param manager 音频管理器
 */
void j2me_audio_resume_all(j2me_audio_manager_t* manager);

/**
 * @brief 停止所有播放器
 * @param manager 音频管理器
 */
void j2me_audio_stop_all(j2me_audio_manager_t* manager);

// 音调序列支持 (Tone Sequence)

/**
 * @brief 创建音调序列
 * @param vm 虚拟机实例
 * @param sequence 音调序列数据
 * @param length 序列长度
 * @return 音频剪辑指针
 */
j2me_audio_clip_t* j2me_audio_create_tone_sequence(j2me_vm_t* vm, const uint8_t* sequence, size_t length);

/**
 * @brief 播放单个音调
 * @param manager 音频管理器
 * @param note 音符 (0-127, MIDI音符)
 * @param duration 持续时间 (毫秒)
 * @param volume 音量 (0-100)
 * @return 错误码
 */
j2me_error_t j2me_audio_play_tone(j2me_audio_manager_t* manager, int note, int duration, int volume);

// 音频格式检测

/**
 * @brief 检测音频格式
 * @param data 音频数据
 * @param size 数据大小
 * @return 音频格式
 */
j2me_audio_format_t j2me_audio_detect_format(const uint8_t* data, size_t size);

/**
 * @brief 获取格式名称
 * @param format 音频格式
 * @return 格式名称字符串
 */
const char* j2me_audio_get_format_name(j2me_audio_format_t format);

/**
 * @brief 检查格式是否支持
 * @param format 音频格式
 * @return 是否支持
 */
bool j2me_audio_is_format_supported(j2me_audio_format_t format);

#endif // J2ME_AUDIO_H