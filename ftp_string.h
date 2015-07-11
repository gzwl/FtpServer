#ifndef FTP_STRING_H
#define FTP_STRING_H

void StrSplit(const char *text,char *lhs,char *rhs,char cut);
void CleanRight(char *text);

//将字符串中的字母都变成大写
void LetterUpper(char *text);

//判断字符串是否都是空格
int AllSpace(const char *text);



#endif
