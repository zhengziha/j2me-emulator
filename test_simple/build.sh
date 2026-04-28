#!/bin/bash

# 简单MIDlet编译和打包脚本

echo "=== 编译SimpleMIDlet ==="

# 清理旧文件
rm -rf classes
rm -f SimpleMIDlet.jar

# 创建输出目录
mkdir -p classes

# 编译（使用Java 8兼容模式，不验证MIDP API）
echo "编译Java源文件..."
javac -source 1.8 -target 1.8 -bootclasspath "" -extdirs "" -d classes SimpleMIDlet.java 2>&1 | grep -v "warning: \[options\]" || true

if [ $? -ne 0 ]; then
    echo "❌ 编译失败"
    exit 1
fi

echo "✅ 编译成功"

# 创建JAR文件
echo "创建JAR文件..."
cd classes
jar cfm ../SimpleMIDlet.jar ../MANIFEST.MF *.class
cd ..

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
