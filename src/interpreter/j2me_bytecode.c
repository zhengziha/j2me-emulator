#include "j2me_bytecode.h"
#include <string.h>

/**
 * @file j2me_bytecode.c
 * @brief J2ME字节码指令信息实现
 * 
 * 提供字节码指令的详细信息和工具函数
 */

// 指令信息表
static const j2me_instruction_info_t instruction_table[] = {
    // 常量指令
    {OPCODE_NOP,         "nop",         0,  0},
    {OPCODE_ACONST_NULL, "aconst_null", 0,  1},
    {OPCODE_ICONST_M1,   "iconst_m1",   0,  1},
    {OPCODE_ICONST_0,    "iconst_0",    0,  1},
    {OPCODE_ICONST_1,    "iconst_1",    0,  1},
    {OPCODE_ICONST_2,    "iconst_2",    0,  1},
    {OPCODE_ICONST_3,    "iconst_3",    0,  1},
    {OPCODE_ICONST_4,    "iconst_4",    0,  1},
    {OPCODE_ICONST_5,    "iconst_5",    0,  1},
    {OPCODE_LCONST_0,    "lconst_0",    0,  2},
    {OPCODE_LCONST_1,    "lconst_1",    0,  2},
    {OPCODE_FCONST_0,    "fconst_0",    0,  1},
    {OPCODE_FCONST_1,    "fconst_1",    0,  1},
    {OPCODE_FCONST_2,    "fconst_2",    0,  1},
    {OPCODE_DCONST_0,    "dconst_0",    0,  2},
    {OPCODE_DCONST_1,    "dconst_1",    0,  2},
    {OPCODE_BIPUSH,      "bipush",      1,  1},
    {OPCODE_SIPUSH,      "sipush",      2,  1},
    {OPCODE_LDC,         "ldc",         1,  1},
    {OPCODE_LDC_W,       "ldc_w",       2,  1},
    {OPCODE_LDC2_W,      "ldc2_w",      2,  2},
    
    // 局部变量加载指令
    {OPCODE_ILOAD,       "iload",       1,  1},
    {OPCODE_LLOAD,       "lload",       1,  2},
    {OPCODE_FLOAD,       "fload",       1,  1},
    {OPCODE_DLOAD,       "dload",       1,  2},
    {OPCODE_ALOAD,       "aload",       1,  1},
    {OPCODE_ILOAD_0,     "iload_0",     0,  1},
    {OPCODE_ILOAD_1,     "iload_1",     0,  1},
    {OPCODE_ILOAD_2,     "iload_2",     0,  1},
    {OPCODE_ILOAD_3,     "iload_3",     0,  1},
    {OPCODE_LLOAD_0,     "lload_0",     0,  2},
    {OPCODE_LLOAD_1,     "lload_1",     0,  2},
    {OPCODE_LLOAD_2,     "lload_2",     0,  2},
    {OPCODE_LLOAD_3,     "lload_3",     0,  2},
    {OPCODE_FLOAD_0,     "fload_0",     0,  1},
    {OPCODE_FLOAD_1,     "fload_1",     0,  1},
    {OPCODE_FLOAD_2,     "fload_2",     0,  1},
    {OPCODE_FLOAD_3,     "fload_3",     0,  1},
    {OPCODE_DLOAD_0,     "dload_0",     0,  2},
    {OPCODE_DLOAD_1,     "dload_1",     0,  2},
    {OPCODE_DLOAD_2,     "dload_2",     0,  2},
    {OPCODE_DLOAD_3,     "dload_3",     0,  2},
    {OPCODE_ALOAD_0,     "aload_0",     0,  1},
    {OPCODE_ALOAD_1,     "aload_1",     0,  1},
    {OPCODE_ALOAD_2,     "aload_2",     0,  1},
    {OPCODE_ALOAD_3,     "aload_3",     0,  1},
    
    // 数组加载指令
    {OPCODE_IALOAD,      "iaload",      0, -1},
    {OPCODE_LALOAD,      "laload",      0,  0},
    {OPCODE_FALOAD,      "faload",      0, -1},
    {OPCODE_DALOAD,      "daload",      0,  0},
    {OPCODE_AALOAD,      "aaload",      0, -1},
    {OPCODE_BALOAD,      "baload",      0, -1},
    {OPCODE_CALOAD,      "caload",      0, -1},
    {OPCODE_SALOAD,      "saload",      0, -1},
    
    // 局部变量存储指令
    {OPCODE_ISTORE,      "istore",      1, -1},
    {OPCODE_LSTORE,      "lstore",      1, -2},
    {OPCODE_FSTORE,      "fstore",      1, -1},
    {OPCODE_DSTORE,      "dstore",      1, -2},
    {OPCODE_ASTORE,      "astore",      1, -1},
    {OPCODE_ISTORE_0,    "istore_0",    0, -1},
    {OPCODE_ISTORE_1,    "istore_1",    0, -1},
    {OPCODE_ISTORE_2,    "istore_2",    0, -1},
    {OPCODE_ISTORE_3,    "istore_3",    0, -1},
    {OPCODE_LSTORE_0,    "lstore_0",    0, -2},
    {OPCODE_LSTORE_1,    "lstore_1",    0, -2},
    {OPCODE_LSTORE_2,    "lstore_2",    0, -2},
    {OPCODE_LSTORE_3,    "lstore_3",    0, -2},
    {OPCODE_FSTORE_0,    "fstore_0",    0, -1},
    {OPCODE_FSTORE_1,    "fstore_1",    0, -1},
    {OPCODE_FSTORE_2,    "fstore_2",    0, -1},
    {OPCODE_FSTORE_3,    "fstore_3",    0, -1},
    {OPCODE_DSTORE_0,    "dstore_0",    0, -2},
    {OPCODE_DSTORE_1,    "dstore_1",    0, -2},
    {OPCODE_DSTORE_2,    "dstore_2",    0, -2},
    {OPCODE_DSTORE_3,    "dstore_3",    0, -2},
    {OPCODE_ASTORE_0,    "astore_0",    0, -1},
    {OPCODE_ASTORE_1,    "astore_1",    0, -1},
    {OPCODE_ASTORE_2,    "astore_2",    0, -1},
    {OPCODE_ASTORE_3,    "astore_3",    0, -1},
    
    // 数组存储指令
    {OPCODE_IASTORE,     "iastore",     0, -3},
    {OPCODE_LASTORE,     "lastore",     0, -4},
    {OPCODE_FASTORE,     "fastore",     0, -3},
    {OPCODE_DASTORE,     "dastore",     0, -4},
    {OPCODE_AASTORE,     "aastore",     0, -3},
    {OPCODE_BASTORE,     "bastore",     0, -3},
    {OPCODE_CASTORE,     "castore",     0, -3},
    {OPCODE_SASTORE,     "sastore",     0, -3},
    
    // 栈操作指令
    {OPCODE_POP,         "pop",         0, -1},
    {OPCODE_POP2,        "pop2",        0, -2},
    {OPCODE_DUP,         "dup",         0,  1},
    {OPCODE_DUP_X1,      "dup_x1",      0,  1},
    {OPCODE_DUP_X2,      "dup_x2",      0,  1},
    {OPCODE_DUP2,        "dup2",        0,  2},
    {OPCODE_DUP2_X1,     "dup2_x1",     0,  2},
    {OPCODE_DUP2_X2,     "dup2_x2",     0,  2},
    {OPCODE_SWAP,        "swap",        0,  0},
    
    // 算术指令
    {OPCODE_IADD,        "iadd",        0, -1},
    {OPCODE_LADD,        "ladd",        0, -2},
    {OPCODE_FADD,        "fadd",        0, -1},
    {OPCODE_DADD,        "dadd",        0, -2},
    {OPCODE_ISUB,        "isub",        0, -1},
    {OPCODE_LSUB,        "lsub",        0, -2},
    {OPCODE_FSUB,        "fsub",        0, -1},
    {OPCODE_DSUB,        "dsub",        0, -2},
    {OPCODE_IMUL,        "imul",        0, -1},
    {OPCODE_LMUL,        "lmul",        0, -2},
    {OPCODE_FMUL,        "fmul",        0, -1},
    {OPCODE_DMUL,        "dmul",        0, -2},
    {OPCODE_IDIV,        "idiv",        0, -1},
    {OPCODE_LDIV,        "ldiv",        0, -2},
    {OPCODE_FDIV,        "fdiv",        0, -1},
    {OPCODE_DDIV,        "ddiv",        0, -2},
    {OPCODE_IREM,        "irem",        0, -1},
    {OPCODE_LREM,        "lrem",        0, -2},
    {OPCODE_FREM,        "frem",        0, -1},
    {OPCODE_DREM,        "drem",        0, -2},
    {OPCODE_INEG,        "ineg",        0,  0},
    {OPCODE_LNEG,        "lneg",        0,  0},
    {OPCODE_FNEG,        "fneg",        0,  0},
    {OPCODE_DNEG,        "dneg",        0,  0},
    
    // 位运算指令
    {OPCODE_ISHL,        "ishl",        0, -1},
    {OPCODE_LSHL,        "lshl",        0, -1},
    {OPCODE_ISHR,        "ishr",        0, -1},
    {OPCODE_LSHR,        "lshr",        0, -1},
    {OPCODE_IUSHR,       "iushr",       0, -1},
    {OPCODE_LUSHR,       "lushr",       0, -1},
    {OPCODE_IAND,        "iand",        0, -1},
    {OPCODE_LAND,        "land",        0, -2},
    {OPCODE_IOR,         "ior",         0, -1},
    {OPCODE_LOR,         "lor",         0, -2},
    {OPCODE_IXOR,        "ixor",        0, -1},
    {OPCODE_LXOR,        "lxor",        0, -2},
    
    // 类型转换指令
    {OPCODE_I2L,         "i2l",         0,  1},
    {OPCODE_I2F,         "i2f",         0,  0},
    {OPCODE_I2D,         "i2d",         0,  1},
    {OPCODE_L2I,         "l2i",         0, -1},
    {OPCODE_L2F,         "l2f",         0, -1},
    {OPCODE_L2D,         "l2d",         0,  0},
    {OPCODE_F2I,         "f2i",         0,  0},
    {OPCODE_F2L,         "f2l",         0,  1},
    {OPCODE_F2D,         "f2d",         0,  1},
    {OPCODE_D2I,         "d2i",         0, -1},
    {OPCODE_D2L,         "d2l",         0,  0},
    {OPCODE_D2F,         "d2f",         0, -1},
    {OPCODE_I2B,         "i2b",         0,  0},
    {OPCODE_I2C,         "i2c",         0,  0},
    {OPCODE_I2S,         "i2s",         0,  0},
    
    // 比较指令
    {OPCODE_LCMP,        "lcmp",        0, -3},
    {OPCODE_FCMPL,       "fcmpl",       0, -1},
    {OPCODE_FCMPG,       "fcmpg",       0, -1},
    {OPCODE_DCMPL,       "dcmpl",       0, -3},
    {OPCODE_DCMPG,       "dcmpg",       0, -3},
    {OPCODE_IFEQ,        "ifeq",        2, -1},
    {OPCODE_IFNE,        "ifne",        2, -1},
    {OPCODE_IFLT,        "iflt",        2, -1},
    {OPCODE_IFGE,        "ifge",        2, -1},
    {OPCODE_IFGT,        "ifgt",        2, -1},
    {OPCODE_IFLE,        "ifle",        2, -1},
    {OPCODE_IF_ICMPEQ,   "if_icmpeq",   2, -2},
    {OPCODE_IF_ICMPNE,   "if_icmpne",   2, -2},
    {OPCODE_IF_ICMPLT,   "if_icmplt",   2, -2},
    {OPCODE_IF_ICMPGE,   "if_icmpge",   2, -2},
    {OPCODE_IF_ICMPGT,   "if_icmpgt",   2, -2},
    {OPCODE_IF_ICMPLE,   "if_icmple",   2, -2},
    {OPCODE_IF_ACMPEQ,   "if_acmpeq",   2, -2},
    {OPCODE_IF_ACMPNE,   "if_acmpne",   2, -2},
    
    // 控制流指令
    {OPCODE_GOTO,        "goto",        2,  0},
    {OPCODE_JSR,         "jsr",         2,  1},
    {OPCODE_RET,         "ret",         1,  0},
    {OPCODE_TABLESWITCH, "tableswitch", -1, -1},  // 变长指令
    {OPCODE_LOOKUPSWITCH,"lookupswitch",-1, -1},  // 变长指令
    {OPCODE_IRETURN,     "ireturn",     0, -1},
    {OPCODE_LRETURN,     "lreturn",     0, -2},
    {OPCODE_FRETURN,     "freturn",     0, -1},
    {OPCODE_DRETURN,     "dreturn",     0, -2},
    {OPCODE_ARETURN,     "areturn",     0, -1},
    {OPCODE_RETURN,      "return",      0,  0},
    
    // 字段和方法指令
    {OPCODE_GETSTATIC,   "getstatic",   2,  1},   // 简化，实际栈效果取决于字段类型
    {OPCODE_PUTSTATIC,   "putstatic",   2, -1},   // 简化，实际栈效果取决于字段类型
    {OPCODE_GETFIELD,    "getfield",    2,  0},   // 简化，实际栈效果取决于字段类型
    {OPCODE_PUTFIELD,    "putfield",    2, -2},   // 简化，实际栈效果取决于字段类型
    {OPCODE_INVOKEVIRTUAL,   "invokevirtual",   2, -1},  // 简化，实际栈效果取决于方法签名
    {OPCODE_INVOKESPECIAL,   "invokespecial",   2, -1},  // 简化，实际栈效果取决于方法签名
    {OPCODE_INVOKESTATIC,    "invokestatic",    2,  0},  // 简化，实际栈效果取决于方法签名
    {OPCODE_INVOKEINTERFACE, "invokeinterface", 4, -1},  // 简化，实际栈效果取决于方法签名
    
    // 对象和数组指令
    {OPCODE_NEW,         "new",         2,  1},
    {OPCODE_NEWARRAY,    "newarray",    1,  0},
    {OPCODE_ANEWARRAY,   "anewarray",   2,  0},
    {OPCODE_ARRAYLENGTH, "arraylength", 0,  0},
    {OPCODE_ATHROW,      "athrow",      0, -1},
    {OPCODE_CHECKCAST,   "checkcast",   2,  0},
    {OPCODE_INSTANCEOF,  "instanceof",  2,  0},
    {OPCODE_MONITORENTER,"monitorenter",0, -1},
    {OPCODE_MONITOREXIT, "monitorexit", 0, -1},
    
    // 扩展指令
    {OPCODE_WIDE,        "wide",        -1,  0},  // 变长指令
    {OPCODE_MULTIANEWARRAY, "multianewarray", 3, -1},  // 简化，实际栈效果取决于维数
    {OPCODE_IFNULL,      "ifnull",      2, -1},
    {OPCODE_IFNONNULL,   "ifnonnull",   2, -1},
    {OPCODE_GOTO_W,      "goto_w",      4,  0},
    {OPCODE_JSR_W,       "jsr_w",       4,  1},
};

#define INSTRUCTION_COUNT (sizeof(instruction_table) / sizeof(instruction_table[0]))

const j2me_instruction_info_t* j2me_get_instruction_info(j2me_opcode_t opcode) {
    for (size_t i = 0; i < INSTRUCTION_COUNT; i++) {
        if (instruction_table[i].opcode == opcode) {
            return &instruction_table[i];
        }
    }
    return NULL;
}

const char* j2me_get_instruction_name(j2me_opcode_t opcode) {
    const j2me_instruction_info_t* info = j2me_get_instruction_info(opcode);
    return info ? info->name : "unknown";
}

int j2me_get_instruction_length(const uint8_t* bytecode, uint32_t pc) {
    if (!bytecode) {
        return 0;
    }
    
    j2me_opcode_t opcode = bytecode[pc];
    const j2me_instruction_info_t* info = j2me_get_instruction_info(opcode);
    
    if (!info) {
        return 1; // 未知指令，假设长度为1
    }
    
    // 处理变长指令
    switch (opcode) {
        case OPCODE_TABLESWITCH: {
            // tableswitch指令的长度计算
            uint32_t aligned_pc = (pc + 4) & ~3; // 4字节对齐
            uint32_t low = (bytecode[aligned_pc] << 24) | (bytecode[aligned_pc+1] << 16) |
                          (bytecode[aligned_pc+2] << 8) | bytecode[aligned_pc+3];
            uint32_t high = (bytecode[aligned_pc+4] << 24) | (bytecode[aligned_pc+5] << 16) |
                           (bytecode[aligned_pc+6] << 8) | bytecode[aligned_pc+7];
            return (aligned_pc - pc) + 12 + (high - low + 1) * 4;
        }
        
        case OPCODE_LOOKUPSWITCH: {
            // lookupswitch指令的长度计算
            uint32_t aligned_pc = (pc + 4) & ~3; // 4字节对齐
            uint32_t npairs = (bytecode[aligned_pc+4] << 24) | (bytecode[aligned_pc+5] << 16) |
                             (bytecode[aligned_pc+6] << 8) | bytecode[aligned_pc+7];
            return (aligned_pc - pc) + 8 + npairs * 8;
        }
        
        case OPCODE_WIDE: {
            // wide指令修饰下一条指令
            j2me_opcode_t next_opcode = bytecode[pc + 1];
            if (next_opcode == OPCODE_IINC) {
                return 6; // wide iinc
            } else {
                return 4; // wide load/store
            }
        }
        
        default:
            return 1 + info->operand_count;
    }
}