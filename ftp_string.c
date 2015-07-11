#include "ftp_string.h"
#include "common.h"

void StrSplit(const char *text,char *lhs,char *rhs,char cut)
{
	char *p = strchr(text,cut);
	if(p == NULL){
		strcpy(lhs,text);
	}
	else{
		;	
	}


}

void CleanRight(char *text)
{
	size_t len = strlen(text);
	while(len && (text[len-1] == '\r' || text[len-1] == '\n'))	len--;
	text[len] = '\0';

}

void LetterUpper(char *text)
{
	while(*text){
		if(*text >= 'a' && *text <= 'z'){
			*text = *text - ('a' - 'A');
		}
		text++;
	}
}

int AllSpace(const char *text)
{
	while(*text){	
		if(*text++ != ' ')		return 0;
	}
	return  1;

}



