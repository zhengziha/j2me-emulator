package java.lang;

public class Thread implements Runnable {
    public Thread() {}
    public Thread(Runnable target) {}
    public Thread(String name) {}

    public synchronized native void start();
    public void run() {}
    public static native Thread currentThread();
    public native String getName();
    public native void setName(String name);
    public native boolean isAlive();
    public static native int activeCount();
    public static native void sleep(long millis) throws InterruptedException;
    public static native void yield();
    public native void interrupt();
    public native boolean isInterrupted();
    public static native boolean interrupted();
    public native void setPriority(int newPriority);
    public native int getPriority();
}
