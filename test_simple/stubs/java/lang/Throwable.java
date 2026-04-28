package java.lang;

public class Throwable implements java.io.Serializable {
    public Throwable() { fillInStackTrace(); }
    public Throwable(String message) { fillInStackTrace(); }
    public Throwable(String message, Throwable cause) { fillInStackTrace(); }
    public Throwable(Throwable cause) { fillInStackTrace(); }

    public native String getMessage();
    public native Throwable getCause();
    public native Throwable initCause(Throwable cause);
    public native void printStackTrace();
    public native String toString();
    public native StackTraceElement[] getStackTrace();
    public native void setStackTrace(StackTraceElement[] stackTrace);
    public native Throwable fillInStackTrace();
}
