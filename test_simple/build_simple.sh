#!/bin/bash

# 简单测试类编译脚本

echo "=== 编译SimpleTest ==="

# 清理旧文件
rm -rf classes
rm -f SimpleTest.jar

# 创建输出目录
mkdir -p classes

# 编译
echo "编译Java源文件..."
javac -source 1.8 -target 1.8 -d classes SimpleTest.java

if [ $? -ne 0 ]; then
    echo "❌ 编译失败"
    exit 1
fi

echo "✅ 编译成功"

# 查看生成的class文件
echo ""
echo "生成的class文件:"
ls -lh classes/

# 反编译查看字节码
echo ""
echo "=== 字节码信息 ==="
javap -c -v classes/SimpleTest.class | head -100

echo ""
echo "✅ 编译完成"
echo "class文件位置: classes/SimpleTest.class"
