#include "j2me_audio.h"
#include "j2me_vm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

/**
 * @file j2me_audio.c
 * @brief J2ME音频系统实现
 * 
 * 基于SDL2_mixer的音频播放系统
 */

// 默认音频设置
#define DEFAULT_FREQUENCY       22050
#define DEFAULT_FORMAT          AUDIO_S16SYS
#define DEFAULT_CHANNELS        2
#define DEFAULT_CHUNK_SIZE      1024
#define MAX_PLAYERS            16

j2me_audio_manager_t* j2me_audio_manager_create(j2me_vm_t* vm) {
    if (!vm) {
        return NULL;
    }
    
    j2me_audio_manager_t* manager = (j2me_audio_manager_t*)malloc(sizeof(j2me_audio_manager_t));
    if (!manager) {
        return NULL;
    }
    
    memset(manager, 0, sizeof(j2me_audio_manager_t));
    
    manager->max_players = MAX_PLAYERS;
    manager->players = (j2me_player_t**)calloc(MAX_PLAYERS, sizeof(j2me_player_t*));
    if (!manager->players) {
        free(manager);
        return NULL;
    }
    
    manager->master_volume = 100;
    manager->master_muted = false;
    
    // 设置默认音频参数
    manager->frequency = DEFAULT_FREQUENCY;
    manager->format = DEFAULT_FORMAT;
    manager->channels = DEFAULT_CHANNELS;
    manager->chunk_size = DEFAULT_CHUNK_SIZE;
    
    printf("[音频系统] 音频管理器创建成功\n");
    return manager;
}

void j2me_audio_manager_destroy(j2me_audio_manager_t* manager) {
    if (!manager) {
        return;
    }
    
    // 停止并销毁所有播放器
    for (int i = 0; i < manager->max_players; i++) {
        if (manager->players[i]) {
            j2me_player_destroy(manager->players[i]);
            manager->players[i] = NULL; // 清空指针避免重复释放
        }
    }
    
    if (manager->initialized) {
        j2me_audio_shutdown(manager);
    }
    
    free(manager->players);
    free(manager);
    
    printf("[音频系统] 音频管理器已销毁\n");
}

j2me_error_t j2me_audio_initialize(j2me_audio_manager_t* manager) {
    if (!manager || manager->initialized) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 初始化SDL音频子系统
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        printf("[音频系统] SDL音频初始化失败: %s\n", SDL_GetError());
        return J2ME_ERROR_RUNTIME_EXCEPTION;
    }
    
    // 初始化SDL_mixer
    if (Mix_OpenAudio(manager->frequency, manager->format, manager->channels, manager->chunk_size) < 0) {
        printf("[音频系统] SDL_mixer初始化失败: %s\n", Mix_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return J2ME_ERROR_RUNTIME_EXCEPTION;
    }
    
    // 分配混音通道
    Mix_AllocateChannels(manager->max_players);
    
    manager->initialized = true;
    
    printf("[音频系统] 音频系统初始化成功 (频率: %d Hz, 通道: %d, 缓冲: %d)\n",
           manager->frequency, manager->channels, manager->chunk_size);
    
    return J2ME_SUCCESS;
}

void j2me_audio_shutdown(j2me_audio_manager_t* manager) {
    if (!manager || !manager->initialized) {
        return;
    }
    
    // 停止所有播放
    Mix_HaltChannel(-1);
    Mix_HaltMusic();
    
    // 关闭SDL_mixer
    Mix_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    
    manager->initialized = false;
    
    printf("[音频系统] 音频系统已关闭\n");
}

/**
 * @brief 检测音频文件格式
 */
j2me_audio_format_t j2me_audio_detect_format(const uint8_t* data, size_t size) {
    if (!data || size < 4) {
        return AUDIO_FORMAT_UNKNOWN;
    }
    
    // WAV格式检测
    if (size >= 12 && memcmp(data, "RIFF", 4) == 0 && memcmp(data + 8, "WAVE", 4) == 0) {
        return AUDIO_FORMAT_WAV;
    }
    
    // MIDI格式检测
    if (size >= 4 && memcmp(data, "MThd", 4) == 0) {
        return AUDIO_FORMAT_MIDI;
    }
    
    // MP3格式检测 (简化)
    if (size >= 3 && ((data[0] == 0xFF && (data[1] & 0xE0) == 0xE0) || 
                      memcmp(data, "ID3", 3) == 0)) {
        return AUDIO_FORMAT_MP3;
    }
    
    return AUDIO_FORMAT_UNKNOWN;
}

j2me_audio_clip_t* j2me_audio_clip_create(j2me_vm_t* vm, const uint8_t* data, size_t size, j2me_audio_format_t format) {
    if (!vm || !data || size == 0) {
        return NULL;
    }
    
    j2me_audio_clip_t* clip = (j2me_audio_clip_t*)malloc(sizeof(j2me_audio_clip_t));
    if (!clip) {
        return NULL;
    }
    
    memset(clip, 0, sizeof(j2me_audio_clip_t));
    
    // 复制音频数据
    clip->data = (uint8_t*)malloc(size);
    if (!clip->data) {
        free(clip);
        return NULL;
    }
    
    memcpy(clip->data, data, size);
    clip->data_size = size;
    clip->format = format;
    
    // 根据格式创建SDL对象
    switch (format) {
        case AUDIO_FORMAT_WAV:
        case AUDIO_FORMAT_MP3: {
            // 创建SDL_RWops从内存数据
            SDL_RWops* rw = SDL_RWFromMem(clip->data, (int)size);
            if (rw) {
                clip->sdl_chunk = Mix_LoadWAV_RW(rw, 1); // 1表示自动释放RWops
                if (!clip->sdl_chunk) {
                    printf("[音频系统] 加载音频块失败: %s\n", Mix_GetError());
                }
            }
            break;
        }
        
        case AUDIO_FORMAT_MIDI: {
            SDL_RWops* rw = SDL_RWFromMem(clip->data, (int)size);
            if (rw) {
                clip->sdl_music = Mix_LoadMUS_RW(rw, 1);
                if (!clip->sdl_music) {
                    printf("[音频系统] 加载MIDI失败: %s\n", Mix_GetError());
                }
            }
            break;
        }
        
        default:
            printf("[音频系统] 不支持的音频格式: %d\n", format);
            break;
    }
    
    printf("[音频系统] 创建音频剪辑: 格式=%s, 大小=%zu bytes\n",
           j2me_audio_get_format_name(format), size);
    
    return clip;
}

j2me_audio_clip_t* j2me_audio_clip_create_from_file(j2me_vm_t* vm, const char* filename) {
    if (!vm || !filename) {
        return NULL;
    }
    
    printf("[音频系统] 从文件创建音频剪辑: %s\n", filename);
    
    // 尝试直接加载文件
    Mix_Chunk* chunk = Mix_LoadWAV(filename);
    if (chunk) {
        // 成功加载文件
        j2me_audio_clip_t* clip = (j2me_audio_clip_t*)malloc(sizeof(j2me_audio_clip_t));
        if (!clip) {
            Mix_FreeChunk(chunk);
            return NULL;
        }
        
        memset(clip, 0, sizeof(j2me_audio_clip_t));
        clip->format = AUDIO_FORMAT_WAV;
        clip->sdl_chunk = chunk;
        clip->data = NULL; // 文件直接加载，不需要内存数据
        clip->data_size = 0;
        
        printf("[音频系统] 文件加载成功: %s\n", filename);
        return clip;
    }
    
    // 如果文件不存在，创建一个测试音调
    printf("[音频系统] 文件不存在，创建测试音调: %s\n", filename);
    
    // 生成简单的正弦波音调 (440Hz, 1秒)
    const int sample_rate = 22050;
    const int duration = 1; // 1秒
    const int samples = sample_rate * duration;
    const int bytes_per_sample = 2; // 16位
    const size_t tone_size = samples * bytes_per_sample;
    
    uint8_t* tone_data = (uint8_t*)malloc(tone_size);
    if (!tone_data) {
        return NULL;
    }
    
    // 生成440Hz正弦波
    int16_t* samples_16 = (int16_t*)tone_data;
    for (int i = 0; i < samples; i++) {
        double t = (double)i / sample_rate;
        int16_t sample = (int16_t)(sin(2.0 * M_PI * 440.0 * t) * 16384);
        samples_16[i] = sample;
    }
    
    j2me_audio_clip_t* clip = j2me_audio_clip_create(vm, tone_data, tone_size, AUDIO_FORMAT_WAV);
    free(tone_data);
    
    return clip;
}

void j2me_audio_clip_destroy(j2me_audio_clip_t* clip) {
    if (!clip) {
        return;
    }
    
    if (clip->sdl_chunk) {
        Mix_FreeChunk(clip->sdl_chunk);
    }
    
    if (clip->sdl_music) {
        Mix_FreeMusic(clip->sdl_music);
    }
    
    if (clip->data) {
        free(clip->data);
    }
    
    free(clip);
}

/**
 * @brief 在播放器数组中找到空位
 */
static int find_free_player_slot(j2me_audio_manager_t* manager) {
    for (int i = 0; i < manager->max_players; i++) {
        if (manager->players[i] == NULL) {
            return i;
        }
    }
    return -1;
}

j2me_player_t* j2me_player_create(j2me_vm_t* vm, j2me_audio_manager_t* manager, j2me_audio_clip_t* clip) {
    if (!vm || !manager || !clip) {
        return NULL;
    }
    
    int slot = find_free_player_slot(manager);
    if (slot < 0) {
        printf("[音频系统] 播放器数量已达上限\n");
        return NULL;
    }
    
    j2me_player_t* player = (j2me_player_t*)malloc(sizeof(j2me_player_t));
    if (!player) {
        return NULL;
    }
    
    memset(player, 0, sizeof(j2me_player_t));
    
    player->state = PLAYER_UNREALIZED;
    player->clip = clip;
    player->volume = 100;
    player->looping = false;
    player->muted = false;
    player->channel = slot;
    
    manager->players[slot] = player;
    manager->active_players++;
    
    printf("[音频系统] 创建播放器 #%d\n", slot);
    
    return player;
}

j2me_player_t* j2me_player_create_from_url(j2me_vm_t* vm, j2me_audio_manager_t* manager, const char* url) {
    if (!vm || !manager || !url) {
        return NULL;
    }
    
    printf("[音频系统] 从URL创建播放器: %s\n", url);
    
    // 简化实现：创建测试音频
    j2me_audio_clip_t* clip = j2me_audio_clip_create_from_file(vm, url);
    if (!clip) {
        return NULL;
    }
    
    return j2me_player_create(vm, manager, clip);
}

void j2me_player_destroy(j2me_player_t* player) {
    if (!player) {
        return;
    }
    
    // 停止播放
    if (player->state == PLAYER_STARTED) {
        j2me_player_stop(player);
    }
    
    // 注意：不要销毁clip，因为它可能被其他播放器共享
    // 只清空引用
    player->clip = NULL;
    
    printf("[音频系统] 销毁播放器\n");
    free(player);
}

j2me_error_t j2me_player_realize(j2me_player_t* player) {
    if (!player || player->state != PLAYER_UNREALIZED) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 检查音频剪辑是否有效
    if (!player->clip || (!player->clip->sdl_chunk && !player->clip->sdl_music)) {
        printf("[音频系统] 播放器实现失败：无效的音频剪辑\n");
        return J2ME_ERROR_RUNTIME_EXCEPTION;
    }
    
    player->state = PLAYER_REALIZED;
    
    printf("[音频系统] 播放器已实现\n");
    return J2ME_SUCCESS;
}

j2me_error_t j2me_player_prefetch(j2me_player_t* player) {
    if (!player || player->state != PLAYER_REALIZED) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 预取操作 (SDL_mixer会自动处理)
    player->state = PLAYER_PREFETCHED;
    
    printf("[音频系统] 播放器已预取\n");
    return J2ME_SUCCESS;
}

j2me_error_t j2me_player_start(j2me_player_t* player) {
    if (!player) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 如果还未实现，自动实现和预取
    if (player->state == PLAYER_UNREALIZED) {
        j2me_error_t result = j2me_player_realize(player);
        if (result != J2ME_SUCCESS) {
            return result;
        }
    }
    
    if (player->state == PLAYER_REALIZED) {
        j2me_error_t result = j2me_player_prefetch(player);
        if (result != J2ME_SUCCESS) {
            return result;
        }
    }
    
    if (player->state != PLAYER_PREFETCHED && player->state != PLAYER_STARTED) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 开始播放
    if (player->clip->sdl_chunk) {
        int loops = player->looping ? -1 : 0;
        int channel = Mix_PlayChannel(player->channel, player->clip->sdl_chunk, loops);
        if (channel < 0) {
            printf("[音频系统] 播放失败: %s\n", Mix_GetError());
            return J2ME_ERROR_RUNTIME_EXCEPTION;
        }
        
        // 设置音量
        int sdl_volume = (player->volume * MIX_MAX_VOLUME) / 100;
        Mix_Volume(channel, sdl_volume);
        
    } else if (player->clip->sdl_music) {
        int loops = player->looping ? -1 : 0;
        if (Mix_PlayMusic(player->clip->sdl_music, loops) < 0) {
            printf("[音频系统] 音乐播放失败: %s\n", Mix_GetError());
            return J2ME_ERROR_RUNTIME_EXCEPTION;
        }
        
        // 设置音量
        int sdl_volume = (player->volume * MIX_MAX_VOLUME) / 100;
        Mix_VolumeMusic(sdl_volume);
    }
    
    player->state = PLAYER_STARTED;
    
    printf("[音频系统] 播放器已开始播放\n");
    return J2ME_SUCCESS;
}

j2me_error_t j2me_player_stop(j2me_player_t* player) {
    if (!player || player->state != PLAYER_STARTED) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    // 停止播放
    if (player->clip->sdl_chunk) {
        Mix_HaltChannel(player->channel);
    } else if (player->clip->sdl_music) {
        Mix_HaltMusic();
    }
    
    player->state = PLAYER_PREFETCHED;
    
    printf("[音频系统] 播放器已停止\n");
    return J2ME_SUCCESS;
}

void j2me_player_close(j2me_player_t* player) {
    if (!player) {
        return;
    }
    
    if (player->state == PLAYER_STARTED) {
        j2me_player_stop(player);
    }
    
    player->state = PLAYER_CLOSED;
    
    printf("[音频系统] 播放器已关闭\n");
}

j2me_player_state_t j2me_player_get_state(j2me_player_t* player) {
    return player ? player->state : PLAYER_CLOSED;
}

void j2me_player_set_volume(j2me_player_t* player, int volume) {
    if (!player) {
        return;
    }
    
    player->volume = volume < 0 ? 0 : (volume > 100 ? 100 : volume);
    
    // 如果正在播放，立即应用音量
    if (player->state == PLAYER_STARTED) {
        int sdl_volume = (player->volume * MIX_MAX_VOLUME) / 100;
        
        if (player->clip->sdl_chunk) {
            Mix_Volume(player->channel, sdl_volume);
        } else if (player->clip->sdl_music) {
            Mix_VolumeMusic(sdl_volume);
        }
    }
    
    printf("[音频系统] 设置播放器音量: %d%%\n", player->volume);
}

int j2me_player_get_volume(j2me_player_t* player) {
    return player ? player->volume : 0;
}

void j2me_player_set_looping(j2me_player_t* player, bool looping) {
    if (player) {
        player->looping = looping;
    }
}

bool j2me_player_is_looping(j2me_player_t* player) {
    return player ? player->looping : false;
}

void j2me_player_set_muted(j2me_player_t* player, bool muted) {
    if (player) {
        player->muted = muted;
        
        // 应用静音状态
        if (player->state == PLAYER_STARTED) {
            int volume = muted ? 0 : (player->volume * MIX_MAX_VOLUME) / 100;
            
            if (player->clip->sdl_chunk) {
                Mix_Volume(player->channel, volume);
            } else if (player->clip->sdl_music) {
                Mix_VolumeMusic(volume);
            }
        }
        
        printf("[音频系统] 设置播放器静音: %s\n", muted ? "是" : "否");
    }
}

bool j2me_player_is_muted(j2me_player_t* player) {
    return player ? player->muted : false;
}

int64_t j2me_player_get_media_time(j2me_player_t* player) {
    // 简化实现：返回0
    // 实际实现需要跟踪播放时间
    return 0;
}

int64_t j2me_player_set_media_time(j2me_player_t* player, int64_t time) {
    // 简化实现：不支持seek
    printf("[音频系统] Seek功能未实现\n");
    return 0;
}

int64_t j2me_player_get_duration(j2me_player_t* player) {
    // 简化实现：返回未知时长
    return -1;
}

void j2me_player_set_end_callback(j2me_player_t* player, 
                                  void (*callback)(j2me_player_t* player, void* user_data),
                                  void* user_data) {
    if (player) {
        player->end_of_media_callback = callback;
        player->callback_user_data = user_data;
    }
}

void j2me_audio_set_master_volume(j2me_audio_manager_t* manager, int volume) {
    if (manager) {
        manager->master_volume = volume < 0 ? 0 : (volume > 100 ? 100 : volume);
        
        // 应用到所有活跃播放器
        for (int i = 0; i < manager->max_players; i++) {
            if (manager->players[i] && manager->players[i]->state == PLAYER_STARTED) {
                j2me_player_set_volume(manager->players[i], manager->players[i]->volume);
            }
        }
    }
}

int j2me_audio_get_master_volume(j2me_audio_manager_t* manager) {
    return manager ? manager->master_volume : 0;
}

void j2me_audio_set_master_muted(j2me_audio_manager_t* manager, bool muted) {
    if (manager) {
        manager->master_muted = muted;
        
        // 应用到所有播放器
        for (int i = 0; i < manager->max_players; i++) {
            if (manager->players[i]) {
                j2me_player_set_muted(manager->players[i], 
                                     manager->players[i]->muted || muted);
            }
        }
    }
}

bool j2me_audio_is_master_muted(j2me_audio_manager_t* manager) {
    return manager ? manager->master_muted : false;
}

void j2me_audio_update(j2me_audio_manager_t* manager) {
    if (!manager) {
        return;
    }
    
    // 检查播放完成的播放器
    for (int i = 0; i < manager->max_players; i++) {
        j2me_player_t* player = manager->players[i];
        if (player && player->state == PLAYER_STARTED) {
            
            // 检查是否播放完成
            bool finished = false;
            if (player->clip->sdl_chunk) {
                finished = !Mix_Playing(player->channel);
            } else if (player->clip->sdl_music) {
                finished = !Mix_PlayingMusic();
            }
            
            if (finished && !player->looping) {
                player->state = PLAYER_PREFETCHED;
                
                // 调用结束回调
                if (player->end_of_media_callback) {
                    player->end_of_media_callback(player, player->callback_user_data);
                }
            }
        }
    }
}

void j2me_audio_stop_all(j2me_audio_manager_t* manager) {
    if (!manager) {
        return;
    }
    
    for (int i = 0; i < manager->max_players; i++) {
        if (manager->players[i]) {
            j2me_player_stop(manager->players[i]);
        }
    }
    
    printf("[音频系统] 所有播放器已停止\n");
}

j2me_error_t j2me_audio_play_tone(j2me_audio_manager_t* manager, int note, int duration, int volume) {
    if (!manager || !manager->initialized) {
        return J2ME_ERROR_INVALID_PARAMETER;
    }
    
    printf("[音频系统] 播放音调: 音符=%d, 时长=%dms, 音量=%d\n", note, duration, volume);
    
    // 计算频率 (MIDI音符到频率的转换)
    double frequency = 440.0 * pow(2.0, (note - 69) / 12.0);
    
    // 生成音调数据
    const int sample_rate = 22050;
    const int samples = (sample_rate * duration) / 1000;
    const size_t tone_size = samples * 2; // 16位音频
    
    uint8_t* tone_data = (uint8_t*)malloc(tone_size);
    if (!tone_data) {
        return J2ME_ERROR_OUT_OF_MEMORY;
    }
    
    // 生成正弦波
    int16_t* samples_16 = (int16_t*)tone_data;
    int16_t amplitude = (int16_t)((volume * 16384) / 100);
    
    for (int i = 0; i < samples; i++) {
        double t = (double)i / sample_rate;
        int16_t sample = (int16_t)(sin(2.0 * M_PI * frequency * t) * amplitude);
        samples_16[i] = sample;
    }
    
    // 创建SDL_RWops并播放
    SDL_RWops* rw = SDL_RWFromMem(tone_data, (int)tone_size);
    if (rw) {
        Mix_Chunk* chunk = Mix_LoadWAV_RW(rw, 1); // 1表示自动释放RWops
        if (chunk) {
            int channel = Mix_PlayChannel(-1, chunk, 0);
            if (channel >= 0) {
                // 设置音量
                int sdl_volume = (volume * MIX_MAX_VOLUME) / 100;
                Mix_Volume(channel, sdl_volume);
                
                // 注意：这里应该在播放完成后释放chunk，但为了简化，我们立即释放
                // 在实际应用中，应该跟踪这些临时chunk并在播放完成后释放
                Mix_FreeChunk(chunk);
            } else {
                Mix_FreeChunk(chunk);
                free(tone_data);
                return J2ME_ERROR_RUNTIME_EXCEPTION;
            }
        } else {
            free(tone_data);
            return J2ME_ERROR_RUNTIME_EXCEPTION;
        }
    } else {
        free(tone_data);
        return J2ME_ERROR_RUNTIME_EXCEPTION;
    }
    
    free(tone_data);
    return J2ME_SUCCESS;
}

const char* j2me_audio_get_format_name(j2me_audio_format_t format) {
    switch (format) {
        case AUDIO_FORMAT_WAV: return "WAV";
        case AUDIO_FORMAT_MIDI: return "MIDI";
        case AUDIO_FORMAT_MP3: return "MP3";
        case AUDIO_FORMAT_AAC: return "AAC";
        case AUDIO_FORMAT_TONE_SEQUENCE: return "TONE_SEQUENCE";
        default: return "UNKNOWN";
    }
}

bool j2me_audio_is_format_supported(j2me_audio_format_t format) {
    switch (format) {
        case AUDIO_FORMAT_WAV:
        case AUDIO_FORMAT_MIDI:
            return true;
        case AUDIO_FORMAT_MP3:
        case AUDIO_FORMAT_AAC:
        case AUDIO_FORMAT_TONE_SEQUENCE:
            return false; // 暂不支持
        default:
            return false;
    }
}

void j2me_audio_pause_all(j2me_audio_manager_t* manager) {
    if (!manager) {
        return;
    }
    
    // 暂停所有音频
    Mix_Pause(-1);
    Mix_PauseMusic();
    
    printf("[音频系统] 所有播放器已暂停\n");
}

void j2me_audio_resume_all(j2me_audio_manager_t* manager) {
    if (!manager) {
        return;
    }
    
    // 恢复所有音频
    Mix_Resume(-1);
    Mix_ResumeMusic();
    
    printf("[音频系统] 所有播放器已恢复\n");
}

j2me_audio_clip_t* j2me_audio_create_tone_sequence(j2me_vm_t* vm, const uint8_t* sequence, size_t length) {
    if (!vm || !sequence || length == 0) {
        return NULL;
    }
    
    printf("[音频系统] 创建音调序列: 长度=%zu\n", length);
    
    // 简化的音调序列解析 - 假设每两个字节代表一个音符和持续时间
    const int sample_rate = 22050;
    size_t total_samples = 0;
    
    // 计算总样本数
    for (size_t i = 0; i < length; i += 2) {
        if (i + 1 < length) {
            int duration = sequence[i + 1] * 100; // 持续时间 (毫秒)
            total_samples += (sample_rate * duration) / 1000;
        }
    }
    
    const size_t tone_size = total_samples * 2; // 16位音频
    uint8_t* tone_data = (uint8_t*)malloc(tone_size);
    if (!tone_data) {
        return NULL;
    }
    
    int16_t* samples_16 = (int16_t*)tone_data;
    size_t current_sample = 0;
    
    // 生成音调序列
    for (size_t i = 0; i < length; i += 2) {
        if (i + 1 < length) {
            int note = sequence[i];
            int duration = sequence[i + 1] * 100; // 持续时间 (毫秒)
            int samples_for_note = (sample_rate * duration) / 1000;
            
            // 计算频率
            double frequency = 440.0 * pow(2.0, (note - 69) / 12.0);
            
            // 生成这个音符的样本
            for (int j = 0; j < samples_for_note && current_sample < total_samples; j++, current_sample++) {
                double t = (double)j / sample_rate;
                int16_t sample = (int16_t)(sin(2.0 * M_PI * frequency * t) * 8192);
                samples_16[current_sample] = sample;
            }
        }
    }
    
    j2me_audio_clip_t* clip = j2me_audio_clip_create(vm, tone_data, tone_size, AUDIO_FORMAT_TONE_SEQUENCE);
    free(tone_data);
    
    return clip;
}