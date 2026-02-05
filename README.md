# J2ME Emulator

基于phoneME的跨平台J2ME模拟器，使用C/C++和SDL2开发。

## 项目概述

本项目旨在创建一个高性能的J2ME模拟器，能够运行大多数2D游戏。设计重点：
- 高性能解释器实现
- 跨平台兼容性（Windows、Linux、macOS）
- 清晰的代码架构和层次结构
- 完整的文档和注释

## 技术栈

- **语言**: C/C++
- **图形库**: SDL2
- **构建系统**: CMake
- **参考实现**: phoneME

## 项目结构

```
j2me-emulator/
├── src/                    # 源代码
│   ├── core/              # 核心虚拟机
│   ├── interpreter/       # 字节码解释器
│   ├── graphics/          # 图形渲染
│   ├── audio/             # 音频处理
│   ├── platform/          # 平台相关代码
│   └── utils/             # 工具函数
├── include/               # 头文件
├── libs/                  # 第三方库
├── docs/                  # 文档
├── tests/                 # 测试代码
├── examples/              # 示例程序
└── build/                 # 构建输出
```

## 构建说明

### 依赖项
- CMake 3.15+
- SDL2
- C++17兼容编译器

### 构建步骤
```bash
mkdir build
cd build
cmake ..
make
```

## 开发计划

1. **阶段1**: 基础架构和虚拟机核心
2. **阶段2**: 字节码解释器
3. **阶段3**: 图形和输入系统
4. **阶段4**: 音频系统
5. **阶段5**: 优化和测试

## 许可证

MIT License