import javax.microedition.midlet.MIDlet;
import javax.microedition.lcdui.Display;

public class CanvasTest extends MIDlet {
    private SimpleCanvas canvas;
    
    public CanvasTest() {
        canvas = new SimpleCanvas();
    }
    
    public void startApp() {
        Display display = Display.getDisplay(this);
        display.setCurrent(canvas);
    }
    
    public void pauseApp() {
    }
    
    public void destroyApp(boolean unconditional) {
    }
}
