# Phase 4: 下一步行动计划

## 当前状态分析

### 已完成 ✓
1. JAR文件成功加载和解析
2. 类文件成功解析（XMIDlet）
3. 字节码成功执行（构造方法、startApp、pauseApp）
4. 游戏循环运行（300帧，57 FPS）

### 发现的问题

通过反编译XMIDlet.class，我们发现游戏的startApp方法执行以下操作：

```java
protected void startApp() {
    if (this.a == null) {  // a是Canvas实例
        this.a = new j();  // 创建Canvas（j类）
        Display.getDisplay(this).setCurrent(y.a());  // 设置显示
        Thread thread = new Thread(this.a);  // 创建游戏线程
        thread.start();  // 启动线程
    }
}
```

### 缺失的组件

1. **父类加载**
   - `javax.microedition.midlet.MIDlet` - MIDlet基类
   - 需要实现构造方法和notifyDestroyed()

2. **Display API**
   - `javax.microedition.lcdui.Display`
   - `Display.getDisplay(MIDlet)` - 获取Display实例
   - `Display.setCurrent(Displayable)` - 设置当前显示

3. **Canvas类**
   - `j` 类（游戏的Canvas子类）
   - 需要从JAR加载
   - 需要实现paint()方法

4. **Thread支持**
   - `java.lang.Thread`
   - `Thread.<init>(Runnable)`
   - `Thread.start()`

5. **Displayable类**
   - `javax.microedition.lcdui.Displayable`
   - Canvas的父类

## 实现策略

### 方案A：完整实现（推荐）
实现完整的MIDP类层次结构：

```
java.lang.Object
  └─ javax.microedition.midlet.MIDlet
  └─ javax.microedition.lcdui.Displayable
      └─ javax.microedition.lcdui.Canvas
          └─ j (游戏Canvas)
          └─ y (另一个Canvas)
```

**优点**：
- 完整支持MIDP规范
- 可以运行更多游戏
- 架构清晰

**缺点**：
- 工作量大
- 需要实现很多类

### 方案B：Stub实现（快速原型）
创建最小的stub类，只实现游戏需要的方法：

```c
// 在本地方法中模拟这些类
- MIDlet.<init>() -> 返回成功
- Display.getDisplay() -> 返回Display对象
- Display.setCurrent() -> 设置vm->current_canvas
- Thread.<init>() -> 创建线程对象
- Thread.start() -> 启动游戏循环
```

**优点**：
- 快速实现
- 可以立即看到效果

**缺点**：
- 不完整
- 难以扩展

### 方案C：混合方案（平衡）
1. 为关键类创建真实的Java类文件
2. 使用本地方法实现核心功能
3. 逐步完善

## 推荐的实现顺序

### 第1步：实现Display系统（1-2小时）
```c
// 在j2me_native_methods.c中添加
- Display_getDisplay() -> 返回vm->display
- Display_setCurrent() -> 设置当前Canvas
- Display_getCurrent() -> 获取当前Canvas
```

### 第2步：加载Canvas类（30分钟）
```c
// 从JAR加载j类和y类
- j2me_class_loader_load_class(loader, "j")
- j2me_class_loader_load_class(loader, "y")
```

### 第3步：实现Thread支持（1小时）
```c
// 简化的线程实现
- Thread.<init>() -> 保存Runnable
- Thread.start() -> 在游戏循环中调用run()
```

### 第4步：实现Canvas.paint()调用（2小时）
```c
// 在游戏循环中
- 调用canvas.paint(graphics)
- 将Graphics绘制到SDL surface
- 显示到屏幕
```

### 第5步：实现输入处理（1小时）
```c
// 将SDL事件转换为Canvas事件
- keyPressed(keyCode)
- keyReleased(keyCode)
```

## 立即行动

### 最小可行实现（2-3小时）

1. **创建Display stub**
```c
// 在invoke_static中添加特殊处理
if (strcmp(class_name, "javax/microedition/lcdui/Display") == 0) {
    if (strcmp(method_name, "getDisplay") == 0) {
        // 返回Display对象引用
        push_to_stack(vm->display);
        return J2ME_SUCCESS;
    }
}
```

2. **创建Thread stub**
```c
// 在invoke_special中添加
if (strcmp(class_name, "java/lang/Thread") == 0) {
    if (strcmp(method_name, "<init>") == 0) {
        // 保存Runnable到线程对象
        return J2ME_SUCCESS;
    }
}
```

3. **加载游戏Canvas**
```c
// 在startApp执行后
j2me_class_t* canvas_class = j2me_class_loader_load_class(loader, "j");
```

4. **调用Canvas.paint()**
```c
// 在游戏循环中
if (canvas_instance) {
    j2me_method_t* paint = find_method(canvas_class, "paint", "(Ljavax/microedition/lcdui/Graphics;)V");
    if (paint) {
        execute_method(vm, paint, canvas_instance, graphics);
    }
}
```

## 预期结果

完成上述步骤后，应该能够：
1. ✓ 游戏成功创建Canvas
2. ✓ Display.setCurrent()成功设置Canvas
3. ✓ 游戏线程成功启动
4. ✓ Canvas.paint()被定期调用
5. ✓ 游戏画面显示在屏幕上
6. ✓ 键盘输入被传递给游戏

## 时间估算

- **最小实现**: 2-3小时
- **基本可玩**: 4-6小时
- **完整实现**: 10-15小时

## 下一个命令

开始实现Display stub：
```bash
# 编辑src/core/j2me_method_invocation.c
# 在invoke_static中添加Display.getDisplay()的处理
```
