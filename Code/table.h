#ifndef __PARSER_TABLE__
#define __PARSER_TABLE__

#include "InterCode.h"
#include "TreeNode.h"
#include "stdbool.h"
#include "stddef.h"
#include "stdio.h"
#include "string.h"

#define TABLE_SIZE 0x3fff
#define FROM_GLOBAL 1
#define FROM_COMPOUND 2
#define FROM_STRUCT 3
#define FROM_PARAM 4
// insert_into_table
#define INSERT_SUCCESS 0
#define REDEFINE_ERROR 1
#define DEF_MISMATCH_DEC 2
#define DEC_MISMATCH_DEF 3
#define DEC_MISMATCH_DEC 4

typedef struct Type_ *Type;
typedef struct Structure_ *Structure;
typedef struct VarType_ *VarType;
typedef struct FuncType_ *FuncType;

struct Type_ {
    enum {
        BASIC,
        ARRAY,
        STRUCTURE,
        CONSTANT
    } type;  // BASIC可以做左值，CONSTANT不能做左值
    union {
        // 基本类型信息
        enum { INT_TYPE, FLOAT_TYPE } basic;
        // 数组类型信息: 元素类型与数组大小
        struct {
            Type element;
            int size;
        } array;
        // 结构体类型信息
        Structure structure;
    } type_info;
};

struct Structure_ {
    char *name;
    VarType varList;
};

struct VarType_ {
    char *name;
    Type type;
    // open hashing
    VarType next;        // 哈希表同一表项中所构成的链表
    VarType next_field;  // 结构体中连接所有成员变量的链表
};

struct FuncType_ {
    char *name;      //函数名
    bool isDefined;  //函数可只声明不定义
    int row;  //函数最初被识别时在编辑器中的行，可能是声明也可能是定义
    Type returnType;
    VarType param;  // 参数列表
    FuncType next;  // 哈希表同一表项中所构成的链表
};

void initHashTable(void);
void printVarTable(void);
void printFuncTable(void);

unsigned int hash_pjw(char *name);
int insertVar(VarType vl);
int insertFunc(FuncType f);
void insertParam(FuncType f);

bool isTypeEqual(Type t1, Type t2);
bool isParamEqual(VarType v1, VarType v2);
VarType findSymbol(char *name);
FuncType findFunc(char *name);

void Program(Node *n);
void ExtDefList(Node *n);
void ExtDef(Node *n);
void ExtDecList(Node *n, Type t);
Type Specifier(Node *n);
Type StructSpecifier(Node *n);
VarType DefList(Node *n, int src_type);
VarType Def(Node *n, int src_type);
VarType DecList(Node *n, Type type, int src_type);
VarType Dec(Node *n, Type type, int src_type);
FuncType FunDec(Node *n, Type return_type);
VarType VarList(Node *n);
VarType ParamDec(Node *n);
VarType VarDec(Node *n, Type type, int src_type);
void CompSt(Node *n, Type return_type);
void StmtList(Node *n, Type return_type);
void Stmt(Node *n, Type return_type);
Type Exp(Node *n, Operand place, Operand offset);
Type BinaryExp(Node *left, Node *op, Node *right, Operand place, Node *father);
Type translate_Cond(Node *n, Operand label_true, Operand label_false);
bool Args(Node *n, VarType v, Operand arg_list);

int getArraySize(Type t);
int getTypeSize(Type t);
char *Type2String(Type t);
void printParam(VarType v);
void printArgs(Node *n);

#endif