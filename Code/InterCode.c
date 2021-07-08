#include "InterCode.h"

InterCode head = NULL;
InterCode tail = NULL;

int varNo = 1;
int labelNo = 1;

void insertInterCode(InterCode i) {
    i->prev = NULL;
    i->next = NULL;
    if (head) {
        tail->next = i;
        i->prev = tail;
        tail = i;
    } else {
        head = i;
        tail = i;
    }
}

void printOperand(Operand o, FILE* file) {
    if (!o) {
        return;
    }
    char tmpstr[32];
    switch (o->kind) {
        case VAR:
            sprintf(tmpstr, "%s", o->u.value);
            fputs(tmpstr, file);
            break;
        case CONSTANT_OP:
            sprintf(tmpstr, "#%s", o->u.value);
            fputs(tmpstr, file);
            break;
        case VAR_ADDRESS:
            sprintf(tmpstr, "*%s", o->u.var->u.value);
            fputs(tmpstr, file);
            break;
        case LABEL_OP:
            sprintf(tmpstr, "label%d", o->u.var_no);
            fputs(tmpstr, file);
            break;
        case FUNCTION_OP:
            sprintf(tmpstr, "%s", o->u.value);
            fputs(tmpstr, file);
            break;
        case TMP_VAR:
            sprintf(tmpstr, "t%d", o->u.var_no);
            fputs(tmpstr, file);
            break;
        case TMP_VAR_ADDRESS:
            sprintf(tmpstr, "*t%d", o->u.var->u.var_no);
            fputs(tmpstr, file);
            break;
    }
}

void printInterCode(FILE* file) {
    InterCode tmp = head;
    char str[32];
    while (tmp) {
        switch (tmp->kind) {
            case LABEL:
                fputs("LABEL ", file);
                printOperand(tmp->u.unary.op, file);
                fputs(" :", file);
                break;
            case FUNCTION:
                fputs("FUNCTION ", file);
                printOperand(tmp->u.unary.op, file);
                fputs(" :", file);
                break;
            case ASSIGN:
                printOperand(tmp->u.assign.left, file);
                fputs(" := ", file);
                printOperand(tmp->u.assign.right, file);
                break;
            case ADD_KIND:
                printOperand(tmp->u.binop.result, file);
                fputs(" := ", file);
                printOperand(tmp->u.binop.op1, file);
                fputs(" + ", file);
                printOperand(tmp->u.binop.op2, file);
                break;
            case SUB_KIND:
                printOperand(tmp->u.binop.result, file);
                fputs(" := ", file);
                printOperand(tmp->u.binop.op1, file);
                fputs(" - ", file);
                printOperand(tmp->u.binop.op2, file);
                break;
            case MUL_KIND:
                printOperand(tmp->u.binop.result, file);
                fputs(" := ", file);
                printOperand(tmp->u.binop.op1, file);
                fputs(" * ", file);
                printOperand(tmp->u.binop.op2, file);
                break;
            case DIV_KIND:
                printOperand(tmp->u.binop.result, file);
                fputs(" := ", file);
                printOperand(tmp->u.binop.op1, file);
                fputs(" / ", file);
                printOperand(tmp->u.binop.op2, file);
                break;
            case RIGHTAT:
                printOperand(tmp->u.assign.left, file);
                fputs(" := &", file);
                printOperand(tmp->u.assign.right, file);
                break;
            case GOTO:
                fputs("GOTO ", file);
                printOperand(tmp->u.unary.op, file);
                break;
            case IFGOTO:
                fputs("IF ", file);
                printOperand(tmp->u.ifgoto.t1, file);
                sprintf(str, " %s ", tmp->u.ifgoto.op);
                fputs(str, file);
                printOperand(tmp->u.ifgoto.t2, file);
                fputs(" GOTO ", file);
                printOperand(tmp->u.ifgoto.label, file);
                break;
            case RETURN_KIND:
                fputs("RETURN ", file);
                printOperand(tmp->u.unary.op, file);
                break;
            case DEC:
                fputs("DEC ", file);
                printOperand(tmp->u.dec.op, file);
                sprintf(str, " %d", tmp->u.dec.size);
                fputs(str, file);
                break;
            case ARG:
                fputs("ARG ", file);
                printOperand(tmp->u.unary.op, file);
                break;
            case CALL:
                printOperand(tmp->u.assign.left, file);
                fputs(" := CALL ", file);
                printOperand(tmp->u.assign.right, file);
                break;
            case PARAM:
                fputs("PARAM ", file);
                printOperand(tmp->u.unary.op, file);
                break;
            case READ:
                fputs("READ ", file);
                printOperand(tmp->u.unary.op, file);
                break;
            case WRITE:
                fputs("WRITE ", file);
                printOperand(tmp->u.unary.op, file);
                break;
        }
        fputs("\n", file);
        tmp = tmp->next;
    }
}