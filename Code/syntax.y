%{
	#include <stdio.h>
	#include "TreeNode.h"
	#include "IR2obj.h"
	#include "InterCode.h"
	#include "lex.yy.c"
	#include "table.h"
	PtrToNode root;
%}

/*types*/
%union {
	PtrToNode node;
};

/*tokens*/
%token <node> INT FLOAT ID TYPE
%token <node> SEMI COMMA 
%token <node> LC RC
%token <node> IF 

%right <node> ASSIGNOP  
%left <node> OR 
%left <node> AND 
%left <node> RELOP
%left <node> PLUS MINUS 
%left <node> MUL DIV
%right <node> NOT
%left <node> LB RB LP RP
%left <node> DOT
%nonassoc <node> LOWER_THAN_ELSE 
%nonassoc <node> ELSE
%nonassoc <node> STRUCT RETURN WHILE

/*non-terminal*/
%type <node> Program ExtDefList ExtDef ExtDecList Specifier
%type <node> StructSpecifier OptTag Tag VarDec FunDec VarList
%type <node> ParamDec CompSt StmtList Stmt DefList Def DecList
%type <node> Dec Exp Args

%%

Program	: 	ExtDefList	{$$=NewNode("Program","");addChild($$,$1);root=$$;}
		;
ExtDefList	:	ExtDef ExtDefList	{$$=NewNode("ExtDefList","");addChild($$,$2);addChild($$,$1);}
	   	|	/*empty*/				{$$=NewNode("ExtDefList","");}	
		;
ExtDef	:	Specifier ExtDecList SEMI 	{$$=NewNode("ExtDef","");addChild($$,$3);addChild($$,$2);addChild($$,$1);}	
      	|	Specifier SEMI				{$$=NewNode("ExtDef","");addChild($$,$2);addChild($$,$1);}
		|	Specifier FunDec SEMI		{$$=NewNode("ExtDef","");addChild($$,$3);addChild($$,$2);addChild($$,$1);}
		|	Specifier FunDec CompSt		{$$=NewNode("ExtDef","");addChild($$,$3);addChild($$,$2);addChild($$,$1);}
		| 	error SEMI					{errorFlag=0;}
		;
ExtDecList	:	VarDec				{$$=NewNode("ExtDecList","");addChild($$,$1);}
	   	|	VarDec COMMA ExtDecList	{$$=NewNode("ExtDecList","");addChild($$,$3);addChild($$,$2);addChild($$,$1);} 
		;

Specifier	:	TYPE			{$$=NewNode("Specifier","");addChild($$,$1);}
	  	|	StructSpecifier		{$$=NewNode("Specifier","");addChild($$,$1);}
		;
StructSpecifier	:	STRUCT OptTag LC DefList RC 
							{$$=NewNode("StructSpecifier","");addChild($$,$5);addChild($$,$4);addChild($$,$3);addChild($$,$2);addChild($$,$1);}	
		|	STRUCT Tag		{$$=NewNode("StructSpecifier","");addChild($$,$2);addChild($$,$1);}
		;
OptTag	:	ID			{$$=NewNode("OptTag","");addChild($$,$1);}
       	|	/*empty*/	{$$=NewNode("OptTag","");}
		;
Tag	:	ID	{$$=NewNode("Tag","");addChild($$,$1);}
    	;

VarDec	:	ID					{$$=NewNode("VarDec","");addChild($$,$1);}
       	| 	VarDec LB INT RB	{/*数组定义*/$$=NewNode("VarDec","");addChild($$,$4);addChild($$,$3);addChild($$,$2);addChild($$,$1);}
		;
FunDec	: 	ID LP VarList RP	{$$=NewNode("FunDec","");addChild($$,$4);addChild($$,$3);addChild($$,$2);addChild($$,$1);}
       	|	ID LP RP		 	{$$=NewNode("FunDec","");addChild($$,$3);addChild($$,$2);addChild($$,$1);}
		|	error RP			{errorFlag=0;}
		;
		
VarList	:	ParamDec COMMA VarList	{$$=NewNode("VarList","");addChild($$,$3);addChild($$,$2);addChild($$,$1);}
		|	ParamDec				{$$=NewNode("VarList","");addChild($$,$1);}
		;
ParamDec:	Specifier VarDec	{$$=NewNode("ParamDec","");addChild($$,$2);addChild($$,$1);}
		;

CompSt	:	LC DefList StmtList RC	{$$=NewNode("CompSt","");addChild($$,$4);addChild($$,$3);addChild($$,$2);addChild($$,$1);}
		|   error RC				{errorFlag=0;}
		;
StmtList:	Stmt StmtList	{$$=NewNode("StmtList","");addChild($$,$2);addChild($$,$1);}
	 	|	/*empty*/		{$$=NewNode("StmtList","");}
		;		
Stmt:	Exp SEMI			{$$=NewNode("Stmt","");addChild($$,$2);addChild($$,$1);}
    |	CompSt				{$$=NewNode("Stmt","");addChild($$,$1);}
	|	RETURN Exp SEMI		{$$=NewNode("Stmt","");addChild($$,$3);addChild($$,$2);addChild($$,$1);}
	|	IF LP Exp RP Stmt %prec LOWER_THAN_ELSE 
							{$$=NewNode("Stmt","");addChild($$,$5);addChild($$,$4);addChild($$,$3);addChild($$,$2);addChild($$,$1);}
	|	IF LP Exp RP Stmt ELSE Stmt	
							{$$=NewNode("Stmt","");addChild($$,$7);addChild($$,$6);addChild($$,$5);addChild($$,$4);addChild($$,$3);addChild($$,$2);addChild($$,$1);}
	|	WHILE LP Exp RP Stmt	
							{$$=NewNode("Stmt","");addChild($$,$5);addChild($$,$4);addChild($$,$3);addChild($$,$2);addChild($$,$1);}
	|	error RP			{errorFlag=0;}
	| 	error SEMI			{errorFlag=0;}
	;

DefList	:	Def DefList			{$$=NewNode("DefList","");addChild($$,$2);addChild($$,$1);}
		|	/*empty*/			{$$=NewNode("DefList","");}	
		;
Def	:	Specifier DecList SEMI	{$$=NewNode("Def","");addChild($$,$3);addChild($$,$2);addChild($$,$1);}
	;
DecList	:	Dec					{$$=NewNode("DecList","");addChild($$,$1);}
		|	Dec COMMA DecList	{$$=NewNode("DecList","");addChild($$,$3);addChild($$,$2);addChild($$,$1);}
		;
Dec	:	VarDec					{$$=NewNode("Dec","");addChild($$,$1);}
    |	VarDec ASSIGNOP Exp		{$$=NewNode("Dec","");addChild($$,$3);addChild($$,$2);addChild($$,$1);}
	;

Exp :	Exp ASSIGNOP Exp	{$$=NewNode("Exp","");addChild($$,$3);addChild($$,$2);addChild($$,$1);}
    |	Exp AND Exp			{$$=NewNode("Exp","");addChild($$,$3);addChild($$,$2);addChild($$,$1);}
	| 	Exp OR Exp			{$$=NewNode("Exp","");addChild($$,$3);addChild($$,$2);addChild($$,$1);}
	|	Exp RELOP Exp		{$$=NewNode("Exp","");addChild($$,$3);addChild($$,$2);addChild($$,$1);}
	|	Exp PLUS Exp		{$$=NewNode("Exp","");addChild($$,$3);addChild($$,$2);addChild($$,$1);}
	|	Exp MINUS Exp		{$$=NewNode("Exp","");addChild($$,$3);addChild($$,$2);addChild($$,$1);}
	|	Exp MUL Exp			{$$=NewNode("Exp","");addChild($$,$3);addChild($$,$2);addChild($$,$1);}
	|	Exp DIV Exp			{$$=NewNode("Exp","");addChild($$,$3);addChild($$,$2);addChild($$,$1);}
	| 	LP Exp RP			{$$=NewNode("Exp","");addChild($$,$3);addChild($$,$2);addChild($$,$1);}
	|	MINUS Exp			{$$=NewNode("Exp","");addChild($$,$2);addChild($$,$1);}
	|	NOT Exp				{$$=NewNode("Exp","");addChild($$,$2);addChild($$,$1);}
	|	ID LP Args RP		{$$=NewNode("Exp","");addChild($$,$4);addChild($$,$3);addChild($$,$2);addChild($$,$1);}
	|	ID LP RP			{$$=NewNode("Exp","");addChild($$,$3);addChild($$,$2);addChild($$,$1);}
	|	Exp LB Exp RB		{$$=NewNode("Exp","");addChild($$,$4);addChild($$,$3);addChild($$,$2);addChild($$,$1);}
	|	Exp DOT ID			{$$=NewNode("Exp","");addChild($$,$3);addChild($$,$2);addChild($$,$1);}
	|	ID					{$$=NewNode("Exp","");addChild($$,$1);}	
	|	INT					{$$=NewNode("Exp","");addChild($$,$1);}
	|	FLOAT				{$$=NewNode("Exp","");addChild($$,$1);}
	;

Args:	Exp COMMA Args	{$$=NewNode("Args","");addChild($$,$3);addChild($$,$2);addChild($$,$1);}
	|	Exp				{$$=NewNode("Args","");addChild($$,$1);}
	;

%%

int main(int argc, char** argv)
{
 	if (argc <= 1) return -1;
	FILE* f = fopen(argv[1],"r");
	if (!f)
	{
		perror(argv[1]);
		return 1;
	}	
	root=NULL;
	yylineno=1;
	yyrestart(f);
	yyparse();
    // printf("-----------Parse Tree-----------\n");
	// printTree(root, 0);
	if(errorFlag)
	{
		initHashTable();
		Program(root);
		checkFuncIsDefined();
	}
	// printf("\n-----------Var Table----------\n");
	// printVarTable();

	// printf("\n----------Func Table----------\n");
	// printFuncTable();

	// printf("\n----------Inter Code----------\n");
	
	FILE * IRfile=fopen("intercode.ir","w");
	if(IRfile==NULL){
		printf("[Internal ERROR]: open IR file failed\n");
		exit(1);
	}
	printInterCode(IRfile);
	fclose(IRfile);
	printAllCode("obj.s");
	printf("Compile Done\n");
	return 0;
}

int yyerror(char* msg)
{
	printf("Error type B at line %d:%s: unexpected character near '%s'\n", yylineno, msg,yylval.node->value);
	return 0;
}

