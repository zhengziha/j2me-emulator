import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;

/**
 * 简单的测试MIDlet
 * 用于验证解释器的基本功能
 */
public class SimpleMIDlet extends MIDlet {
    
    public SimpleMIDlet() {
        System.out.println("SimpleMIDlet: 构造函数");
    }
    
    protected void startApp() {
        System.out.println("SimpleMIDlet: startApp开始");
        
        // 测试基本运算
        int a = 10;
        int b = 20;
        int c = a + b;
        System.out.println("SimpleMIDlet: 10 + 20 = " + c);
        
        // 测试条件判断
        if (c == 30) {
            System.out.println("SimpleMIDlet: 条件判断正确");
        } else {
            System.out.println("SimpleMIDlet: 条件判断错误");
        }
        
        // 测试循环
        int sum = 0;
        for (int i = 1; i <= 5; i++) {
            sum += i;
        }
        System.out.println("SimpleMIDlet: 1+2+3+4+5 = " + sum);
        
        // 测试对象创建
        SimpleCanvas canvas = new SimpleCanvas();
        System.out.println("SimpleMIDlet: Canvas创建成功");
        
        // 测试Display
        Display display = Display.getDisplay(this);
        display.setCurrent(canvas);
        System.out.println("SimpleMIDlet: Display.setCurrent完成");
        
        System.out.println("SimpleMIDlet: startApp完成");
    }
    
    protected void pauseApp() {
        System.out.println("SimpleMIDlet: pauseApp");
    }
    
    protected void destroyApp(boolean unconditional) {
        System.out.println("SimpleMIDlet: destroyApp");
    }
}

class SimpleCanvas extends Canvas {
    
    public SimpleCanvas() {
        System.out.println("SimpleCanvas: 构造函数");
    }
    
    protected void paint(Graphics g) {
        System.out.println("SimpleCanvas: paint开始");
        
        // 设置白色背景
        g.setColor(255, 255, 255);
        g.fillRect(0, 0, getWidth(), getHeight());
        
        // 绘制红色矩形
        g.setColor(255, 0, 0);
        g.fillRect(10, 10, 100, 50);
        
        // 绘制蓝色文字
        g.setColor(0, 0, 255);
        g.drawString("Hello J2ME!", 10, 70, Graphics.TOP | Graphics.LEFT);
        
        System.out.println("SimpleCanvas: paint完成");
    }
}
