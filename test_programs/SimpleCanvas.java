import javax.microedition.lcdui.Canvas;
import javax.microedition.lcdui.Graphics;

public class SimpleCanvas extends Canvas {
    public void paint(Graphics g) {
        // 设置背景为黑色
        g.setColor(0, 0, 0);
        g.fillRect(0, 0, getWidth(), getHeight());
        
        // 绘制红色矩形
        g.setColor(255, 0, 0);
        g.fillRect(10, 10, 100, 100);
        
        // 绘制绿色矩形
        g.setColor(0, 255, 0);
        g.fillRect(120, 10, 100, 100);
        
        // 绘制蓝色矩形
        g.setColor(0, 0, 255);
        g.fillRect(10, 120, 100, 100);
        
        // 绘制黄色矩形
        g.setColor(255, 255, 0);
        g.fillRect(120, 120, 100, 100);
    }
}
