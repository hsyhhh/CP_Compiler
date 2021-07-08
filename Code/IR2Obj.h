#ifndef __IR_2_OBJ__
#define __IR_2_OBJ__

#include "InterCode.h"
#define REG_NUM 26
#define FRAME_OFFSET \
    8  //因为函数至少把ra和fp写到内存中，故要讲FrameOffset初始化为8
typedef struct reg_struct {
    char name[3];  //寄存器名称
    char* vname;   //绑定的变量名
    int LRU_count;  //当寄存器满的时候，溢出LRU_count值最大的寄存器
} reg;

typedef struct var_node {
    char* vname;  //变量名
    int reg;      //对应的寄存器
    int offset;   //变量的帧偏移
    struct var_node* next;
} vnode;

reg regs[REG_NUM];
//规定
//0-9是t系列寄存器(和mips标准不同)，10-17为s系列寄存器，18fp帧指针，19sp栈指针，20ra返回地址，21v0系统调用和返回值，22-25为a系列寄存器(仅用于函数参数)
// mips还有寄存器v1(表达式求值和函数结果)。
// zero(常数0)，at(汇编器保留)，k0,k1(中断处理)，此处不用
//使用局部寄存器分配算法，每次遇到跳转时把所有正在使用的寄存器写入内存。
int FrameOffset;     //当前帧偏移
vnode* FuncVarList;  //函数的变量链表
vnode* VarListTail;  //链表尾节点
int ArgCount;        //实参计数

void Init();  //寄存器、变量表、帧偏移、参数计数初始化
void regNamer(char name[], char* str);  //给寄存器赋名称
void DelVarList();                      //删除整个变量链表
void addVar(vnode* v);                  //添加v到变量链表尾部
void PrintVarList();  //打印出变量表中的变量名，测试用
void resetStRegs();   // reset所有t系列和s系列寄存器
int allocReg(Operand op, FILE* file);  //给变量分配寄存器
int GetEmptyReg(
    FILE*
        file);  //在t系列和s系列寄存器中找空余寄存器，没有则选LRU_count值最大的溢出，溢出时将写入内存的代码写入fp
void PrintReg(int index, FILE* file);  //打印寄存器名
void printAllCode(char* fname);        //向fname中打印所有目标代码
void printObjCode(InterCode c,
                  FILE* file);  //将一份中间代码转成目标代码写到fp中
void StoreVarList(FILE* file);  //将变量列表中的变量存储到内存中
void ReleaseRegs();             //释放变量占用的寄存器
//翻译函数
void TransLable(InterCode ic, FILE* file);
void TransAssign(InterCode ic, FILE* file);
void TransBinaryAssign(InterCode ic, FILE* file);
void TransCall(InterCode ic, FILE* file);
void TransFunc(InterCode ic, FILE* file);
void TransRightAt(InterCode ic, FILE* file);
void TransGOTO(InterCode ic, FILE* file);
void TransIFGOTO(InterCode ic, FILE* file);
void TransReturn(InterCode ic, FILE* file);
void TransPushArg(InterCode ic, FILE* file);
void TransRead(InterCode ic, FILE* file);
void TransWrite(InterCode ic, FILE* file);
//计算函数最大使用的内存空间
int MaxStoreSize(InterCode ic);
//将常数和地址放到变量中
Operand Const2Tmpvar(Operand cst, FILE* file);
Operand Addr2Tmpvar(Operand addr, FILE* file);

#endif
