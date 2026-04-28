package java.lang;

public class Runtime {
    private Runtime() {}

    public static native Runtime getRuntime();
    public native void exit(int status);
    public native void gc();
    public native long freeMemory();
    public native long totalMemory();
    public native long maxMemory();
    public native int availableProcessors();
}
