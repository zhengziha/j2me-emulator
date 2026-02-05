#ifndef J2ME_BYTECODE_H
#define J2ME_BYTECODE_H

#include "j2me_types.h"

/**
 * @file j2me_bytecode.h
 * @brief J2ME字节码指令定义
 * 
 * 定义完整的Java字节码指令集和相关常量
 */

// 常量指令 (0x00-0x14)
#define OPCODE_NOP          0x00    // 无操作
#define OPCODE_ACONST_NULL  0x01    // 将null压入栈
#define OPCODE_ICONST_M1    0x02    // 将int常量-1压入栈
#define OPCODE_ICONST_0     0x03    // 将int常量0压入栈
#define OPCODE_ICONST_1     0x04    // 将int常量1压入栈
#define OPCODE_ICONST_2     0x05    // 将int常量2压入栈
#define OPCODE_ICONST_3     0x06    // 将int常量3压入栈
#define OPCODE_ICONST_4     0x07    // 将int常量4压入栈
#define OPCODE_ICONST_5     0x08    // 将int常量5压入栈
#define OPCODE_LCONST_0     0x09    // 将long常量0压入栈
#define OPCODE_LCONST_1     0x0a    // 将long常量1压入栈
#define OPCODE_FCONST_0     0x0b    // 将float常量0压入栈
#define OPCODE_FCONST_1     0x0c    // 将float常量1压入栈
#define OPCODE_FCONST_2     0x0d    // 将float常量2压入栈
#define OPCODE_DCONST_0     0x0e    // 将double常量0压入栈
#define OPCODE_DCONST_1     0x0f    // 将double常量1压入栈
#define OPCODE_BIPUSH       0x10    // 将byte值压入栈
#define OPCODE_SIPUSH       0x11    // 将short值压入栈
#define OPCODE_LDC          0x12    // 从常量池加载常量
#define OPCODE_LDC_W        0x13    // 从常量池加载常量(宽索引)
#define OPCODE_LDC2_W       0x14    // 从常量池加载long/double常量

// 局部变量加载指令 (0x15-0x2d)
#define OPCODE_ILOAD        0x15    // 从局部变量加载int
#define OPCODE_LLOAD        0x16    // 从局部变量加载long
#define OPCODE_FLOAD        0x17    // 从局部变量加载float
#define OPCODE_DLOAD        0x18    // 从局部变量加载double
#define OPCODE_ALOAD        0x19    // 从局部变量加载引用
#define OPCODE_ILOAD_0      0x1a    // 从局部变量0加载int
#define OPCODE_ILOAD_1      0x1b    // 从局部变量1加载int
#define OPCODE_ILOAD_2      0x1c    // 从局部变量2加载int
#define OPCODE_ILOAD_3      0x1d    // 从局部变量3加载int
#define OPCODE_LLOAD_0      0x1e    // 从局部变量0加载long
#define OPCODE_LLOAD_1      0x1f    // 从局部变量1加载long
#define OPCODE_LLOAD_2      0x20    // 从局部变量2加载long
#define OPCODE_LLOAD_3      0x21    // 从局部变量3加载long
#define OPCODE_FLOAD_0      0x22    // 从局部变量0加载float
#define OPCODE_FLOAD_1      0x23    // 从局部变量1加载float
#define OPCODE_FLOAD_2      0x24    // 从局部变量2加载float
#define OPCODE_FLOAD_3      0x25    // 从局部变量3加载float
#define OPCODE_DLOAD_0      0x26    // 从局部变量0加载double
#define OPCODE_DLOAD_1      0x27    // 从局部变量1加载double
#define OPCODE_DLOAD_2      0x28    // 从局部变量2加载double
#define OPCODE_DLOAD_3      0x29    // 从局部变量3加载double
#define OPCODE_ALOAD_0      0x2a    // 从局部变量0加载引用
#define OPCODE_ALOAD_1      0x2b    // 从局部变量1加载引用
#define OPCODE_ALOAD_2      0x2c    // 从局部变量2加载引用
#define OPCODE_ALOAD_3      0x2d    // 从局部变量3加载引用

// 数组加载指令 (0x2e-0x35)
#define OPCODE_IALOAD       0x2e    // 从int数组加载
#define OPCODE_LALOAD       0x2f    // 从long数组加载
#define OPCODE_FALOAD       0x30    // 从float数组加载
#define OPCODE_DALOAD       0x31    // 从double数组加载
#define OPCODE_AALOAD       0x32    // 从引用数组加载
#define OPCODE_BALOAD       0x33    // 从byte/boolean数组加载
#define OPCODE_CALOAD       0x34    // 从char数组加载
#define OPCODE_SALOAD       0x35    // 从short数组加载

// 局部变量存储指令 (0x36-0x4e)
#define OPCODE_ISTORE       0x36    // 存储int到局部变量
#define OPCODE_LSTORE       0x37    // 存储long到局部变量
#define OPCODE_FSTORE       0x38    // 存储float到局部变量
#define OPCODE_DSTORE       0x39    // 存储double到局部变量
#define OPCODE_ASTORE       0x3a    // 存储引用到局部变量
#define OPCODE_ISTORE_0     0x3b    // 存储int到局部变量0
#define OPCODE_ISTORE_1     0x3c    // 存储int到局部变量1
#define OPCODE_ISTORE_2     0x3d    // 存储int到局部变量2
#define OPCODE_ISTORE_3     0x3e    // 存储int到局部变量3
#define OPCODE_LSTORE_0     0x3f    // 存储long到局部变量0
#define OPCODE_LSTORE_1     0x40    // 存储long到局部变量1
#define OPCODE_LSTORE_2     0x41    // 存储long到局部变量2
#define OPCODE_LSTORE_3     0x42    // 存储long到局部变量3
#define OPCODE_FSTORE_0     0x43    // 存储float到局部变量0
#define OPCODE_FSTORE_1     0x44    // 存储float到局部变量1
#define OPCODE_FSTORE_2     0x45    // 存储float到局部变量2
#define OPCODE_FSTORE_3     0x46    // 存储float到局部变量3
#define OPCODE_DSTORE_0     0x47    // 存储double到局部变量0
#define OPCODE_DSTORE_1     0x48    // 存储double到局部变量1
#define OPCODE_DSTORE_2     0x49    // 存储double到局部变量2
#define OPCODE_DSTORE_3     0x4a    // 存储double到局部变量3
#define OPCODE_ASTORE_0     0x4b    // 存储引用到局部变量0
#define OPCODE_ASTORE_1     0x4c    // 存储引用到局部变量1
#define OPCODE_ASTORE_2     0x4d    // 存储引用到局部变量2
#define OPCODE_ASTORE_3     0x4e    // 存储引用到局部变量3

// 数组存储指令 (0x4f-0x56)
#define OPCODE_IASTORE      0x4f    // 存储到int数组
#define OPCODE_LASTORE      0x50    // 存储到long数组
#define OPCODE_FASTORE      0x51    // 存储到float数组
#define OPCODE_DASTORE      0x52    // 存储到double数组
#define OPCODE_AASTORE      0x53    // 存储到引用数组
#define OPCODE_BASTORE      0x54    // 存储到byte/boolean数组
#define OPCODE_CASTORE      0x55    // 存储到char数组
#define OPCODE_SASTORE      0x56    // 存储到short数组

// 栈操作指令 (0x57-0x5f)
#define OPCODE_POP          0x57    // 弹出栈顶值
#define OPCODE_POP2         0x58    // 弹出栈顶两个值
#define OPCODE_DUP          0x59    // 复制栈顶值
#define OPCODE_DUP_X1       0x5a    // 复制栈顶值并插入
#define OPCODE_DUP_X2       0x5b    // 复制栈顶值并插入
#define OPCODE_DUP2         0x5c    // 复制栈顶两个值
#define OPCODE_DUP2_X1      0x5d    // 复制栈顶两个值并插入
#define OPCODE_DUP2_X2      0x5e    // 复制栈顶两个值并插入
#define OPCODE_SWAP         0x5f    // 交换栈顶两个值

// 算术指令 (0x60-0x83)
#define OPCODE_IADD         0x60    // int加法
#define OPCODE_LADD         0x61    // long加法
#define OPCODE_FADD         0x62    // float加法
#define OPCODE_DADD         0x63    // double加法
#define OPCODE_ISUB         0x64    // int减法
#define OPCODE_LSUB         0x65    // long减法
#define OPCODE_FSUB         0x66    // float减法
#define OPCODE_DSUB         0x67    // double减法
#define OPCODE_IMUL         0x68    // int乘法
#define OPCODE_LMUL         0x69    // long乘法
#define OPCODE_FMUL         0x6a    // float乘法
#define OPCODE_DMUL         0x6b    // double乘法
#define OPCODE_IDIV         0x6c    // int除法
#define OPCODE_LDIV         0x6d    // long除法
#define OPCODE_FDIV         0x6e    // float除法
#define OPCODE_DDIV         0x6f    // double除法
#define OPCODE_IREM         0x70    // int取余
#define OPCODE_LREM         0x71    // long取余
#define OPCODE_FREM         0x72    // float取余
#define OPCODE_DREM         0x73    // double取余
#define OPCODE_INEG         0x74    // int取负
#define OPCODE_LNEG         0x75    // long取负
#define OPCODE_FNEG         0x76    // float取负
#define OPCODE_DNEG         0x77    // double取负

// 位运算指令 (0x78-0x83)
#define OPCODE_ISHL         0x78    // int左移
#define OPCODE_LSHL         0x79    // long左移
#define OPCODE_ISHR         0x7a    // int算术右移
#define OPCODE_LSHR         0x7b    // long算术右移
#define OPCODE_IUSHR        0x7c    // int逻辑右移
#define OPCODE_LUSHR        0x7d    // long逻辑右移
#define OPCODE_IAND         0x7e    // int按位与
#define OPCODE_LAND         0x7f    // long按位与
#define OPCODE_IOR          0x80    // int按位或
#define OPCODE_LOR          0x81    // long按位或
#define OPCODE_IXOR         0x82    // int按位异或
#define OPCODE_LXOR         0x83    // long按位异或

// 类型转换指令 (0x84-0x93)
#define OPCODE_IINC         0x84    // int增量
#define OPCODE_I2L          0x85    // int转long
#define OPCODE_I2F          0x86    // int转float
#define OPCODE_I2D          0x87    // int转double
#define OPCODE_L2I          0x88    // long转int
#define OPCODE_L2F          0x89    // long转float
#define OPCODE_L2D          0x8a    // long转double
#define OPCODE_F2I          0x8b    // float转int
#define OPCODE_F2L          0x8c    // float转long
#define OPCODE_F2D          0x8d    // float转double
#define OPCODE_D2I          0x8e    // double转int
#define OPCODE_D2L          0x8f    // double转long
#define OPCODE_D2F          0x90    // double转float
#define OPCODE_I2B          0x91    // int转byte
#define OPCODE_I2C          0x92    // int转char
#define OPCODE_I2S          0x93    // int转short

// 比较指令 (0x94-0xa6)
#define OPCODE_LCMP         0x94    // long比较
#define OPCODE_FCMPL        0x95    // float比较(NaN时返回-1)
#define OPCODE_FCMPG        0x96    // float比较(NaN时返回1)
#define OPCODE_DCMPL        0x97    // double比较(NaN时返回-1)
#define OPCODE_DCMPG        0x98    // double比较(NaN时返回1)
#define OPCODE_IFEQ         0x99    // 如果等于0则跳转
#define OPCODE_IFNE         0x9a    // 如果不等于0则跳转
#define OPCODE_IFLT         0x9b    // 如果小于0则跳转
#define OPCODE_IFGE         0x9c    // 如果大于等于0则跳转
#define OPCODE_IFGT         0x9d    // 如果大于0则跳转
#define OPCODE_IFLE         0x9e    // 如果小于等于0则跳转
#define OPCODE_IF_ICMPEQ    0x9f    // 如果两个int相等则跳转
#define OPCODE_IF_ICMPNE    0xa0    // 如果两个int不等则跳转
#define OPCODE_IF_ICMPLT    0xa1    // 如果int1<int2则跳转
#define OPCODE_IF_ICMPGE    0xa2    // 如果int1>=int2则跳转
#define OPCODE_IF_ICMPGT    0xa3    // 如果int1>int2则跳转
#define OPCODE_IF_ICMPLE    0xa4    // 如果int1<=int2则跳转
#define OPCODE_IF_ACMPEQ    0xa5    // 如果两个引用相等则跳转
#define OPCODE_IF_ACMPNE    0xa6    // 如果两个引用不等则跳转

// 控制流指令 (0xa7-0xb1)
#define OPCODE_GOTO         0xa7    // 无条件跳转
#define OPCODE_JSR          0xa8    // 跳转到子程序
#define OPCODE_RET          0xa9    // 从子程序返回
#define OPCODE_TABLESWITCH  0xaa    // 表格跳转
#define OPCODE_LOOKUPSWITCH 0xab    // 查找跳转
#define OPCODE_IRETURN      0xac    // 返回int
#define OPCODE_LRETURN      0xad    // 返回long
#define OPCODE_FRETURN      0xae    // 返回float
#define OPCODE_DRETURN      0xaf    // 返回double
#define OPCODE_ARETURN      0xb0    // 返回引用
#define OPCODE_RETURN       0xb1    // 返回void

// 字段和方法指令 (0xb2-0xb9)
#define OPCODE_GETSTATIC    0xb2    // 获取静态字段
#define OPCODE_PUTSTATIC    0xb3    // 设置静态字段
#define OPCODE_GETFIELD     0xb4    // 获取实例字段
#define OPCODE_PUTFIELD     0xb5    // 设置实例字段
#define OPCODE_INVOKEVIRTUAL    0xb6    // 调用虚方法
#define OPCODE_INVOKESPECIAL    0xb7    // 调用特殊方法
#define OPCODE_INVOKESTATIC     0xb8    // 调用静态方法
#define OPCODE_INVOKEINTERFACE  0xb9    // 调用接口方法

// 对象和数组指令 (0xbb-0xc3)
#define OPCODE_NEW          0xbb    // 创建对象
#define OPCODE_NEWARRAY     0xbc    // 创建基本类型数组
#define OPCODE_ANEWARRAY    0xbd    // 创建引用类型数组
#define OPCODE_ARRAYLENGTH  0xbe    // 获取数组长度
#define OPCODE_ATHROW       0xbf    // 抛出异常
#define OPCODE_CHECKCAST    0xc0    // 类型检查
#define OPCODE_INSTANCEOF   0xc1    // 实例检查
#define OPCODE_MONITORENTER 0xc2    // 进入监视器
#define OPCODE_MONITOREXIT  0xc3    // 退出监视器

// 扩展指令 (0xc4-0xc9)
#define OPCODE_WIDE         0xc4    // 扩展局部变量索引
#define OPCODE_MULTIANEWARRAY   0xc5    // 创建多维数组
#define OPCODE_IFNULL       0xc6    // 如果为null则跳转
#define OPCODE_IFNONNULL    0xc7    // 如果不为null则跳转
#define OPCODE_GOTO_W       0xc8    // 宽索引无条件跳转
#define OPCODE_JSR_W        0xc9    // 宽索引跳转到子程序

// 数组类型常量
#define T_BOOLEAN   4
#define T_CHAR      5
#define T_FLOAT     6
#define T_DOUBLE    7
#define T_BYTE      8
#define T_SHORT     9
#define T_INT       10
#define T_LONG      11

/**
 * @brief 指令信息结构
 */
typedef struct {
    j2me_opcode_t opcode;       // 指令码
    const char* name;           // 指令名称
    int operand_count;          // 操作数个数
    int stack_effect;           // 对栈的影响 (正数表示压栈，负数表示出栈)
} j2me_instruction_info_t;

/**
 * @brief 获取指令信息
 * @param opcode 指令码
 * @return 指令信息指针，如果指令不存在返回NULL
 */
const j2me_instruction_info_t* j2me_get_instruction_info(j2me_opcode_t opcode);

/**
 * @brief 获取指令名称
 * @param opcode 指令码
 * @return 指令名称字符串
 */
const char* j2me_get_instruction_name(j2me_opcode_t opcode);

/**
 * @brief 计算指令长度 (包括操作数)
 * @param bytecode 字节码指针
 * @param pc 程序计数器
 * @return 指令长度
 */
int j2me_get_instruction_length(const uint8_t* bytecode, uint32_t pc);

#endif // J2ME_BYTECODE_H