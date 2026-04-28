package javax.microedition.lcdui;

public class Graphics {
    public static final int TOP = 16;
    public static final int BOTTOM = 32;
    public static final int LEFT = 4;
    public static final int RIGHT = 8;
    public static final int HCENTER = 1;
    public static final int VCENTER = 2;
    public static final int BASELINE = 64;
    public void setColor(int red, int green, int blue) {}
    public void setColor(int rgb) {}
    public void fillRect(int x, int y, int width, int height) {}
    public void drawRect(int x, int y, int width, int height) {}
    public void drawString(String str, int x, int y, int anchor) {}
    public void drawLine(int x1, int y1, int x2, int y2) {}
    public void drawOval(int x, int y, int width, int height) {}
    public void fillOval(int x, int y, int width, int height) {}
    public void drawArc(int x, int y, int w, int h, int startAngle, int arcAngle) {}
    public void drawImage(Image img, int x, int y, int anchor) {}
}
