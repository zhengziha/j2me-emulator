package java.lang;

import java.io.PrintStream;

public final class System {
    public static PrintStream out;
    public static PrintStream err;
    public static PrintStream in;

    private System() {}

    public static native long currentTimeMillis();
    public static native void arraycopy(Object src, int srcPos, Object dest, int destPos, int length);
    public static native int identityHashCode(Object x);
    public static native String getProperty(String key);
    public static native void exit(int status);
}
