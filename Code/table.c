#include "table.h"

VarType varTable[TABLE_SIZE] = {0};
FuncType funcTable[TABLE_SIZE] = {0};

// hash function by P.J.Weinberger
unsigned int hash_pjw(char *name) {
    unsigned int val = 0, i;
    for (; *name; ++name) {
        val = (val << 2) + *name;
        if ((i = val) & ~TABLE_SIZE) val = (val ^ (i >> 12)) & TABLE_SIZE;
    }
    return val;
}

void printVarTable(void) {
    int i;
    for (i = 0; i < TABLE_SIZE; i++) {
        if (varTable[i]) {
            VarType cur = varTable[i];
            printf("index: %d\n", i);
            while (cur) {
                printf("  name: %s, type: %s\n", cur->name,
                       Type2String(cur->type));
                cur = cur->next;
            }
        }
    }
}

void printFuncTable(void) {
    int i;
    for (i = 0; i < TABLE_SIZE; i++) {
        if (funcTable[i]) {
            FuncType cur = funcTable[i];
            printf("index: %d\n", i);
            while (cur) {
                printf("  name: %s\n", cur->name);
                printf("    isDefined: %d\n", cur->isDefined);
                printf("    row: %d\n", cur->row);
                printf("    returnType: %s\n", Type2String(cur->returnType));

                VarType tmp = cur->param;
                if (!tmp) {
                    printf("    param: empty\n");
                } else {
                    printf("    param:\n");
                    while (tmp) {
                        printf("      name: %s, type: %s\n", tmp->name,
                               Type2String(tmp->type));
                        tmp = tmp->next_field;
                    }
                }
                cur = cur->next;
            }
        }
    }
}

void initHashTable(void) {
    // 需要在符号表中预先添加read和write,因为在中间代码的生成过程中这两个函数和普通函数是不同的
    // int read(void); 返回值为读入的整数值
    FuncType read = (FuncType)malloc(sizeof(struct FuncType_));
    read->name = (char *)malloc(16);
    strcpy(read->name, "read");
    read->isDefined = true;
    read->row = 0;
    read->returnType = (Type)malloc(sizeof(struct Type_));
    read->returnType->type = BASIC;
    read->returnType->type_info.basic = INT_TYPE;
    read->param = NULL;
    read->next = NULL;
    insertFunc(read);

    // int write(int); 参数为要输出的整数值,返回值固定为0
    FuncType write = (FuncType)malloc(sizeof(struct FuncType_));
    write->name = (char *)malloc(16);
    strcpy(write->name, "write");
    write->isDefined = true;
    write->row = 0;
    write->returnType = (Type)malloc(sizeof(struct Type_));
    write->returnType->type = BASIC;
    write->returnType->type_info.basic = INT_TYPE;
    write->param = (VarType)malloc(sizeof(struct VarType_));
    write->param->name = (char *)malloc(16);
    write->param->type = (Type)malloc(sizeof(struct Type_));
    write->param->type->type = BASIC;
    write->param->type->type_info.basic = INT_TYPE;
    strcpy(write->param->name, "write_param");
    write->returnType = (Type)malloc(sizeof(struct Type_));
    write->returnType->type = BASIC;
    write->returnType->type_info.basic = INT_TYPE;
    write->param->next_field = NULL;
    write->param->next = NULL;
    write->next = NULL;
    insertFunc(write);
}

// insertVar: 将变量符号插入符号哈希表
// return: -1: 为空,不插入, 1: 该符号已存在, 0: 插入成功
int insertVar(VarType vl) {
    if (vl->name == NULL) return -1;

    unsigned int index = hash_pjw(vl->name);
    if (varTable[index] == NULL) {
        varTable[index] = vl;
    } else {
        VarType cur = varTable[index], pre;
        while (cur) {
            if (!strcmp(cur->name, vl->name))  //重名变量
                return REDEFINE_ERROR;
            pre = cur;
            cur = cur->next;
        }
        pre->next = vl;
    }
    return INSERT_SUCCESS;  //成功插入
}

// insertFunc: 将函数插入函数哈希表。注意c语言不支持重载
/* return: -1: 不插入(函数名为NULL), 1: 函数已被定义,
2:插入定义时定义和声明不匹配 3:插入声明时定义和声明不匹配
4:重复的声明参数或返回值不相同 0: 插入成功 */
int insertFunc(FuncType f) {
    if (f->name == NULL) return -1;

    unsigned int index = hash_pjw(f->name);
    if (funcTable[index] == NULL) {
        funcTable[index] = f;
        insertParam(f);
    } else {
        FuncType cur = funcTable[index], pre;
        while (cur) {
            if (!strcmp(cur->name, f->name)) {
                // 如果现在要插入一个定义
                if (f->isDefined == true) {
                    // 如果该函数已经存在定义
                    if (cur->isDefined) {
                        return REDEFINE_ERROR;
                        // 如果没有定义,则已经声明,且参数或返回值不同
                    } else if (!isTypeEqual(cur->returnType, f->returnType) ||
                               !isParamEqual(cur->param, f->param)) {
                        return DEF_MISMATCH_DEC;
                    } else {
                        cur->isDefined = true;
                        return INSERT_SUCCESS;
                    }
                    // 如果现在要插入一个声明,注意重复的声明不会报错
                } else {
                    // 如果插入的声明和之前已经存在的参数或返回值不同
                    if (!isTypeEqual(cur->returnType, f->returnType) ||
                        !isParamEqual(cur->param, f->param)) {
                        // 声明和定义不同
                        if (cur->isDefined) return DEC_MISMATCH_DEF;
                        // 声明和声明不同
                        else
                            return DEC_MISMATCH_DEC;
                    } else {
                        return INSERT_SUCCESS;
                    }
                }
            }
            pre = cur;
            cur = cur->next;
        }
        pre->next = f;
        insertParam(f);
    }
    return INSERT_SUCCESS;
}

void insertParam(FuncType f) {
    VarType param = f->param;
    int ret_code = 0;  // 返回状态码
    while (param) {
        ret_code = insertVar(param);
        if (ret_code == REDEFINE_ERROR) {
            // Error type 3
            printf("Error type 3 at line %d: Redefinition of variable '%s'\n",
                   f->row, param->name);
        }
        param = param->next_field;
    }
}

//类型检查
bool isTypeEqual(Type t1, Type t2) {
    if (t1 == NULL || t2 == NULL) {
        printf("ERROR: Type param is NULL in isTypeEqual function!\n");
        return false;
    }
    if ((t1->type == BASIC && t2->type == CONSTANT) ||
        (t1->type == CONSTANT && t2->type == BASIC)) {  //变量和常数
        return t1->type_info.basic == t2->type_info.basic;
    } else if (t1->type != t2->type) {
        return false;
    } else {                      //变量类型相同
        if (t1->type == BASIC) {  //变量和变量
            if (t1->type_info.basic != t2->type_info.basic) return false;
        } else if (t1->type == ARRAY) {
            return isTypeEqual(t1->type_info.array.element,
                               t2->type_info.array.element);
        } else if (t1->type == STRUCTURE) {
            // 存在无名结构体（匿名结构体和其它结构体都不等价,gcc是如此）
            if (t1->type_info.structure->name == NULL ||
                t2->type_info.structure->name == NULL) {
                return false;
            }
            // 结构体不同名（结构体不同名即不等价,gcc是如此）
            if (strcmp(t1->type_info.structure->name,
                       t2->type_info.structure->name))
                return false;
        }
    }
    return true;
}

//检查参数列表是否相同(只检查参数类型，允许参数名不同)
bool isParamEqual(VarType v1, VarType v2) {
    if (v1 == NULL && v2 == NULL) return true;
    if (v1 == NULL || v2 == NULL) return false;
    if (isTypeEqual(v1->type, v2->type))
        return isParamEqual(v1->next_field, v2->next_field);
    else
        return false;
}

//检查变量表中是否存在该变量，不存在则返回null，存在则返回地址
VarType findSymbol(char *name) {
    unsigned int index = hash_pjw(name);
    if (varTable[index] == NULL) return NULL;
    VarType cur = varTable[index];
    while (cur) {
        if (!strcmp(cur->name, name)) return cur;
        cur = cur->next;
    }
    return NULL;
}

//检查函数表中是否存在该函数，不存在则返回null，存在则返回地址
FuncType findFunc(char *name) {
    unsigned int index = hash_pjw(name);
    if (funcTable[index] == NULL) return NULL;
    FuncType cur = funcTable[index];
    while (cur) {
        if (!strcmp(cur->name, name)) return cur;
        cur = cur->next;
    }
    return NULL;
}

void checkFuncIsDefined(void) {
    int i;
    for (i = 0; i < TABLE_SIZE; i++) {
        if (funcTable[i]) {
            FuncType cur = funcTable[i];
            while (cur) {
                if (!cur->isDefined) {
                    printf(
                        "Error type 18 at line %d: Undefined function '%s'\n",
                        cur->row, cur->name);
                }
                cur = cur->next;
            }
        }
    }
}

// 根据创建好的语法树，构建语义分析

// Program -> ExtDefList
void Program(Node *n) { ExtDefList(n->children); }

// ExtDefList -> ExtDef ExtDefList | ε
void ExtDefList(Node *n) {
    if (n) {
        Node *child = n->children;
        if (child) {
            ExtDef(child);
            if (child->next) ExtDefList(child->next);
        }
    }
}

// ExtDef -> Specifier ExtDecList SEMI | Specifier SEMI
// 		   | Specifier FunDec CompSt | Specifier FunDec SEMI
void ExtDef(Node *n) {
    Node *child = n->children;  // child: Specifier
    if (!child) return;
    Type t = Specifier(child);
    if (!strcmp(child->next->name, "ExtDecList")) {  // variable
        ExtDecList(child->next, t);
        return;
    } else if (!strcmp(child->next->name, "SEMI")) {  // STRUCT declare
        return;
    } else if (!strcmp(child->next->name, "FunDec")) {
        FuncType f = FunDec(child->next, t);  // t作为返回值类型传入
        // 此时f已经获得了返回值和参数类型
        if (f) {
            if (!strcmp(child->next->next->name, "CompSt")) {  // 函数定义
                f->isDefined = true;
                int ret_code = insertFunc(f);
                if (ret_code == REDEFINE_ERROR) {
                    // 函数重定义
                    printf(
                        "Error type 4 at line %d: Redefinition of function "
                        "'%s'\n",
                        f->row, f->name);
                } else if (ret_code == DEF_MISMATCH_DEC) {
                    // 当前定义和先前的声明不同
                    printf(
                        "Error type 19 at line %d: Definition of function '%s' "
                        "is different from the previous declaration\n",
                        f->row, f->name);
                } else {
                    // 正确的定义,开始中间代码生成
                    // 插入函数定义的中间代码
                    Operand tmp_op = (Operand)malloc(sizeof(struct Operand_));
                    InterCode tmp_code =
                        (InterCode)malloc(sizeof(struct InterCode_));
                    tmp_op->kind = FUNCTION_OP;
                    tmp_op->u.value = f->name;
                    tmp_code->kind = FUNCTION;
                    tmp_code->u.unary.op = tmp_op;
                    insertInterCode(tmp_code);
                    // 插入函数参数声明的中间代码
                    VarType param = f->param;
                    while (param) {
                        tmp_op = (Operand)malloc(sizeof(struct Operand_));
                        tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
                        tmp_op->kind = VAR;
                        tmp_op->u.value = param->name;
                        tmp_code->kind = PARAM;
                        tmp_code->u.unary.op = tmp_op;
                        insertInterCode(tmp_code);

                        param = param->next_field;
                    }
                }
                CompSt(child->next->next, t);
                return;
            } else if (!strcmp(child->next->next->name, "SEMI")) {  // 函数声明
                f->isDefined = false;
                // 此时已知f的返回类型、参数类型且是声明
                int ret_code = insertFunc(f);
                if (ret_code == 3) {
                    // 当前声明和先前的定义不同
                    printf(
                        "Error type 19 at line %d: Declaration of function "
                        "'%s' is different from the previous definition\n",
                        f->row, f->name);
                } else if (ret_code == 4) {
                    // 当前声明和先前的声明不同
                    printf(
                        "Error type 19 at line %d: Declaration of function "
                        "'%s' is different from the previous declaration\n",
                        f->row, f->name);
                }
                return;
            }
        }
    }
    printf("[Internal Error] error in semantic analysis in ExtDef()\n");
    return;
}

// ExtDecList -> VarDec | VarDec COMMA ExtDecList
void ExtDecList(Node *n, Type t) {
    Node *child = n->children;
    if (!child) return;
    VarType tmp = VarDec(child, t, FROM_GLOBAL);
    // VarDec返回单个变量或数组变量,这里需要为数组变量分配内存空间,生成中间代码
    if (tmp && tmp->type->type == ARRAY) {
        Operand tmp_op = (Operand)malloc(sizeof(struct Operand_));
        InterCode tmp_code = (InterCode)malloc(sizeof(struct InterCode_));

        // DEC 为数组开辟空间 字节为单位
        // DEC t1 [size]
        tmp_op->kind = TMP_VAR;
        tmp_op->u.var_no = varNo++;
        tmp_code->kind = DEC;
        tmp_code->u.dec.op = tmp_op;
        tmp_code->u.dec.size = getTypeSize(tmp->type);
        insertInterCode(tmp_code);

        // RIGHTAT 将数组的地址赋给原本的变量名
        // array := &t1
        Operand array_op = (Operand)malloc(sizeof(struct Operand_));
        array_op->kind = VAR;
        array_op->u.value = tmp->name;
        tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
        tmp_code->kind = RIGHTAT;
        tmp_code->u.assign.left = array_op;
        tmp_code->u.assign.right = tmp_op;
        insertInterCode(tmp_code);
    }
    if (child->next) {
        if (!strcmp(child->next->name, "COMMA") &&
            !strcmp(child->next->next->name, "ExtDecList")) {
            ExtDecList(child->next->next, t);
        }
    } else {
        return;
    }
}

// Specifier -> TYPE | StructSpecifier
Type Specifier(Node *n) {
    Node *child = n->children;
    if (!child) return NULL;
    if (!strcmp(child->name, "TYPE")) {
        Type t = (Type)malloc(sizeof(struct Type_));
        if (!t) return NULL;
        t->type = BASIC;
        if (!strcmp(child->value, "int")) {
            t->type_info.basic = INT_TYPE;
        } else if (!strcmp(child->value, "float")) {
            t->type_info.basic = FLOAT_TYPE;
        }
        return t;
    } else if (!strcmp(child->name, "StructSpecifier")) {
        Type t = StructSpecifier(child);
        return t;
    }
    return NULL;
}

// StructSpecifier -> STRUCT OptTag LC DefList RC
// 					| STRUCT Tag
// OptTag和Tag不需要定义函数，直接在StructSpecifier中就解决了
Type StructSpecifier(Node *n) {
    Node *child = n->children;
    if (!strcmp(child->name, "STRUCT")) {
        if (!strcmp(child->next->name, "OptTag")) {  //首次定义结构体变量
            Type t = (Type)malloc(sizeof(struct Type_));
            t->type = STRUCTURE;
            t->type_info.structure =
                (Structure)malloc(sizeof(struct Structure_));
            t->type_info.structure->name = NULL;
            t->type_info.structure->varList = NULL;

            Node *OptTag_node = child->next;
            // 如果OptTag有孩子,说明是有名结构定义(OptTag->ID)
            if (OptTag_node->children) {
                t->type_info.structure->name = (char *)malloc(
                    sizeof(OptTag_node->children
                               ->value));  // ID是终结符，value就是ID名
                strcpy(t->type_info.structure->name,
                       OptTag_node->children->value);
            }  // 否则OptTag无孩子(OpttTag->e),说明是无名结构定义,什么也不做
            Node *tmp = OptTag_node->next;
            if (!strcmp(tmp->name,
                        "LC")) {  //如果是首次定义结构体，定义内部结构
                Node *DefList_node = tmp->next;
                if (!strcmp(DefList_node->name, "DefList")) {
                    VarType v = DefList(DefList_node, FROM_STRUCT);
                    t->type_info.structure->varList = v;
                    // 无名结构体,无需插入至哈希表
                    if (t->type_info.structure->name == NULL) {
                        return t;
                    }
                    // 将定义好的结构体本身插入哈希表
                    // 结构体内部的变量不在此处插入
                    VarType tmp = (VarType)malloc(sizeof(struct VarType_));
                    tmp->type = t;
                    tmp->name =
                        (char *)malloc(sizeof(t->type_info.structure->name));
                    strcpy(tmp->name, t->type_info.structure->name);
                    int ret_code = insertVar(tmp);
                    // 结构体的名字和定义过的结构体或变量的名字重复
                    if (ret_code == 1) {
                        printf(
                            "Error type 16 at line %d: The name of struct '%s' "
                            "duplicates name of another variable or struct",
                            DefList_node->row, t->type_info.structure->name);
                        return NULL;
                    }
                    return t;
                }
            }
            // 根据已有的结构定义新的结构变量
        } else if (!strcmp(child->next->name, "Tag")) {
            if (!child->next->children) return NULL;
            // 查找变量符号表中是否已经定义了该结构
            VarType tmp = findSymbol(child->next->children->value);
            if (!tmp || tmp->type->type != STRUCTURE ||
                strcmp(tmp->name, tmp->type->type_info.structure->name)) {
                // 结构体未定义
                printf("Error type 17 at line %d: Undefined struct type '%s'\n",
                       child->next->row, child->next->children->value);
                return NULL;
            } else {
                return tmp->type;
            }
        }
    }
    printf(
        "[Internal Error] error in semantic analysis in StructSpecifier()\n");
    return NULL;
}

// DefList-> Def DefList
//			| e
//变量定义和声明：src_type是该非终结符被定义的地方。
VarType DefList(
    Node *n,
    int src_type) {  // Struct中的DefList不允许声明时赋值，CompoundStatement中的允许赋值
    Node *child = n->children;
    if (child) {  //存在声明
        VarType v;
        v = Def(child, src_type);
        child = child->next;  // DefList
        VarType tmp = v;
        if (v != NULL) {  // Def可能定义了多个变量
            while (tmp->next_field != NULL) tmp = tmp->next_field;
            tmp->next_field = DefList(child, src_type);
        } else
            v = DefList(child, src_type);
        return v;
    } else
        return NULL;
}

// Def -> Specifier DecList SEMI
VarType Def(Node *n, int src_type) {
    Node *child = n->children;
    VarType v;
    Type type = Specifier(child);
    child = child->next;
    v = DecList(child, type, src_type);
    return v;
}

// DecList -> Dec
//		   | Dec COMMA DecList
VarType DecList(Node *n, Type type, int src_type) {
    Node *child = n->children;
    VarType v = Dec(child, type, src_type);
    if (child->next != NULL) {
        child = child->next->next;  // DecList
        if (v != NULL) {
            VarType tmp = v;
            while (tmp->next_field != NULL) tmp = tmp->next_field;
            tmp->next_field = DecList(child, type, src_type);
        } else
            v = DecList(child, type, src_type);
    }
    return v;
}

// Dec -> VarDec
//	   | VarDec ASSIGNOP Exp
// FROM_STRUCT:不允许使用Dec -> VarDec ASSIGNOP
// Exp的规则，FROM_COUMPOUND:两个规则都可以
VarType Dec(Node *n, Type type, int src_type) {
    Node *child = n->children;
    VarType v = VarDec(child, type, src_type);
    if (!v) return NULL;
    // 为局部数组变量申请空间
    if (v->type->type == ARRAY && src_type == FROM_COMPOUND) {
        Operand tmp_op = (Operand)malloc(sizeof(struct Operand_));
        InterCode tmp_code = (InterCode)malloc(sizeof(struct InterCode_));

        // DEC t1 [size]
        tmp_op->kind = TMP_VAR;
        tmp_op->u.var_no = varNo++;
        tmp_code->kind = DEC;
        tmp_code->u.dec.op = tmp_op;
        tmp_code->u.dec.size = getTypeSize(v->type);
        insertInterCode(tmp_code);

        // array := &t1
        Operand array_op = (Operand)malloc(sizeof(struct Operand_));
        array_op->kind = VAR;
        array_op->u.value = v->name;
        tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
        tmp_code->kind = RIGHTAT;
        tmp_code->u.assign.left = array_op;
        tmp_code->u.assign.right = tmp_op;
        insertInterCode(tmp_code);
    }
    // 局部变量赋初值
    if (child->next != NULL) {
        if (src_type == FROM_STRUCT) {
            printf(
                "Error type 15 at line %d: Variable initialized in struct '%s' "
                "is not allowed\n",
                child->row, v->name);
            return v;
        }
        Operand place = (Operand)malloc(sizeof(struct Operand_));
        place->kind = VAR;
        place->u.value = v->name;
        if (child->next) child = child->next->next;  // Exp
        // 最后会把值赋给place
        Type t = Exp(child, place, NULL);
        if (t && type && !isTypeEqual(type, t)) {
            printf("Error type 5 at line %d: The type mismatched\n",
                   child->row);
        }
        // 如果发现place被Exp改变了,则需要手动赋值
        // 比如 i = 1, 进入Exp后place会变成CONSTANT_OP类型
        if (place->kind != VAR || strcmp(place->u.value, v->name)) {
            Operand tmp_op = (Operand)malloc(sizeof(struct Operand_));
            tmp_op->kind = VAR;
            tmp_op->u.value = v->name;

            InterCode tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
            tmp_code->kind = ASSIGN;
            tmp_code->u.assign.left = tmp_op;
            tmp_code->u.assign.right = place;
            insertInterCode(tmp_code);
        }
    }
    return v;
}

// FunDec-> ID LP VarList RP
//		|  ID LP RP
//		|  error RP (语法错误，不在此处处理)
FuncType FunDec(Node *n, Type return_type) {
    Node *child = n->children;
    FuncType func = (FuncType)malloc(sizeof(struct FuncType_));
    func->isDefined = false;
    func->name = (char *)malloc(sizeof(child->value));
    strcpy(func->name, child->value);
    func->returnType = return_type;
    func->row = child->row;
    Node *child3 = child->next->next;  //右括号或VarList
    if (!strcmp(child3->name, "VarList"))
        func->param = VarList(child3);
    else
        func->param = NULL;
    return func;
}

// VarList -> ParamDec COMMA VarList
//		   | ParamDec
VarType VarList(Node *n) {  //返回参数列表
    Node *child = n->children;
    VarType v;
    v = ParamDec(child);
    if (child->next != NULL) {
        child = child->next->next;  // VarList
        VarType tmp = v;
        if (v != NULL) {
            while (tmp->next_field != NULL) tmp = tmp->next_field;
            tmp->next_field = VarList(child);
        } else
            v = VarList(child);
    }
    return v;
}

// ParamDec -> Specifier VarDec
VarType ParamDec(Node *n) {
    Node *child = n->children;
    Type type = Specifier(child);
    VarType v = VarDec(child->next, type, FROM_PARAM);
    return v;
}

// VarDec -> ID
// 		  | VarDec LB INT RB
VarType VarDec(Node *n, Type type, int src_type) {  //将定义的变量插入变量表
    Node *child = n->children;
    if (!child) return NULL;
    if (!strcmp(child->name, "ID")) {
        VarType v = (VarType)malloc(sizeof(struct VarType_));
        v->name = (char *)malloc(sizeof(child->value));  //变量名
        strcpy(v->name, child->value);
        v->type = type;
        v->next = NULL;
        v->next_field = NULL;
        if (src_type == FROM_PARAM)
            return v;  //在函数参数列表中定义的变量，不需要插入变量表
        if (insertVar(v) == REDEFINE_ERROR) {
            if (src_type == FROM_GLOBAL ||
                src_type == FROM_COMPOUND)  //暂时不区分全局变量和域变量
                printf(
                    "Error type 3 at line %d: Redefined global variable'%s'\n",
                    child->row, v->name);
            else  //来自结构体
                printf(
                    "Error type 15 at line %d: Redefined field variable '%s'\n",
                    child->row, v->name);
            return NULL;
        }
        // ???
        if (src_type == FROM_COMPOUND && type->type == STRUCTURE) {
            // if (type->type == STRUCTURE) {
            Operand tmp_op = (Operand)malloc(sizeof(struct Operand_));
            InterCode tmp_code = (InterCode)malloc(sizeof(struct InterCode_));

            tmp_op->kind = TMP_VAR;
            tmp_op->u.var_no = varNo++;
            tmp_code->kind = DEC;
            tmp_code->u.dec.op = tmp_op;
            tmp_code->u.dec.size = getTypeSize(type);
            insertInterCode(tmp_code);

            Operand struct_op = (Operand)malloc(sizeof(struct Operand_));
            struct_op->kind = VAR;
            struct_op->u.value = child->value;

            tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
            tmp_code->kind = RIGHTAT;
            tmp_code->u.assign.left = struct_op;
            tmp_code->u.assign.right = tmp_op;
            insertInterCode(tmp_code);
        }
        return v;
    } else if (!strcmp(child->name, "VarDec")) {  //数组
        Type NewType = (Type)malloc(sizeof(struct Type_));
        NewType->type = ARRAY;
        NewType->type_info.array.size = atoi(child->next->next->value);
        NewType->type_info.array.element = type;
        VarType v = VarDec(child, NewType, src_type);
        if (v == NULL) return NULL;
        return v;
    } else {
        printf("CODE ERROR in VarDec function\n");
        return NULL;
    }
}

// CompSt -> LC DefList StmtList RC
// 这个CoumpSt仅用于函数体，不用于结构体，故有返回值
void CompSt(Node *n, Type return_type) {
    Node *child = n->children->next;  // DefList
    DefList(child, FROM_COMPOUND);
    child = child->next;  // StmtList
    StmtList(child, return_type);
}

// StmtList -> Stmt StmtList
//			| e
void StmtList(Node *n, Type return_type) {
    Node *child = n->children;
    if (child) {
        Stmt(child, return_type);
        child = child->next;
        StmtList(child, return_type);
    }
    return;
}

// Stmt -> Exp SEMI
//     |  CompSt
//	   |  RETURN Exp SEMI
//	   |  IF LP Exp RP Stmt
//	   |  IF LP Exp RP Stmt ELSE Stmt
//	   |  WHILE LP Exp RP Stmt
void Stmt(Node *n, Type return_type) {
    Node *child = n->children;
    if (!child) return;
    if (!strcmp(child->name, "Exp")) {
        Exp(child, NULL, NULL);
        return;
    } else if (!strcmp(child->name, "CompSt")) {
        CompSt(child, return_type);
        return;
    } else if (!strcmp(child->name, "RETURN")) {
        Operand return_op = (Operand)malloc(sizeof(struct Operand_));
        return_op->kind = TMP_VAR;
        return_op->u.var_no = varNo++;

        child = child->next;
        Type t = Exp(child, return_op, NULL);
        if (return_type == NULL || t == NULL) return;
        if (!isTypeEqual(return_type, t)) {
            printf("Error type 8 at line %d: Return type mismatched\n",
                   child->row);
            return;
        }
        // RETURN的中间代码
        InterCode tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
        tmp_code->kind = RETURN_KIND;
        tmp_code->u.unary.op = return_op;
        insertInterCode(tmp_code);
        return;
    } else if (!strcmp(child->name, "IF")) {
        child = child->next->next;  // Exp
        Operand label1_op = (Operand)malloc(sizeof(struct Operand_));
        Operand label2_op = (Operand)malloc(sizeof(struct Operand_));
        label1_op->kind = LABEL_OP;
        label2_op->kind = LABEL_OP;
        label1_op->u.var_no = labelNo++;
        label2_op->u.var_no = labelNo++;

        Type t = translate_Cond(child, label1_op, label2_op);
        // t==NULL的话说明在Exp函数中已经报错了
        if (t != NULL && !((t->type == BASIC || t->type == CONSTANT) &&
                           t->type_info.basic == INT_TYPE)) {
            printf(
                "Error at line %d: type %s is not allowed for if condition\n",
                child->row, Type2String(t));
        }

        // Label label1
        InterCode tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
        tmp_code->kind = LABEL;
        tmp_code->u.unary.op = label1_op;
        insertInterCode(tmp_code);

        Node *Stmt1_node = child->next->next;
        Stmt(Stmt1_node, return_type);

        // 存在else部分
        if (Stmt1_node->next) {
            // GOTO label3
            Operand label3_op = (Operand)malloc(sizeof(struct Operand_));
            label3_op->kind = LABEL_OP;
            label3_op->u.var_no = labelNo++;
            tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
            tmp_code->kind = GOTO;
            tmp_code->u.unary.op = label3_op;
            insertInterCode(tmp_code);

            // LABEL label2
            tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
            tmp_code->kind = LABEL;
            tmp_code->u.unary.op = label2_op;
            insertInterCode(tmp_code);

            Node *Stmt2_node = Stmt1_node->next->next;
            Stmt(Stmt2_node, return_type);

            // LABEL label3
            tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
            tmp_code->kind = LABEL;
            tmp_code->u.unary.op = label3_op;
            insertInterCode(tmp_code);
            return;
            // 不存在else部分
        } else {
            // Label label2
            tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
            tmp_code->kind = LABEL;
            tmp_code->u.unary.op = label2_op;
            insertInterCode(tmp_code);
            return;
        }
    } else if (!strcmp(child->name, "WHILE")) {
        Operand label1_op = (Operand)malloc(sizeof(struct Operand_));
        Operand label2_op = (Operand)malloc(sizeof(struct Operand_));
        Operand label3_op = (Operand)malloc(sizeof(struct Operand_));
        label1_op->kind = LABEL_OP;
        label2_op->kind = LABEL_OP;
        label3_op->kind = LABEL_OP;
        label1_op->u.var_no = labelNo++;
        label2_op->u.var_no = labelNo++;
        label3_op->u.var_no = labelNo++;

        // LABEL label1
        InterCode tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
        tmp_code->kind = LABEL;
        tmp_code->u.unary.op = label1_op;
        insertInterCode(tmp_code);

        Node *Exp_node = child->next->next;
        Type t = translate_Cond(Exp_node, label2_op, label3_op);
        // t==NULL的话说明在Exp函数中已经报错了
        if (t != NULL && !((t->type == BASIC || t->type == CONSTANT) &&
                           t->type_info.basic == INT_TYPE)) {
            printf(
                "Error at line %d: type %s is not allowed for while "
                "condition\n",
                child->row, Type2String(t));
        }

        // LABEL label2
        tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
        tmp_code->kind = LABEL;
        tmp_code->u.unary.op = label2_op;
        insertInterCode(tmp_code);

        Node *Stmt_node = Exp_node->next->next;
        Stmt(Stmt_node, return_type);

        // GOTO label1
        tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
        tmp_code->kind = GOTO;
        tmp_code->u.unary.op = label1_op;
        insertInterCode(tmp_code);

        // LABEL label3
        tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
        tmp_code->kind = LABEL;
        tmp_code->u.unary.op = label3_op;
        insertInterCode(tmp_code);

        return;
    } else {
        printf("CODE ERROR: in Stmt function\n");
        return;
    }
}

/*Exp ->Exp ASSIGNOP Exp
    |	Exp AND Exp
    | 	Exp OR Exp
    |	Exp RELOP Exp
    |	Exp PLUS Exp
    |	Exp MINUS Exp
    |	Exp MUL Exp
    |	Exp DIV Exp
    | 	LP Exp RP
    |	MINUS Exp
    |	NOT Exp
    |	ID LP Args RP
    |	ID LP RP
    |	Exp LB Exp RB
    |	Exp DOT ID
    |	ID
    |	INT
    |	FLOAT
    ;*/
//返回值要说明Exp的值的类型
Type Exp(Node *n, Operand place, Operand offset) {
    Node *child = n->children;
    if (!child) return NULL;
    if (!strcmp(child->name, "Exp")) {
        // array
        if (!strcmp(child->next->name, "LB")) {
            Operand op1 = (Operand)malloc(sizeof(struct Operand_));
            op1->kind = TMP_VAR;
            op1->u.var_no = varNo++;
            int flag = 0;

            if (!offset) {
                flag = 1;
                offset = (Operand)malloc(sizeof(struct Operand_));
                offset->kind = TMP_VAR;
                offset->u.var_no = varNo++;
            }
            Type t1 = Exp(child, op1, offset);

            if (t1 == NULL) return NULL;
            if (t1->type != ARRAY) {
                printf("Error type 10 at line %d: '%s' must be an array\n",
                       child->row, child->children->value);
                return NULL;
            }

            child = child->next->next;  //第二个exp
            Operand op2 = (Operand)malloc(sizeof(struct Operand_));
            op2->kind = TMP_VAR;
            op2->u.var_no = varNo++;
            Type t2 = Exp(child, op2, NULL);

            if (t2 == NULL) return NULL;
            if (!((t2->type == BASIC || t2->type == CONSTANT) &&
                  t2->type_info.basic == INT_TYPE)) {
                printf("Error type 12 at line %d: Operands type mistaken\n",
                       child->row);
                return NULL;
            }

            int array_size = getArraySize(t1);
            // #array_size
            Operand size_op = (Operand)malloc(sizeof(struct Operand_));
            size_op->kind = CONSTANT_OP;
            size_op->u.value = (char *)malloc(20);
            sprintf(size_op->u.value, "%d", array_size);
            // tmp
            Operand tmp_op = (Operand)malloc(sizeof(struct Operand_));
            tmp_op->kind = TMP_VAR;
            tmp_op->u.var_no = varNo++;
            // tmp := #array_size * op2
            InterCode tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
            tmp_code->kind = MUL_KIND;
            tmp_code->u.binop.result = tmp_op;
            tmp_code->u.binop.op1 = size_op;
            tmp_code->u.binop.op2 = op2;
            insertInterCode(tmp_code);

            tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
            tmp_code->kind = ADD_KIND;
            tmp_code->u.binop.op1 = op1;
            tmp_code->u.binop.op2 = tmp_op;

            Operand res_op = (Operand)malloc(sizeof(struct Operand_));
            res_op->kind = TMP_VAR;
            res_op->u.var_no = varNo++;
            if (flag) {
                place->kind = TMP_VAR_ADDRESS;
                place->u.var = res_op;
            } else {
                place->kind = TMP_VAR;
                place->u.var_no = res_op->u.var_no;
            }
            tmp_code->u.binop.result = res_op;

            insertInterCode(tmp_code);

            return t1->type_info.array.element;
        }
        // struct
        else if (!strcmp(child->next->name, "DOT")) {
            Operand op1 = (Operand)malloc(sizeof(struct Operand_));
            op1->kind = TMP_VAR;
            op1->u.var_no = varNo++;

            Operand no_use = (Operand)malloc(sizeof(struct Operand_));
            Type t1 = Exp(child, op1, no_use);
            if (t1 == NULL) return NULL;
            if (t1->type != STRUCTURE) {
                printf("Error type 13 at line %d: Illegal use of '.'\n",
                       child->row);
                return NULL;
            }
            VarType v = t1->type_info.structure->varList;
            child = child->next->next;
            int offset = 0;  // 结构体变量的偏移量
            while (v != NULL) {
                if (!strcmp(v->name, child->value)) {
                    InterCode tmp_code =
                        (InterCode)malloc(sizeof(struct InterCode_));
                    tmp_code->kind = ADD_KIND;

                    Operand res_op = (Operand)malloc(sizeof(struct Operand_));
                    res_op->kind = TMP_VAR;
                    res_op->u.var_no = varNo++;

                    Operand offset_op =
                        (Operand)malloc(sizeof(struct Operand_));
                    offset_op->kind = CONSTANT_OP;
                    offset_op->u.value = (char *)malloc(20);
                    sprintf(offset_op->u.value, "%d", offset);

                    tmp_code->u.binop.op1 = op1;
                    tmp_code->u.binop.op2 = offset_op;

                    // 如果是基础类型,则返回临时变量,对应待取的变量的地址
                    if (v->type->type == BASIC) {
                        place->kind = TMP_VAR_ADDRESS;
                        place->u.var = res_op;
                        tmp_code->u.binop.result = res_op;
                        // 否则,还没到达要取得的变量的地址,继续递归
                    } else {
                        tmp_code->u.binop.result = place;
                    }

                    insertInterCode(tmp_code);
                    return v->type;
                }
                offset += getTypeSize(v->type);
                v = v->next_field;
            }
            printf("Error type 14 at line %d: Un-existed field '%s'\n",
                   child->row, child->value);
            return NULL;
        }
        // binary exp
        else {
            return BinaryExp(child, child->next, child->next->next, place, n);
        }
    } else if (!strcmp(child->name, "LP")) {
        return Exp(child->next, place, NULL);
    } else if (!strcmp(child->name, "MINUS")) {
        Node *Exp_node = child->next;

        Operand tmp_op = (Operand)malloc(sizeof(struct Operand_));
        tmp_op->kind = TMP_VAR;
        tmp_op->u.var_no = varNo++;

        Type t = Exp(Exp_node, tmp_op, NULL);
        if (!t) return NULL;
        if (t->type != BASIC && t->type != CONSTANT) {
            printf("Error type 7 at line %d: Operands type mismatched\n",
                   child->row);
            return NULL;
        }

        // place := #0 - t1
        Operand zero_op = (Operand)malloc(sizeof(struct Operand_));
        zero_op->kind = CONSTANT_OP;
        zero_op->u.value = (char *)malloc(10);
        sprintf(zero_op->u.value, "0");

        InterCode tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
        tmp_code->kind = SUB_KIND;
        tmp_code->u.binop.result = place;
        tmp_code->u.binop.op1 = zero_op;
        tmp_code->u.binop.op2 = tmp_op;
        if (place) insertInterCode(tmp_code);

        return t;
    } else if (!strcmp(child->name, "NOT")) {
        Operand label1_op = (Operand)malloc(sizeof(struct Operand_));
        Operand label2_op = (Operand)malloc(sizeof(struct Operand_));
        label1_op->kind = LABEL_OP;
        label1_op->u.var_no = labelNo++;
        label2_op->kind = LABEL_OP;
        label2_op->u.var_no = labelNo++;

        InterCode tmp_code;
        Operand right_op;

        // place := #0
        if (place) {
            tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
            tmp_code->kind = ASSIGN;
            tmp_code->u.assign.left = place;

            right_op = (Operand)malloc(sizeof(struct Operand_));
            right_op->kind = CONSTANT_OP;
            right_op->u.value = (char *)malloc(10);
            sprintf(right_op->u.value, "0");

            tmp_code->u.assign.right = right_op;
            insertInterCode(tmp_code);
        }

        Type t = translate_Cond(n, label1_op, label2_op);

        if (!t) return NULL;
        if (t->type != BASIC || t->type_info.basic != INT_TYPE) {
            printf("Error type 7 at line %d: Operands type mismatched\n",
                   child->row);
            return NULL;
        }

        // LABEL label1
        tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
        tmp_code->kind = LABEL;
        tmp_code->u.unary.op = label1_op;
        insertInterCode(tmp_code);

        // place := #1
        if (place) {
            tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
            tmp_code->kind = ASSIGN;
            tmp_code->u.assign.left = place;

            right_op = (Operand)malloc(sizeof(struct Operand_));
            right_op->kind = CONSTANT_OP;
            right_op->u.value = (char *)malloc(10);
            sprintf(right_op->u.value, "1");

            tmp_code->u.assign.right = right_op;
            insertInterCode(tmp_code);
        }

        // LABEL label2
        tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
        tmp_code->kind = LABEL;
        tmp_code->u.unary.op = label2_op;
        insertInterCode(tmp_code);

        return t;
    } else if (!strcmp(child->name, "ID")) {
        // function call
        if (child->next != NULL) {
            VarType v = findSymbol(child->value);
            FuncType f = findFunc(child->value);
            if (v != NULL && f == NULL) {
                printf("Error type 11 at line %d: '%s' must be a function\n",
                       child->row, child->value);
                return NULL;
            }
            if (f == NULL) {
                // if (f == NULL || !f->isDefined) {
                printf("Error type 2 at line %d: Undefined function '%s'\n",
                       child->row, child->value);
                return NULL;
            }
            VarType param = f->param;
            child = child->next->next;
            // Exp -> ID LP RP
            if (strcmp(child->name, "RP") == 0) {
                if (param != NULL) {
                    printf("Error type 9 at line : The method '%s(", f->name);
                    printParam(param);
                    printf(")'is not applicable for the arguments '()'\n");
                }
                // READ place
                if (!strcmp(f->name, "read")) {
                    InterCode tmp_code =
                        (InterCode)malloc(sizeof(struct InterCode_));
                    tmp_code->kind = READ;
                    tmp_code->u.unary.op = place;
                    if (place) insertInterCode(tmp_code);
                }
                // place := CALL function.name
                else {
                    Operand tmp_op = (Operand)malloc(sizeof(struct Operand_));
                    tmp_op->kind = FUNCTION_OP;
                    tmp_op->u.value = f->name;

                    InterCode tmp_code =
                        (InterCode)malloc(sizeof(struct InterCode_));
                    tmp_code->kind = CALL;
                    if (!place) {
                        place = (Operand)malloc(sizeof(struct Operand_));
                        place->kind = TMP_VAR;
                        place->u.var_no = varNo;
                    }
                    tmp_code->u.assign.left = place;
                    tmp_code->u.assign.right = tmp_op;
                    insertInterCode(tmp_code);
                }
            }
            // Exp -> ID LP Args RP
            else {
                Operand arg_list = (Operand)malloc(sizeof(struct Operand_));
                arg_list->next = NULL;
                if (!Args(child, param, arg_list)) {
                    printf("Error type 9 at line : The method '%s(", f->name);
                    printParam(param);
                    printf(")'is not applicable for the arguments '(");
                    printArgs(child);
                    printf(")'\n");
                } else {
                    if (!strcmp(f->name, "write")) {
                        // WRITE arg_list[1]
                        InterCode tmp_code =
                            (InterCode)malloc(sizeof(struct InterCode_));
                        tmp_code->kind = WRITE;
                        tmp_code->u.unary.op = arg_list->next;
                        insertInterCode(tmp_code);
                    } else {
                        arg_list = arg_list->next;
                        InterCode tmp_code;
                        while (arg_list) {
                            tmp_code =
                                (InterCode)malloc(sizeof(struct InterCode_));
                            tmp_code->kind = ARG;
                            tmp_code->u.unary.op = arg_list;
                            insertInterCode(tmp_code);
                            arg_list = arg_list->next;
                        }
                        // place := CALL function.name
                        Operand tmp_op =
                            (Operand)malloc(sizeof(struct Operand_));
                        tmp_op->kind = FUNCTION_OP;
                        tmp_op->u.value = f->name;

                        tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
                        tmp_code->kind = CALL;
                        if (!place) {
                            place = (Operand)malloc(sizeof(struct Operand_));
                            place->kind = TMP_VAR;
                            place->u.var_no = varNo;
                        }
                        tmp_code->u.assign.left = place;
                        tmp_code->u.assign.right = tmp_op;
                        insertInterCode(tmp_code);
                    }
                }
            }
            return f->returnType;
        }
        // variable identifier
        else {
            VarType v = findSymbol(child->value);
            if (v == NULL) {
                printf("Error type 1 at line %d: Undefined variable '%s'\n",
                       child->row, child->value);
                return NULL;
            }
            if (place) {
                place->kind = VAR;
                place->u.value = child->value;
            }
            return v->type;
        }
    } else if (!strcmp(child->name, "INT")) {
        Type t = (Type)malloc(sizeof(struct Type_));
        t->type = CONSTANT;
        t->type_info.basic = INT_TYPE;
        if (place) {
            place->kind = CONSTANT_OP;
            place->u.value = child->value;
        }
        return t;
    } else if (!strcmp(child->name, "FLOAT")) {
        Type t = (Type)malloc(sizeof(struct Type_));
        t->type = CONSTANT;
        t->type_info.basic = FLOAT_TYPE;
        if (place) {
            place->kind = CONSTANT_OP;
            place->u.value = child->value;
        }
        return t;
    } else {
        printf("CODE ERROR:in function exp");
        return NULL;
    }
}

// father -> left op right
Type BinaryExp(Node *left, Node *op, Node *right, Operand place, Node *father) {
    // Exp -> Exp1 ASSIGNOP Exp2
    if (!strcmp(op->name, "ASSIGNOP")) {
        Type left_type, right_type;
        // tmp_op: 左值
        Operand tmp_op = (Operand)malloc(sizeof(struct Operand_));
        tmp_op->kind = TMP_VAR;
        tmp_op->u.var_no = varNo++;
        // 单个变量访问
        // Exp1 -> ID 得到ID所对应的变量
        if (!strcmp(left->children->name, "ID") && !left->children->next) {
            VarType tmp;
            if (left->children) tmp = findSymbol(left->children->value);
            if (!tmp) {
                printf("Error type 1 at line %d: Undefined variable '%s'\n",
                       left->row, left->children->value);
                return NULL;
            }
            if (tmp->type->type == BASIC)
                left_type = Exp(left, tmp_op, NULL);
            else {
                printf(
                    "Error type 6 at line %d: The left-hand side of an "
                    "assignment must be a variable\n",
                    left->row);
                return NULL;
            }
        }
        // 数组元素访问
        else if (!strcmp(left->children->name, "Exp") && left->children->next &&
                 !strcmp(left->children->next->name, "LB")) {
            left_type = Exp(left, tmp_op, NULL);
        }
        // 结构体特定域访问
        else if (!strcmp(left->children->name, "Exp") && left->children->next &&
                 !strcmp(left->children->next->name, "DOT")) {
            left_type = Exp(left, tmp_op, NULL);
        } else {
            printf(
                "Error type 6 at line %d: The left-hand side of an assignment "
                "must be a variable\n",
                left->row);
            return NULL;
        }

        Operand left_op = tmp_op;
        Operand right_op = (Operand)malloc(sizeof(struct Operand_));
        right_op->kind = TMP_VAR;
        right_op->u.var_no = varNo++;
        right_type = Exp(right, right_op, NULL);
        if (!left_type || !right_type) return NULL;
        if (isTypeEqual(left_type, right_type)) {
            // 此时右值的结果储存在right_op中
            // variable.name := t1
            InterCode tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
            tmp_code->kind = ASSIGN;
            tmp_code->u.assign.left = left_op;
            tmp_code->u.assign.right = right_op;
            insertInterCode(tmp_code);
            // place := variable.name
            tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
            tmp_code->kind = ASSIGN;
            tmp_code->u.assign.left = place;
            tmp_code->u.assign.right = right_op;
            if (place) insertInterCode(tmp_code);

            return left_type;
        } else {
            printf("Error type 5 at line %d: Type mismatched\n", left->row);
            return NULL;
        }
    }
    // Exp -> Exp1 PLUS Exp2 (MINUS, MUL, DIV)
    else if (!strcmp(op->name, "PLUS") || !strcmp(op->name, "MINUS") ||
             !strcmp(op->name, "MUL") || !strcmp(op->name, "DIV")) {
        Operand op1 = (Operand)malloc(sizeof(struct Operand_));
        Operand op2 = (Operand)malloc(sizeof(struct Operand_));
        op1->kind = TMP_VAR;
        op1->u.var_no = varNo++;
        op2->kind = TMP_VAR;
        op2->u.var_no = varNo++;

        Type left_type = Exp(left, op1, NULL);
        Type right_type = Exp(right, op2, NULL);
        if (!left_type || !right_type) return NULL;
        if ((left_type->type == BASIC || left_type->type == CONSTANT) &&
            (right_type->type == BASIC || right_type->type == CONSTANT)) {
            // place := t1 + t2
            InterCode tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
            // 判断加减乘除
            if (!strcmp(op->name, "PLUS")) {
                tmp_code->kind = ADD_KIND;
            } else if (!strcmp(op->name, "MINUS")) {
                tmp_code->kind = SUB_KIND;
            } else if (!strcmp(op->name, "MUL")) {
                tmp_code->kind = MUL_KIND;
            } else if (!strcmp(op->name, "DIV")) {
                tmp_code->kind = DIV_KIND;
            }
            tmp_code->u.binop.result = place;
            tmp_code->u.binop.op1 = op1;
            tmp_code->u.binop.op2 = op2;
            if (place) {
                insertInterCode(tmp_code);
            }
            return left_type;
        } else {
            printf("Error type 7 at line %d: Operands type mismatched\n",
                   left->row);
            return NULL;
        }
    } else if (!strcmp(op->name, "AND") || !strcmp(op->name, "OR") ||
               !strcmp(op->name, "RELOP")) {
        Operand label1_op = (Operand)malloc(sizeof(struct Operand_));
        Operand label2_op = (Operand)malloc(sizeof(struct Operand_));
        label1_op->kind = LABEL_OP;
        label1_op->u.var_no = labelNo++;
        label2_op->kind = LABEL_OP;
        label2_op->u.var_no = labelNo++;

        InterCode tmp_code;
        Operand right_op;

        // place := #0
        if (place) {
            tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
            tmp_code->kind = ASSIGN;
            tmp_code->u.assign.left = place;

            right_op = (Operand)malloc(sizeof(struct Operand_));
            right_op->kind = CONSTANT_OP;
            right_op->u.value = (char *)malloc(10);
            sprintf(right_op->u.value, "0");

            tmp_code->u.assign.right = right_op;
            insertInterCode(tmp_code);
        }

        Type t = translate_Cond(father, label1_op, label2_op);

        // LABEL label1
        tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
        tmp_code->kind = LABEL;
        tmp_code->u.unary.op = label1_op;
        insertInterCode(tmp_code);

        // place := #1
        if (place) {
            tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
            tmp_code->kind = ASSIGN;
            tmp_code->u.assign.left = place;

            right_op = (Operand)malloc(sizeof(struct Operand_));
            right_op->kind = CONSTANT_OP;
            right_op->u.value = (char *)malloc(10);
            sprintf(right_op->u.value, "1");

            tmp_code->u.assign.right = right_op;
            insertInterCode(tmp_code);
        }

        // LABEL label2
        tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
        tmp_code->kind = LABEL;
        tmp_code->u.unary.op = label2_op;
        insertInterCode(tmp_code);

        return t;
    } else {
        printf("ERROR:unkonwn operand %s at line %d\n", op->value, op->row);
        return NULL;
    }
}

// Exp1 RELOP Exp2
// NOT Exp1
// Exp1 AND Exp2
// Exp1 OR Exp2
// other cases
Type translate_Cond(Node *n, Operand label_true, Operand label_false) {
    Node *child = n->children;
    if (!child) return NULL;
    if (!strcmp(child->name, "Exp")) {
        Node *op_node = child->next;
        if (op_node && !strcmp(op_node->name, "RELOP")) {
            Operand op1 = (Operand)malloc(sizeof(struct Operand_));
            Operand op2 = (Operand)malloc(sizeof(struct Operand_));
            op1->kind = TMP_VAR;
            op1->u.var_no = varNo++;
            op2->kind = TMP_VAR;
            op2->u.var_no = varNo++;

            Node *exp1_node = child;
            Node *exp2_node = op_node->next;
            Type t1 = Exp(exp1_node, op1, NULL);
            Type t2 = Exp(exp2_node, op2, NULL);
            if (!t1 || !t2) return NULL;

            if ((t1->type == BASIC || t1->type == CONSTANT) &&
                (t2->type == BASIC || t2->type == CONSTANT) &&
                t1->type_info.basic == t2->type_info.basic) {
                // IF t1 op t2 GOTO label_true
                InterCode tmp_code =
                    (InterCode)malloc(sizeof(struct InterCode_));
                tmp_code->kind = IFGOTO;
                tmp_code->u.ifgoto.op = op_node->value;
                tmp_code->u.ifgoto.t1 = op1;
                tmp_code->u.ifgoto.t2 = op2;
                tmp_code->u.ifgoto.label = label_true;
                insertInterCode(tmp_code);

                // GOTO label_false
                tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
                tmp_code->kind = GOTO;
                tmp_code->u.unary.op = label_false;
                insertInterCode(tmp_code);

                return t1;
            } else {
                printf("Error type 7 at line %d: Operand type mismatched\n",
                       child->row);
                return NULL;
            }
        } else if (op_node && !strcmp(op_node->name, "AND")) {
            Operand label1_op = (Operand)malloc(sizeof(struct Operand_));
            label1_op->kind = LABEL_OP;
            label1_op->u.var_no = labelNo++;
            // child: Exp1
            Type t1 = translate_Cond(child, label1_op, label_false);

            // LABEL label1
            InterCode tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
            tmp_code->kind = LABEL;
            tmp_code->u.unary.op = label1_op;
            insertInterCode(tmp_code);

            // op_node->next: Exp2
            Type t2 = translate_Cond(op_node->next, label_true, label_false);
            if (!t1 || !t2) return NULL;
            if ((t1->type == BASIC || t1->type == CONSTANT) &&
                (t2->type == BASIC || t2->type == CONSTANT) &&
                t1->type_info.basic == t2->type_info.basic) {
                return t1;
            } else {
                printf("Error type 7 at line %d: Operand type mismatched\n",
                       child->row);
                return NULL;
            }
        } else if (op_node && !strcmp(op_node->name, "OR")) {
            Operand label1_op = (Operand)malloc(sizeof(struct Operand_));
            label1_op->kind = LABEL_OP;
            label1_op->u.var_no = labelNo++;
            // child: Exp1
            Type t1 = translate_Cond(child, label_true, label1_op);

            // LABEL label1
            InterCode tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
            tmp_code->kind = LABEL;
            tmp_code->u.unary.op = label1_op;
            insertInterCode(tmp_code);

            // op_node->next: Exp2
            Type t2 = translate_Cond(op_node->next, label_true, label_false);
            if (!t1 || !t2) return NULL;
            if ((t1->type == BASIC || t1->type == CONSTANT) &&
                (t2->type == BASIC || t2->type == CONSTANT) &&
                t1->type_info.basic == t2->type_info.basic) {
                return t1;
            } else {
                printf("Error type 7 at line %d: Operand type mismatched\n",
                       child->row);
                return NULL;
            }
        }
    } else if (!strcmp(child->name, "NOT")) {
        child = child->next;
        if (!child) return NULL;
        Type t = translate_Cond(child, label_false, label_true);
        if (!t) return NULL;
        if (t->type == BASIC && t->type_info.basic == INT_TYPE)
            return t;
        else {
            printf("Error type 7 at line %d: Operand type mismatched\n",
                   child->row);
            return NULL;
        }
    } else {
        // IF t1 != #0 GOTO label_true
        Operand op1 = (Operand)malloc(sizeof(struct Operand_));
        Operand op2 = (Operand)malloc(sizeof(struct Operand_));
        op1->kind = TMP_VAR;
        op1->u.var_no = varNo++;
        Type op1_type = Exp(n, op1, NULL);
        op2->kind = CONSTANT_OP;
        op2->u.value = malloc(10);
        sprintf(op2->u.value, "%d", 0);

        InterCode tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
        tmp_code->kind = IFGOTO;
        tmp_code->u.ifgoto.t1 = op1;
        tmp_code->u.ifgoto.t2 = op2;
        tmp_code->u.ifgoto.op = (char *)malloc(10);
        sprintf(tmp_code->u.ifgoto.op, "!=");
        tmp_code->u.ifgoto.label = label_true;
        insertInterCode(tmp_code);

        // GOTO label_false
        tmp_code = (InterCode)malloc(sizeof(struct InterCode_));
        tmp_code->kind = GOTO;
        tmp_code->u.unary.op = label_false;
        insertInterCode(tmp_code);

        return op1_type;
    }
    return NULL;
}

// Args -> Exp COMMA Args
//		| Exp
//返回值用于判断实际参数和形式参数是否匹配，VarType v是函数的形式参数列表
bool Args(Node *n, VarType v, Operand arg_list) {
    if (n == NULL && v == NULL)
        return true;
    else if (n == NULL || v == NULL)
        return false;

    Operand tmp_op = (Operand)malloc(sizeof(struct Operand_));
    tmp_op->kind = TMP_VAR;
    tmp_op->u.var_no = varNo++;

    Node *Exp_node = n->children;
    Type t = Exp(Exp_node, tmp_op, NULL);
    tmp_op->next = arg_list->next;
    arg_list->next = tmp_op;
    // 如果exp返回NULL说明出现了错误，此处不用重复报错，直接返回
    if (!t) return true;
    if (!isTypeEqual(t, v->type)) return false;
    //不直接调用Args(child->next->next,v->next_field);是因为child->next==NULL时child->next->next会出现指针错误
    if (Exp_node->next == NULL && v->next_field == NULL)
        return true;
    else if (Exp_node->next == NULL || v->next_field == NULL)
        return false;
    return Args(Exp_node->next->next, v->next_field, arg_list);
}

int getArraySize(Type t) {
    if (t->type == ARRAY) {
        return getTypeSize(t->type_info.array.element);
    } else
        return 0;
}

int getTypeSize(Type t) {
    if (t->type == BASIC || t->type == CONSTANT) {
        // int 和 float 都是四字节
        return 4;
    } else if (t->type == ARRAY) {
        int element_size = getTypeSize(t->type_info.array.element);
        int count = t->type_info.array.size;
        return element_size * count;
    } else if (t->type == STRUCTURE) {
        int size = 0;
        if (t->type_info.structure) {
            VarType cur = t->type_info.structure->varList;
            while (cur) {
                size += getTypeSize(cur->type);
                cur = cur->next_field;
            }
        }
        return size;
    }
    return 0;
}

char *Type2String(Type t) {
    if (t == NULL) return "NULL";
    if (t->type == BASIC || t->type == CONSTANT) {
        if (t->type_info.basic == INT_TYPE)
            return "int";
        else
            return "float";
    } else if (t->type == ARRAY) {
        char *res = (char *)malloc(50);
        int array_size = t->type_info.array.size;
        char *array_type = Type2String(t->type_info.array.element);
        sprintf(res, "ARRAY type{ %s } size %d", array_type, array_size);
        return res;
    } else if (t->type == STRUCTURE) {
        char *res = (char *)malloc(100);
        char *struct_name = t->type_info.structure->name;
        sprintf(res, "STURCTURE %s { ", struct_name);
        VarType cur = t->type_info.structure->varList;
        while (cur) {
            strcat(res, cur->name);
            strcat(res, " ");
            strcat(res, Type2String(cur->type));
            if (cur->next_field)
                strcat(res, ", ");
            else
                strcat(res, " }");
            cur = cur->next_field;
        }
        return res;
    } else {
        return "UNKNOWN TYPE";
    }
}

void printParam(VarType v) {
    while (v != NULL) {
        printf("%s", Type2String(v->type));
        v = v->next_field;
    }
}

void printArgs(Node *n) {
    Node *child = n->children;
    Type t = Exp(child, NULL, NULL);
    if (!t) return;
    printf("%s", Type2String(t));
    child = child->next;
    if (!child) return;
    child = child->next;
    printArgs(child);
}