package java.lang;

public final class String implements CharSequence, java.io.Serializable {
    public String() {}
    public String(String value) {}
    public String(char[] value) {}
    public String(char[] value, int offset, int count) {}
    public String(byte[] bytes) {}

    public native int length();
    public native boolean isEmpty();
    public native char charAt(int index);
    public native int compareTo(String anotherString);
    public native boolean equals(Object anObject);
    public native boolean equalsIgnoreCase(String str);
    public native int hashCode();
    public native String substring(int beginIndex);
    public native String substring(int beginIndex, int endIndex);
    public native String concat(String str);
    public native int indexOf(int ch);
    public native int indexOf(int ch, int fromIndex);
    public native int indexOf(String str);
    public native int indexOf(String str, int fromIndex);
    public native int lastIndexOf(int ch);
    public native int lastIndexOf(int ch, int fromIndex);
    public native int lastIndexOf(String str);
    public native int lastIndexOf(String str, int fromIndex);
    public native String trim();
    public native String toLowerCase();
    public native String toUpperCase();
    public native char[] toCharArray();
    public static native String valueOf(Object obj);
    public static native String valueOf(char[] data);
    public static native String valueOf(char[] data, int offset, int count);
    public static native String valueOf(boolean b);
    public static native String valueOf(char c);
    public static native String valueOf(int i);
    public static native String valueOf(long l);
    public static native String valueOf(float f);
    public static native String valueOf(double d);
    public native String intern();
    public native boolean startsWith(String prefix);
    public native boolean endsWith(String suffix);
    public native String replace(char oldChar, char newChar);
    public native String[] split(String regex);
    public native byte[] getBytes();

    public CharSequence subSequence(int start, int end) { return substring(start, end); }
    public int compareTo(Object o) { return 0; }
}
