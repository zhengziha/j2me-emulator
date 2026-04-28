package java.lang;

public final class Long extends Number implements Comparable {
    public static final long MIN_VALUE = -9223372036854775808L;
    public static final long MAX_VALUE = 9223372036854775807L;

    private long value;

    public Long(long value) { this.value = value; }
    public Long(String s) throws NumberFormatException { this.value = parseLong(s); }

    public native byte byteValue();
    public native short shortValue();
    public native int intValue();
    public native long longValue();
    public native float floatValue();
    public native double doubleValue();
    public native String toString();
    public native int hashCode();
    public native boolean equals(Object obj);
    public native int compareTo(Object anotherLong);
    public static native String toString(long i);
    public static native String toHexString(long i);
    public static native long parseLong(String s) throws NumberFormatException;
    public static native Long valueOf(String s) throws NumberFormatException;
    public static native Long valueOf(long l);
    public static native int bitCount(long i);
    public static native long reverse(long i);
    public static native long reverseBytes(long i);
    public static native int signum(long i);
    public static native long sum(long a, long b);
    public static native long max(long a, long b);
    public static native long min(long a, long b);
}
