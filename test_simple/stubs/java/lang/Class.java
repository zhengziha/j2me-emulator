package java.lang;

public final class Class implements java.io.Serializable {
    Class() {}
    public static native Class forName(String className) throws ClassNotFoundException;
    public native Object newInstance() throws InstantiationException, IllegalAccessException;
    public native boolean isInstance(Object obj);
    public native boolean isAssignableFrom(Class cls);
    public native boolean isInterface();
    public native boolean isArray();
    public native boolean isPrimitive();
    public native String getName();
    public native Class getSuperclass();
    public static native Class getPrimitiveClass(String name);
    public native String toString();
    public native int hashCode();
    public native boolean equals(Object obj);
}
