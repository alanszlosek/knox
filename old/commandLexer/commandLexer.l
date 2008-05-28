%option noyywrap
%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int number = 0;

void getNumber() {
	char *c;
	c = yytext + 1;
	sscanf(c, "%d", &number);
}
%}

colon		:
quote		'
ditto		"\""
back		"\\"
digit		[0-9]
exp		(e|E)(\+|\-)?[0-9]+
hex		{digit}|[a-fA-F]
alpha		[a-zA-Z_]
ch		[^\n\\'"]
space		" "
whitespace	[ \n\t\r\v\f]
operator	"+="|"-="|"*="|"/="|"%="|"<<="|">>="|"<<"|">>"|"&="|"^="|"|="|"--"|"++"|"=="|"!="|"<="|">="|"&&"|"||"|"->"|"="|"+"|"-"|"*"|"/"|"%"|"&"|"!"|"|"|"^"|"~"|","|"?"|":"|"."|"sizeof"

%%

{colon}{digit}+ { getNumber(); }
"pause" { printf("pause: %d\n", number); }
{whitespace} {}

%%

int main() {
	yylex();
	return 0;
}

