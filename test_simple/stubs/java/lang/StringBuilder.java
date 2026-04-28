package java.lang;

public final class StringBuilder implements CharSequence, java.io.Serializable {
    public StringBuilder() {}
    public StringBuilder(int capacity) {}
    public StringBuilder(String str) {}
    public StringBuilder(CharSequence seq) {}

    public StringBuilder append(String str) { return this; }
    public StringBuilder append(boolean b) { return this; }
    public StringBuilder append(char c) { return this; }
    public StringBuilder append(int i) { return this; }
    public StringBuilder append(long lng) { return this; }
    public StringBuilder append(float f) { return this; }
    public StringBuilder append(double d) { return this; }
    public StringBuilder append(Object obj) { return this; }
    public StringBuilder append(char[] str) { return this; }
    public StringBuilder append(CharSequence s) { return this; }

    public String toString() { return ""; }
    public int length() { return 0; }
    public char charAt(int index) { return '\0'; }
    public CharSequence subSequence(int start, int end) { return ""; }
    public int compareTo(Object o) { return 0; }
}
