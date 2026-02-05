# J2ME模拟器构建指南

## 系统要求

### 支持的平台
- **Windows**: Windows 10/11 (x64)
- **Linux**: Ubuntu 18.04+ / CentOS 7+ / Arch Linux
- **macOS**: macOS 10.14+ (Intel/Apple Silicon)

### 必需的工具
- **CMake**: 3.15或更高版本
- **编译器**: 
  - GCC 7.0+ (Linux)
  - Clang 8.0+ (macOS)
  - MSVC 2019+ (Windows)
- **SDL2**: 2.0.12或更高版本

## 依赖项安装

### Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential cmake git
sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev
```

### CentOS/RHEL/Fedora
```bash
# CentOS/RHEL
sudo yum groupinstall "Development Tools"
sudo yum install cmake git SDL2-devel

# Fedora
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake git SDL2-devel
```

### macOS
```bash
# 使用Homebrew
brew install cmake sdl2

# 或使用MacPorts
sudo port install cmake libsdl2
```

### Windows
1. 安装Visual Studio 2019或更高版本
2. 下载并安装CMake: https://cmake.org/download/
3. 下载SDL2开发库: https://www.libsdl.org/download-2.0.php
4. 解压SDL2到 `C:\SDL2` 或设置环境变量

## 构建步骤

### 1. 获取源代码
```bash
git clone https://github.com/your-repo/j2me-emulator.git
cd j2me-emulator
```

### 2. 创建构建目录
```bash
mkdir build
cd build
```

### 3. 配置项目

#### Linux/macOS
```bash
# Debug构建
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release构建
cmake -DCMAKE_BUILD_TYPE=Release ..
```

#### Windows (Visual Studio)
```bash
# 生成Visual Studio项目
cmake -G "Visual Studio 16 2019" -A x64 ..

# 或使用MinGW
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
```

### 4. 编译项目

#### Linux/macOS
```bash
make -j$(nproc)
```

#### Windows
```bash
# Visual Studio
cmake --build . --config Release

# 或直接使用MSBuild
msbuild J2MEEmulator.sln /p:Configuration=Release
```

## 高级构建选项

### CMake配置选项
```bash
# 启用调试信息
cmake -DCMAKE_BUILD_TYPE=Debug ..

# 自定义安装路径
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..

# 指定SDL2路径 (Windows)
cmake -DSDL2_DIR=C:/SDL2 ..

# 启用静态链接
cmake -DBUILD_STATIC=ON ..
```

### 编译器特定选项
```bash
# 使用Clang
cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ ..

# 启用地址消毒器 (调试用)
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON ..

# 启用性能分析
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_PROFILING=ON ..
```

## 交叉编译

### Android (未来支持)
```bash
cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=arm64-v8a \
      -DANDROID_PLATFORM=android-21 ..
```

### 嵌入式Linux
```bash
cmake -DCMAKE_TOOLCHAIN_FILE=toolchain-arm.cmake ..
```

## 安装

### 系统安装
```bash
sudo make install
```

### 打包安装
```bash
# 创建DEB包 (Ubuntu/Debian)
cpack -G DEB

# 创建RPM包 (CentOS/Fedora)
cpack -G RPM

# 创建安装程序 (Windows)
cpack -G NSIS
```

## 故障排除

### 常见问题

#### SDL2未找到
```bash
# 设置SDL2路径
export SDL2_DIR=/usr/local/lib/cmake/SDL2

# 或在CMake中指定
cmake -DSDL2_DIR=/path/to/sdl2 ..
```

#### 编译错误
```bash
# 清理构建目录
rm -rf build/*
cmake ..
make clean && make
```

#### 链接错误
```bash
# 检查库依赖
ldd J2MEEmulator  # Linux
otool -L J2MEEmulator  # macOS
```

### 性能优化构建
```bash
# 最大优化
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_FLAGS="-O3 -march=native" \
      -DCMAKE_CXX_FLAGS="-O3 -march=native" ..

# 启用LTO (链接时优化)
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON ..
```

## 开发环境设置

### VS Code配置
创建 `.vscode/settings.json`:
```json
{
    "cmake.buildDirectory": "${workspaceFolder}/build",
    "cmake.configureArgs": [
        "-DCMAKE_BUILD_TYPE=Debug"
    ],
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools"
}
```

### CLion配置
1. 打开项目根目录
2. CLion会自动检测CMakeLists.txt
3. 配置构建类型和工具链

## 持续集成

### GitHub Actions示例
```yaml
name: Build
on: [push, pull_request]
jobs:
  build:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest, macos-latest]
    steps:
    - uses: actions/checkout@v2
    - name: Install dependencies
      run: |
        # 平台特定的依赖安装命令
    - name: Build
      run: |
        mkdir build && cd build
        cmake ..
        cmake --build .
```

## 验证构建

### 运行测试
```bash
# 基本功能测试
./J2MEEmulator --test

# 性能基准测试
./J2MEEmulator --benchmark

# 内存泄漏检测
valgrind --leak-check=full ./J2MEEmulator
```

### 检查二进制
```bash
# 查看依赖库
ldd J2MEEmulator

# 检查符号表
nm J2MEEmulator | grep j2me

# 查看文件信息
file J2MEEmulator
```