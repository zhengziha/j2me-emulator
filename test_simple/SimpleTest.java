/**
 * 最简单的测试类
 * 只测试基本的Java字节码指令
 */
public class SimpleTest {
    
    // 静态字段
    public static int staticValue = 100;
    
    // 实例字段
    public int instanceValue;
    
    // 构造函数
    public SimpleTest() {
        this.instanceValue = 42;
    }
    
    // 测试基本运算
    public static int testArithmetic() {
        int a = 10;
        int b = 20;
        int c = a + b;  // 30
        int d = c - 5;  // 25
        int e = d * 2;  // 50
        int f = e / 5;  // 10
        return f;
    }
    
    // 测试条件判断
    public static int testCondition(int x) {
        if (x > 0) {
            return 1;
        } else if (x < 0) {
            return -1;
        } else {
            return 0;
        }
    }
    
    // 测试循环
    public static int testLoop() {
        int sum = 0;
        for (int i = 1; i <= 10; i++) {
            sum = sum + i;
        }
        return sum;  // 应该返回55
    }
    
    // 测试方法调用
    public int testMethodCall() {
        return this.instanceValue + staticValue;
    }
    
    // 主方法（模拟MIDlet的startApp）
    public static void main() {
        // 测试运算
        int result1 = testArithmetic();
        
        // 测试条件
        int result2 = testCondition(5);
        int result3 = testCondition(-5);
        int result4 = testCondition(0);
        
        // 测试循环
        int result5 = testLoop();
        
        // 测试对象创建和方法调用
        SimpleTest obj = new SimpleTest();
        int result6 = obj.testMethodCall();
    }
}
