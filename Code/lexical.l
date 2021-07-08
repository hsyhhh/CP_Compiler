%{
	#include "TreeNode.h"
	#include "syntax.tab.h"
    #include "stdio.h"
    #include "stdlib.h"
	int errorFlag=1;
	extern FILE* yyin;
	static void comment(FILE* f);
    int yyerror(char* msg);
%}

space [ \t]+
/* identifier */
ID_character [_0-9a-zA-Z]
Pre_ID [_a-zA-Z]
id {Pre_ID}{ID_character}*
/*int constant and float constant*/
O   [0-7]
D   [0-9]
D_NOZERO [1-9]
E   ([Ee][+-]?{D}+)
H   [a-fA-F0-9]
HP  (0(x|X))
HEX_CONSTANT {HP}{H}+ 
OCT_CONSTANT 0{O}+
DEC_CONSTANT 0|{D_NOZERO}{D}*
float {D}*\.{D}+{E}?
dec_integer {DEC_CONSTANT}
other_integer {HEX_CONSTANT}|{OCT_CONSTANT}

%option yylineno

%%
{space} 	{/*space*/}
{dec_integer} 	{yylval.node=NewNode("INT",yytext);return INT;}
{other_integer} {   
    int tmp = String2Int(yytext);
    char *s = (char*)malloc(32);
    sprintf(s, "%d", tmp);
    yylval.node=NewNode("INT",s);
    return INT;
}
{float} 	{yylval.node=NewNode("FLOAT",yytext);return FLOAT;}
";" 		{yylval.node=NewNode("SEMI",yytext);return SEMI;}
"," 		{yylval.node=NewNode("COMMA",yytext);return COMMA;}
"=" 		{yylval.node=NewNode("ASSIGNOP",yytext);return ASSIGNOP;}
">" 		{yylval.node=NewNode("RELOP",yytext);return RELOP;}
"<" 		{yylval.node=NewNode("RELOP",yytext);return RELOP;}
">=" 		{yylval.node=NewNode("RELOP",yytext);return RELOP;}
"<=" 		{yylval.node=NewNode("RELOP",yytext);return RELOP;}
"==" 		{yylval.node=NewNode("RELOP",yytext);return RELOP;}
"!=" 		{yylval.node=NewNode("RELOP",yytext);return RELOP;}
"+" 		{yylval.node=NewNode("PLUS",yytext);return PLUS;}
"-" 		{yylval.node=NewNode("MINUS",yytext);return MINUS;}
"*" 		{yylval.node=NewNode("MUL",yytext);return MUL;}
"/" 		{yylval.node=NewNode("DIV",yytext);return DIV;}
"&&" 		{yylval.node=NewNode("AND",yytext);return AND;}
"||" 		{yylval.node=NewNode("OR",yytext);return OR;}
"." 		{yylval.node=NewNode("DOT",yytext);return DOT;}
"!" 		{yylval.node=NewNode("NOT",yytext);return NOT;}
int|float 	{yylval.node=NewNode("TYPE",yytext);return TYPE;}
"(" 		{yylval.node=NewNode("LP",yytext);return LP;}
")" 		{yylval.node=NewNode("RP",yytext);return RP;}
"[" 		{yylval.node=NewNode("LB",yytext);return LB;}
"]" 		{yylval.node=NewNode("RB",yytext);return RB;}
"{" 		{yylval.node=NewNode("LC",yytext);return LC;}
"}" 		{yylval.node=NewNode("RC",yytext);return RC;}
struct 		{yylval.node=NewNode("STRUCT",yytext);return STRUCT;}
return 		{yylval.node=NewNode("RETURN",yytext);return RETURN;}
if			{yylval.node=NewNode("IF",yytext);return IF;}
else 		{yylval.node=NewNode("ELSE",yytext);return ELSE;}
while 		{yylval.node=NewNode("WHILE",yytext);return WHILE;}	
\n			{/*line end*/}
{id} 		{yylval.node=NewNode("ID",yytext);return ID;}
.			{errorFlag=0;printf("Error type A at line %d: Mysterious character '%s'\n",yylineno,yytext);}
"/*"		{comment(yyin);}
"//".*\n    {}
%%

int yywrap(){
   return 1;
}

static void comment(FILE* f)//读取注释，直到注释结束
{
    int c;
    // 使用getchar()会一直收到-1，-1代表EOF，说明重定向没有将文件流传入stdin
    while ((c = input()) != 0 && c!=EOF) 
    { 
        if (c == '*')
        {
            while ((c = input()) == '*');
            if (c == '/')return;//找到注释结尾
            if (c == 0)break;//字符串结尾
        }
    } //字符串结尾
    yyerror("unterminated comment");
}

int String2Int(char *s) {  //允许16进制和8进制字符串转整数
    if (s == NULL) {
        printf("ERROR:NULL pointer in String2Int function\n");
        return 0;
    }
    if (s[0] == '-') {
        return -String2Int(s + 1);
    }
    int len = strlen(s);
    int n = 0, tmp, i;
    if (s[0] == '0') {
        if (len == 1)
            return 0;
        else if (s[1] == 'x' || s[1] == 'X') {  // 16进制
            for (i = 2; i < len; i++) {
                if (s[i] >= 'A' &&
                    s[i] <=
                        'F')  //十六进制还要判断他是不是在A-F或者a-f之间a=10。。
                    tmp = s[i] - 'A' + 10;
                else if (s[i] >= 'a' && s[i] <= 'f')
                    tmp = s[i] - 'a' + 10;
                else
                    tmp = s[i] - '0';
                n = n * 16 + tmp;
            }
            return n;
        } else {  // 8进制
            for (i = 1; i < len; i++) {
                tmp = s[i] - '0';
                n = n * 8 + tmp;
            }
            return n;
        }
    } else
        return atoi(s);
}

