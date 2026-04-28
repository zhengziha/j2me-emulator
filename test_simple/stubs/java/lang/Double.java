package java.lang;

public final class Double extends Number implements Comparable {
    public static final double POSITIVE_INFINITY = 1.0 / 0.0;
    public static final double NEGATIVE_INFINITY = -1.0 / 0.0;
    public static final double NaN = 0.0 / 0.0;
    public static final double MAX_VALUE = 0x1.fffffffffffffP+1023;
    public static final double MIN_VALUE = 0x0.0000000000001P-1022;
    public static final int MAX_EXPONENT = 1023;
    public static final int MIN_EXPONENT = -1022;
    public static final int SIZE = 64;
    public static final int BYTES = 8;

    private double value;

    public Double(double value) { this.value = value; }
    public Double(float value) { this.value = value; }
    public Double(String s) throws NumberFormatException { this.value = parseDouble(s); }

    public native boolean isNaN();
    public native boolean isInfinite();
    public native String toString();
    public native byte byteValue();
    public native short shortValue();
    public native int intValue();
    public native long longValue();
    public native float floatValue();
    public native double doubleValue();
    public native int hashCode();
    public native boolean equals(Object obj);
    public static native long doubleToLongBits(double value);
    public static native long doubleToRawLongBits(double value);
    public static native double longBitsToDouble(long bits);
    public static native Double valueOf(String s) throws NumberFormatException;
    public static native String toString(double d);
    public static native String toHexString(double d);
    public static native double parseDouble(String s) throws NumberFormatException;
    public static native boolean isInfinite(double v);
    public static native boolean isNaN(double v);
    public native int compareTo(Object anotherDouble);
}
