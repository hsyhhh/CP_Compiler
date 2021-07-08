all:
	bison -d syntax.y
	flex lexical.l
	gcc -g syntax.tab.c TreeNode.c InterCode.c table.c IR2Obj.c -o parser.exe

clean:
	del -fR syntax.output syntax.tab.c syntax.tab.h lex.yy.c parser.exe