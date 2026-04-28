#!/bin/bash

# MIDlet编译和打包脚本（含运行期API stub）

echo "=== 编译SimpleMIDlet ==="

# 清理旧文件
rm -rf classes build_stubs
rm -f SimpleMIDlet.jar

# 创建输出目录
mkdir -p classes build_stubs

# 第一步：编译核心API stub（java.lang / java.io）
echo "编译核心API stub..."
javac -source 1.8 -target 1.8 -d build_stubs stubs/java/lang/*.java stubs/java/io/*.java 2>&1
if [ $? -ne 0 ]; then
    echo "❌ 核心API stub编译失败"
    exit 1
fi

# 第二步：编译MIDP stub（需要核心API stub作为classpath）
echo "编译MIDP stub..."
javac -source 1.8 -target 1.8 -cp build_stubs -d build_stubs stubs/javax/microedition/midlet/MIDlet.java stubs/javax/microedition/lcdui/*.java 2>&1
if [ $? -ne 0 ]; then
    echo "❌ MIDP stub编译失败"
    exit 1
fi

# 第三步：编译应用代码（使用所有stub作为classpath）
echo "编译Java源文件..."
javac -source 1.8 -target 1.8 -cp build_stubs -d build_stubs SimpleMIDlet.java 2>&1
if [ $? -ne 0 ]; then
    echo "❌ 编译失败"
    exit 1
fi

echo "✅ 编译成功"

# 创建JAR文件（包含所有类：应用 + stub）
echo "创建JAR文件..."
jar cfm SimpleMIDlet.jar MANIFEST.MF -C build_stubs .

if [ -f SimpleMIDlet.jar ]; then
    echo "✅ JAR文件创建成功: SimpleMIDlet.jar"
    echo ""
    echo "JAR内容:"
    jar tf SimpleMIDlet.jar
    echo ""
    echo "可以使用以下命令运行:"
    echo "./build/J2MEEmulator test_simple/SimpleMIDlet.jar"
else
    echo "❌ JAR文件创建失败"
    exit 1
fi
