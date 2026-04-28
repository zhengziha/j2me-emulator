package java.lang;

public abstract class Number implements java.io.Serializable {
    public Number() {}
    public native int intValue();
    public native long longValue();
    public native float floatValue();
    public native double doubleValue();
    public native byte byteValue();
    public native short shortValue();
}
