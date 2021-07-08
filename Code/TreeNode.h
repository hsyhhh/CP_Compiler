#ifndef __TREE_NODE__
#define __TREE_NODE__

#define MAX_NAME 32
#define MAX_VALUE 32
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct TreeNode {
    int row;
    char name[MAX_NAME];  //文法中的名字
    char value
        [MAX_VALUE];  //终结符的value是其yytext，用于区分变量名以及确定常数值
    struct TreeNode* children;
    struct TreeNode* next;
} Node;

typedef Node* PtrToNode;

PtrToNode NewNode(
    char* name,
    char* value);  // name是节点在语法分析中的名称，value用于语义分析

char* Mystrcpy(char* des, char* src, int maxsize);  //有长度限制的strcpy

void addChild(PtrToNode f, PtrToNode c);  // sibling式树。插入子节点

void printTree(PtrToNode r, int count);  //打印整棵树

#endif
