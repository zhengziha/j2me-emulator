package java.lang;

public final class Float extends Number implements Comparable {
    public static final float POSITIVE_INFINITY = 1.0f / 0.0f;
    public static final float NEGATIVE_INFINITY = -1.0f / 0.0f;
    public static final float NaN = 0.0f / 0.0f;
    public static final float MAX_VALUE = 0x1.fffffeP+127f;
    public static final float MIN_VALUE = 0x0.000002P-126f;
    public static final int MAX_EXPONENT = 127;
    public static final int MIN_EXPONENT = -126;
    public static final int SIZE = 32;
    public static final int BYTES = 4;

    private float value;

    public Float(float value) { this.value = value; }
    public Float(double value) { this.value = (float) value; }
    public Float(String s) throws NumberFormatException { this.value = parseFloat(s); }

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
    public static native int floatToIntBits(float value);
    public static native int floatToRawIntBits(float value);
    public static native float intBitsToFloat(int bits);
    public static native Float valueOf(String s) throws NumberFormatException;
    public static native Float valueOf(float f);
    public static native String toString(float f);
    public static native String toHexString(float f);
    public static native float parseFloat(String s) throws NumberFormatException;
    public static native boolean isInfinite(float v);
    public static native boolean isNaN(float v);
    public native int compareTo(Object anotherFloat);
}
