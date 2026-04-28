package java.lang;

public final class StackTraceElement implements java.io.Serializable {
    private String declaringClass;
    private String methodName;
    private String fileName;
    private int lineNumber;

    public StackTraceElement(String declaringClass, String methodName, String fileName, int lineNumber) {
        this.declaringClass = declaringClass;
        this.methodName = methodName;
        this.fileName = fileName;
        this.lineNumber = lineNumber;
    }

    public String getFileName() { return fileName; }
    public int getLineNumber() { return lineNumber; }
    public String getClassName() { return declaringClass; }
    public String getMethodName() { return methodName; }
    public boolean isNativeMethod() { return lineNumber == -2; }
    public String toString() { return declaringClass + "." + methodName + "(" + fileName + ":" + lineNumber + ")"; }
    public boolean equals(Object other) { return false; }
    public int hashCode() { return 0; }
}
