package java.lang;

public final class Integer extends Number implements Comparable {
    public static final int MIN_VALUE = -2147483648;
    public static final int MAX_VALUE = 2147483647;

    private int value;

    public Integer(int value) { this.value = value; }
    public Integer(String s) throws NumberFormatException { this.value = parseInt(s); }

    public native byte byteValue();
    public native short shortValue();
    public native int intValue();
    public native long longValue();
    public native float floatValue();
    public native double doubleValue();
    public native String toString();
    public native int hashCode();
    public native boolean equals(Object obj);
    public native int compareTo(Object anotherInteger);
    public static native String toString(int i);
    public static native String toHexString(int i);
    public static native String toOctalString(int i);
    public static native String toBinaryString(int i);
    public static native int parseInt(String s) throws NumberFormatException;
    public static native Integer valueOf(String s) throws NumberFormatException;
    public static native Integer valueOf(int i);
    public static native Integer decode(String nm) throws NumberFormatException;
    public static native int bitCount(int i);
    public static native int highestOneBit(int i);
    public static native int lowestOneBit(int i);
    public static native int numberOfLeadingZeros(int i);
    public static native int numberOfTrailingZeros(int i);
    public static native int reverse(int i);
    public static native int reverseBytes(int i);
    public static native int rotateLeft(int i, int distance);
    public static native int rotateRight(int i, int distance);
    public static native int signum(int i);
    public static native int sum(int a, int b);
    public static native int max(int a, int b);
    public static native int min(int a, int b);
}
