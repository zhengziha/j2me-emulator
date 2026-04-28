#include <SDL2/SDL.h>
#include "j2me_log.h"
#include <stdio.h>
#include <stdbool.h>

/**
 * @file interactive_render_test.c
 * @brief 交互式渲染测试程序
 * 
 * 通过按键反馈测试SDL渲染是否正常工作
 */

void print_instructions() {
    LOG_DEBUG("\n");
    LOG_DEBUG("===========================================\n");
    LOG_DEBUG("  交互式SDL渲染测试\n");
    LOG_DEBUG("===========================================\n");
    LOG_DEBUG("\n");
    LOG_DEBUG("这个测试将显示不同的颜色和图形。\n");
    LOG_DEBUG("请根据你看到的内容按相应的键：\n");
    LOG_DEBUG("\n");
    LOG_DEBUG("测试1: 全屏红色\n");
    LOG_DEBUG("  - 如果看到红色，按 '1'\n");
    LOG_DEBUG("  - 如果看到黑色或其他颜色，按 '0'\n");
    LOG_DEBUG("  - 如果完全看不到窗口，按 'N'\n");
    LOG_DEBUG("\n");
    LOG_DEBUG("按 SPACE 开始测试...\n");
    LOG_DEBUG("按 ESC 退出\n");
    LOG_DEBUG("\n");
}

void test_1_solid_red(SDL_Renderer* renderer) {
    LOG_DEBUG("\n>>> 测试1: 全屏红色\n");
    LOG_DEBUG("    窗口应该显示纯红色背景\n");
    LOG_DEBUG("    你看到了什么？\n");
    LOG_DEBUG("    1 = 红色  |  0 = 其他颜色/黑色  |  N = 看不到窗口\n");
    LOG_DEBUG("    按键: ");
    fflush(stdout);
    
    // 绘制红色背景
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

void test_2_solid_green(SDL_Renderer* renderer) {
    LOG_DEBUG("\n>>> 测试2: 全屏绿色\n");
    LOG_DEBUG("    窗口应该显示纯绿色背景\n");
    LOG_DEBUG("    你看到了什么？\n");
    LOG_DEBUG("    1 = 绿色  |  0 = 其他颜色/黑色  |  N = 看不到窗口\n");
    LOG_DEBUG("    按键: ");
    fflush(stdout);
    
    // 绘制绿色背景
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

void test_3_solid_blue(SDL_Renderer* renderer) {
    LOG_DEBUG("\n>>> 测试3: 全屏蓝色\n");
    LOG_DEBUG("    窗口应该显示纯蓝色背景\n");
    LOG_DEBUG("    你看到了什么？\n");
    LOG_DEBUG("    1 = 蓝色  |  0 = 其他颜色/黑色  |  N = 看不到窗口\n");
    LOG_DEBUG("    按键: ");
    fflush(stdout);
    
    // 绘制蓝色背景
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

void test_4_white_rect(SDL_Renderer* renderer) {
    LOG_DEBUG("\n>>> 测试4: 黑色背景 + 白色矩形\n");
    LOG_DEBUG("    窗口应该显示黑色背景，中间有一个白色矩形\n");
    LOG_DEBUG("    你看到了什么？\n");
    LOG_DEBUG("    1 = 看到白色矩形  |  0 = 只看到黑色  |  N = 看不到窗口\n");
    LOG_DEBUG("    按键: ");
    fflush(stdout);
    
    // 黑色背景
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    // 白色矩形
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect rect = {60, 100, 120, 120};
    SDL_RenderFillRect(renderer, &rect);
    
    SDL_RenderPresent(renderer);
}

void test_5_multiple_colors(SDL_Renderer* renderer) {
    LOG_DEBUG("\n>>> 测试5: 多个彩色矩形\n");
    LOG_DEBUG("    窗口应该显示多个不同颜色的矩形\n");
    LOG_DEBUG("    你看到了什么？\n");
    LOG_DEBUG("    1 = 看到多个彩色矩形  |  0 = 只看到单色  |  N = 看不到窗口\n");
    LOG_DEBUG("    按键: ");
    fflush(stdout);
    
    // 白色背景
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    
    // 红色矩形
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_Rect rect1 = {10, 10, 60, 60};
    SDL_RenderFillRect(renderer, &rect1);
    
    // 绿色矩形
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_Rect rect2 = {80, 10, 60, 60};
    SDL_RenderFillRect(renderer, &rect2);
    
    // 蓝色矩形
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    SDL_Rect rect3 = {150, 10, 60, 60};
    SDL_RenderFillRect(renderer, &rect3);
    
    // 黄色矩形
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    SDL_Rect rect4 = {10, 80, 60, 60};
    SDL_RenderFillRect(renderer, &rect4);
    
    // 青色矩形
    SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
    SDL_Rect rect5 = {80, 80, 60, 60};
    SDL_RenderFillRect(renderer, &rect5);
    
    // 品红色矩形
    SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
    SDL_Rect rect6 = {150, 80, 60, 60};
    SDL_RenderFillRect(renderer, &rect6);
    
    SDL_RenderPresent(renderer);
}

int main() {
    LOG_DEBUG("===========================================\n");
    LOG_DEBUG("  SDL交互式渲染测试\n");
    LOG_DEBUG("===========================================\n");
    LOG_DEBUG("\n");
    LOG_DEBUG("初始化SDL...\n");
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        LOG_DEBUG("✗ SDL初始化失败: %s\n", SDL_GetError());
        return 1;
    }
    LOG_DEBUG("✓ SDL初始化成功\n");
    
    LOG_DEBUG("\n创建SDL窗口 (240x320)...\n");
    SDL_Window* window = SDL_CreateWindow(
        "SDL渲染测试 - 请观察窗口内容",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        240, 320,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        LOG_DEBUG("✗ 窗口创建失败: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    LOG_DEBUG("✓ 窗口创建成功\n");
    
    // 提升窗口到前台
    SDL_RaiseWindow(window);
    LOG_DEBUG("✓ 窗口已提升到前台\n");
    
    LOG_DEBUG("\n创建SDL渲染器...\n");
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (!renderer) {
        LOG_DEBUG("✗ 渲染器创建失败: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    LOG_DEBUG("✓ 渲染器创建成功\n");
    
    print_instructions();
    
    bool running = true;
    bool test_started = false;
    int current_test = 0;
    int test_results[5] = {-1, -1, -1, -1, -1};  // -1=未测试, 0=失败, 1=成功, 2=看不到窗口
    
    // 初始显示黑色屏幕
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                SDL_Keycode key = event.key.keysym.sym;
                
                if (key == SDLK_ESCAPE) {
                    LOG_DEBUG("\n\n测试已取消\n");
                    running = false;
                } else if (key == SDLK_SPACE && !test_started) {
                    test_started = true;
                    current_test = 1;
                    test_1_solid_red(renderer);
                } else if (test_started && current_test > 0 && current_test <= 5) {
                    // 记录测试结果
                    if (key == SDLK_1) {
                        LOG_DEBUG("1 (成功)\n");
                        test_results[current_test - 1] = 1;
                    } else if (key == SDLK_0) {
                        LOG_DEBUG("0 (失败)\n");
                        test_results[current_test - 1] = 0;
                    } else if (key == SDLK_n) {
                        LOG_DEBUG("N (看不到窗口)\n");
                        test_results[current_test - 1] = 2;
                    } else {
                        continue;  // 忽略其他按键
                    }
                    
                    // 进入下一个测试
                    current_test++;
                    SDL_Delay(500);  // 短暂延迟
                    
                    switch (current_test) {
                        case 2:
                            test_2_solid_green(renderer);
                            break;
                        case 3:
                            test_3_solid_blue(renderer);
                            break;
                        case 4:
                            test_4_white_rect(renderer);
                            break;
                        case 5:
                            test_5_multiple_colors(renderer);
                            break;
                        case 6:
                            // 所有测试完成
                            LOG_DEBUG("\n\n");
                            LOG_DEBUG("===========================================\n");
                            LOG_DEBUG("  测试结果总结\n");
                            LOG_DEBUG("===========================================\n");
                            LOG_DEBUG("\n");
                            
                            const char* test_names[] = {
                                "测试1: 全屏红色",
                                "测试2: 全屏绿色",
                                "测试3: 全屏蓝色",
                                "测试4: 白色矩形",
                                "测试5: 多个彩色矩形"
                            };
                            
                            int success_count = 0;
                            int fail_count = 0;
                            int no_window_count = 0;
                            
                            for (int i = 0; i < 5; i++) {
                                LOG_DEBUG("%s: ", test_names[i]);
                                if (test_results[i] == 1) {
                                    LOG_DEBUG("✓ 成功\n");
                                    success_count++;
                                } else if (test_results[i] == 0) {
                                    LOG_DEBUG("✗ 失败（看到错误内容）\n");
                                    fail_count++;
                                } else if (test_results[i] == 2) {
                                    LOG_DEBUG("✗ 失败（看不到窗口）\n");
                                    no_window_count++;
                                }
                            }
                            
                            LOG_DEBUG("\n");
                            LOG_DEBUG("总计: %d个成功, %d个失败, %d个看不到窗口\n", 
                                   success_count, fail_count, no_window_count);
                            LOG_DEBUG("\n");
                            
                            // 诊断建议
                            LOG_DEBUG("===========================================\n");
                            LOG_DEBUG("  诊断建议\n");
                            LOG_DEBUG("===========================================\n");
                            LOG_DEBUG("\n");
                            
                            if (success_count == 5) {
                                LOG_DEBUG("✓ 所有测试通过！SDL渲染完全正常。\n");
                                LOG_DEBUG("  问题可能在于J2ME的canvas texture渲染。\n");
                                LOG_DEBUG("  建议检查canvas texture的创建和使用。\n");
                            } else if (no_window_count > 0) {
                                LOG_DEBUG("✗ SDL窗口不可见！\n");
                                LOG_DEBUG("  可能原因：\n");
                                LOG_DEBUG("  1. 窗口在其他桌面/工作区\n");
                                LOG_DEBUG("  2. 窗口被其他窗口完全遮挡\n");
                                LOG_DEBUG("  3. SDL窗口创建失败但没有报错\n");
                                LOG_DEBUG("  4. 显示器配置问题\n");
                            } else if (fail_count > 0) {
                                LOG_DEBUG("✗ SDL渲染有问题！\n");
                                LOG_DEBUG("  看到了窗口但内容不正确。\n");
                                LOG_DEBUG("  可能原因：\n");
                                LOG_DEBUG("  1. 颜色格式问题\n");
                                LOG_DEBUG("  2. 渲染器配置问题\n");
                                LOG_DEBUG("  3. 显卡驱动问题\n");
                            } else {
                                LOG_DEBUG("部分测试通过。\n");
                                LOG_DEBUG("请检查失败的测试项。\n");
                            }
                            
                            LOG_DEBUG("\n按ESC退出...\n");
                            current_test = 0;  // 标记测试完成
                            break;
                    }
                }
            }
        }
        
        SDL_Delay(16);  // 约60 FPS
    }
    
    LOG_DEBUG("\n清理资源...\n");
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    LOG_DEBUG("✓ 测试程序已退出\n");
    LOG_DEBUG("\n");
    
    return 0;
}
