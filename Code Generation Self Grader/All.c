/************************************************************************
University of Leeds
School of Computing
COMP2932- Compiler Design and Construction
Lexer Module

I confirm that the following code has been developed and written by me and it is entirely the result of my own work.
I also confirm that I have not copied any parts of this program from another person or any other source or facilitated someone to copy this program from me.
I confirm that I will not publish the program online or share it with anyone without permission of the module leader.

Student Name: Jiahe Lin
Student ID: 201803683
Email: sc23j4l@leeds.ac.uk
Date Work Commenced: 2025/2/13
*************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

// YOU CAN ADD YOUR OWN FUNCTIONS, DECLARATIONS AND VARIABLES HERE
#define ReswordsNum 21
#define SymbolsNum 19
const char *RESWORDS[ReswordsNum] = {"class", "method", "function", "constructor", "int", "boolean", "char", "void", "var", "static", "field", "let", "do", "if", "else", "while", "return", "true", "false", "null", "this"};
const char *SYMBOLS[SymbolsNum] = {"{", "}", "(", ")", "[", "]", ".", ",", ";", "+", "-", "*", "/", "&", "|", "<", ">", "=", "~"};

FILE *file = NULL;
char filename[128];
int LineCount = 0, TokenReady = 0;

Token t;

int isResword(char *str)
{
  for (int i = 0; i < ReswordsNum; i++)
    if (!strcmp(RESWORDS[i], str))
      return 1;
  return 0;
}

int isSymbol(char c)
{
  for (int i = 0; i < SymbolsNum; ++i)
    if (SYMBOLS[i][0] == c)
      return 1;
  return 0;
}

char skipWhitespaceAndComments()
{
  char c;
  while ((c = getc(file)) != EOF)
  {
    if (isspace(c))
    {
      if (c == '\n')
        LineCount++;
      continue;
    }
    if (c == '/')
    {
      char next = fgetc(file);
      if (next == '/')
      {
        while ((c = fgetc(file)) != EOF && c != '\n')
          ;
        if (c == '\n')
          LineCount++;
        continue;
      }
      else if (next == '*')
      {
        while ((c = fgetc(file)) != EOF)
        {
          if (c == '\n')
            LineCount++;
          if (c == '*')
          {
            next = fgetc(file);
            if (next == '/')
              break;
            ungetc(next, file);
          }
        }
        if (c == EOF)
        {
          t.ec = EofInCom;
          strcpy(t.lx, "Error: unexpected eof in comment");
          t.ln = LineCount;
          strcpy(t.fl, filename);
          return EOF;
        }
        continue;
      }
      ungetc(next, file);
    }
    return c;
  }
  return EOF;
}

Token createToken()
{
  t.tp = ERR;
  t.ec = -1; // No error
  strcpy(t.fl, filename);
  char c = skipWhitespaceAndComments();

  // Error: EOF in comment
  if(t.ec == EofInCom) return t;

  if (c == EOF)
  {
    t.tp = EOFile;
    t.ln = LineCount;
    return t;
  }

  char temp[128];
  int i = 0;

  // Deal with ID/Reswords
  if (isalpha(c)|| c == '_')
  {
    while (c != EOF &&  (isalpha(c) || isdigit(c) || c == '_'))
    {
      temp[i++] = c;
      c = getc(file);
    }
    temp[i] = '\0';
    ungetc(c, file);

    strcpy(t.lx, temp);
    if (isResword(temp))
      t.tp = RESWORD;
    else
      t.tp = ID;
    t.ln = LineCount;
    return t;
  }
  // Deal with number
  else if (isdigit(c))
  {
    while (c != EOF && isdigit(c))
    {
      temp[i++] = c;
      c = getc(file);
    }

    temp[i] = '\0';
    ungetc(c, file);

    strcpy(t.lx, temp);
    t.tp = INT;
    t.ln = LineCount;
    return t;
  }
  else if (isSymbol(c))
  {
    t.lx[0] = c;
    t.lx[1] = '\0';
    t.tp = SYMBOL;
    t.ln = LineCount;
    return t;
  }

  // Deal with string
  else if (c == '"')
  {
    int i = 0;
    while ((c = fgetc(file)) != EOF && c != '"')
    {
      if (c == '\n')
      {
        t.ec = NewLnInStr;
        strcpy(t.lx, "Error: new line in string constant");
        t.ln = LineCount;
        LineCount++; 
        return t;
      }
      t.lx[i++] = c;
    }
    if (c == EOF)
    {
      t.ec = EofInStr;
      strcpy(t.lx, "Error: unexpected eof in string constant");
      t.ln = LineCount;
      return t;
    }
    t.tp = STRING;
    t.lx[i] = '\0';
    return t;
  }
  // None of above type
  t.ec = IllSym;
  strcpy(t.lx, "Error: illegal symbol in source file");
  t.ln = LineCount;
  return t;
}

// IMPLEMENT THE FOLLOWING functions
//***********************************

int InitLexer(char *file_name)
{
  file = fopen(file_name, "r");
  if (file == NULL)
    return 0;

  strcpy(filename, file_name);

  TokenReady = 0;
  LineCount = 1;
  return 1;
}

// Get the next token from the source file
Token GetNextToken()
{
  if (TokenReady)
  {
    TokenReady = 0;
    return t;
  }
  t = createToken();
  TokenReady = 0;
  return t;
}

// peek (look) at the next token in the source file without removing it from the stream
Token PeekNextToken()
{
  if (TokenReady)
    return t;
  t = createToken();
  TokenReady = 1;
  return t;
}

// clean out at end, e.g. close files, free memory, ... etc
int StopLexer()
{
  if (file)
  {
    fclose(file);
    file = NULL;
  }
  return 0;
}

// do not remove the next line
#ifndef TEST
int main()
{
  if (!InitLexer("Ball.jack"))
  {
    printf("Failed to open file.\n");
    return 1;
  }

  Token token;
  while (1)
  {
    token = GetNextToken();
    if (token.tp == EOFile)
    {
      printf("< %s, %d, End of File, EOFile >\n", filename, token.ln);
      break;
    }

    switch (token.tp)
    {
    case RESWORD:
      printf("< %s, %d, %s, RESWORD >\n", filename, token.ln, token.lx);
      break;
    case ID:
      printf("< %s, %d, %s, ID >\n", filename, token.ln, token.lx);
      break;
    case INT:
      printf("< %s, %d, %s, INT >\n", filename, token.ln, token.lx);
      break;
    case SYMBOL:
      printf("< %s, %d, %s, SYMBOL >\n", filename, token.ln, token.lx);
      break;
    case STRING:
      printf("< %s, %d, %s, STRING >\n", filename, token.ln, token.lx);
      break;
    case ERR:
      printf("< %s, %d, Error: %s, ERR >\n", filename, token.ln, token.lx);
      break;
    default:
      printf("< %s, %d, Unknown token type, ERR >\n", filename, token.ln);
      break;
    }
  }

  StopLexer();
  return 0;
}
// do not remove the next line
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "lexer.h"
#include "parser.h"
#include "symbols.h"

// you can declare prototypes of parser functions below
extern FILE *file;

ParserInfo lexerError(Token t);
ParserInfo ClassDeclar();
ParserInfo memberDeclar();
ParserInfo classVarDeclar();
ParserInfo parseType();
ParserInfo subroutineDeclar();
ParserInfo paramList();
ParserInfo subroutineBody();
ParserInfo statement();
ParserInfo varDeclarStatement();
ParserInfo letStatement();
ParserInfo ifStatement();
ParserInfo whileStatement();
ParserInfo doStatement();
ParserInfo returnStatement();
ParserInfo expression();
ParserInfo relationalExpression();
ParserInfo arithmeticExpression();
ParserInfo term();
ParserInfo factor();
ParserInfo operand();
ParserInfo subroutineCall();
ParserInfo expressionList();

ParserInfo lexerError(Token t)
{
	ParserInfo pi;
	pi.er = none;
	if (t.tp == ERR)
	{
		pi.er = lexerErr;
		pi.tk = t;
		return pi;
	}
	return pi;
}

// classDeclar → class identifier { {memberDeclar} }
ParserInfo ClassDeclar()
{
    ParserInfo pi;
    pi.er = none;

    // 检查 "class" 关键字
    Token t = GetNextToken();
    pi = lexerError(t);
    if (pi.er != none)
        return pi;
    if (t.tp != RESWORD || strcmp(t.lx, "class") != 0)
    {
        pi.er = classExpected;
        pi.tk = t;
        return pi;
    }

    // 检查类名标识符
    t = GetNextToken();
    pi = lexerError(t);
    if (pi.er != none)
        return pi;
    if (t.tp != ID)
    {
        pi.er = idExpected;
        pi.tk = t;
        return pi;
    }
    // 预处理时已经插入，不需要再次插入。进入类作用域
    EnterSubScope(t.lx);

    // 检查 "{"
    t = GetNextToken();
    pi = lexerError(t);
    if (pi.er != none)
        return pi;
    if (t.tp != SYMBOL || strcmp(t.lx, "{") != 0)
    {
        pi.er = openBraceExpected;
        pi.tk = t;
        return pi;
    }

    t = PeekNextToken();
    while (!(t.tp == SYMBOL && strcmp(t.lx, "}") == 0))
    {
        // 对于子例程声明，需要解析其具体实现
        if (t.tp == RESWORD &&
            (!strcmp(t.lx, "constructor") ||
             !strcmp(t.lx, "function") ||
             !strcmp(t.lx, "method")))
        {
            pi = subroutineDeclar();
            if (pi.er != none)
                return pi;
        }
        else if (t.tp == RESWORD &&
                 (!strcmp(t.lx, "static") || !strcmp(t.lx, "field")))
        {
            // 对于字段声明，调用原有的 classVarDeclar 即可
            pi = classVarDeclar();
            if (pi.er != none)
                return pi;
        }
        else
        {
            pi.er = memberDeclarErr;
            pi.tk = t;
            return pi;
        }
        t = PeekNextToken();
    }

    // 检查 "}" 并退出类作用域
    t = GetNextToken();
    pi = lexerError(t);
    if (pi.er != none)
        return pi;
    if (t.tp != SYMBOL || strcmp(t.lx, "}") != 0)
    {
        pi.er = closeBraceExpected;
        pi.tk = t;
        return pi;
    }
    ExitScope();
    return pi;
}

// memberDeclar → classVarDeclar | subroutineDeclar
ParserInfo memberDeclar()
{
	ParserInfo pi;
	pi.er = none;

	Token t = PeekNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;

	// check class var
	if (t.tp == RESWORD &&
		(!strcmp(t.lx, "static") ||
		 !strcmp(t.lx, "field")))
	{
		return classVarDeclar();
	}
	// check subroutine
	else if (t.tp == RESWORD &&
			 (!strcmp(t.lx, "constructor") ||
			  !strcmp(t.lx, "function") ||
			  !strcmp(t.lx, "method")))
	{
		return subroutineDeclar();
	}
	else
	{
		pi.er = memberDeclarErr;
		pi.tk = t;
		return pi;
	}
}

// classVarDeclar → (static | field) type identifier {, identifier} ;
ParserInfo classVarDeclar()
{
	ParserInfo pi;
	pi.er = none;

	Token t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;

	if (t.tp != RESWORD ||
		(strcmp(t.lx, "static") &&
		 strcmp(t.lx, "field")))
	{
		pi.er = classVarErr;
		pi.tk = t;
		return pi;
	}

	Kinds dataKind = parseKind(t);

	pi = parseType();
	if (pi.er != none)
		return pi;

	// 获取 type 对应的 DataTypes
	DataTypes dataType = parseDataType(pi.tk);
	char type_name[128] = "";  // 创建局部缓冲区
	if (dataType == CLASS) {
		strncpy(type_name, pi.tk.lx, sizeof(type_name) - 1);
		type_name[sizeof(type_name) - 1] = '\0';
	}

	t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	// check id
	if (t.tp != ID)
	{
		pi.er = idExpected;
		pi.tk = t;
		return pi;
	}
	// 预处理已经插入过，不需要再次插入

	t = PeekNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	// check comma
	while (t.tp == SYMBOL && !strcmp(t.lx, ","))
	{
		GetNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
		t = GetNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
		if (t.tp != ID)
		{
			pi.er = idExpected;
			pi.tk = t;
			return pi;
		}

		// 预处理已经插入过，不需要再次插入

		t = PeekNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
	}

	t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != SYMBOL || strcmp(t.lx, ";"))
	{
		pi.er = semicolonExpected;
		pi.tk = t;
		return pi;
	}
	return pi;
}

// type → int | char | boolean | identifier
ParserInfo parseType()
{
	ParserInfo pi;
	pi.er = none;

	Token t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;

	if (t.tp == RESWORD)
	{
		if (strcmp(t.lx, "int") &&
			strcmp(t.lx, "char") &&
			strcmp(t.lx, "boolean"))
		{
			pi.er = illegalType;
			pi.tk = t;
			return pi;
		}
	}
	else if (t.tp != ID)
	{
		pi.er = illegalType;
		pi.tk = t;
		return pi;
	}

	// 检查symbol table
	if (t.tp == ID && FindSymbol(t.lx, true) == NULL)
	{
		pi.er = undecIdentifier;
		pi.tk = t;
		return pi;
	}

	pi.tk = t;
	return pi;
}

// subroutineDeclar → (constructor | function | method) (type|void) identifier (paramList) subroutineBody
ParserInfo subroutineDeclar()
{
	ParserInfo pi;
	pi.er = none;

	Token t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != RESWORD ||
		(strcmp(t.lx, "constructor") &&
		 strcmp(t.lx, "function") &&
		 strcmp(t.lx, "method")))
	{
		pi.er = subroutineDeclarErr;
		pi.tk = t;
		return pi;
	}

	// 存储当前kind和即将进入的scope
	Kinds kind = parseKind(t);
	ScopeType scope = parseScopeType(t);

	t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp == RESWORD && !strcmp(t.lx, "void"))
	{
	}
	else
	{
		if (t.tp == RESWORD)
		{
			if (strcmp(t.lx, "int") &&
				strcmp(t.lx, "char") &&
				strcmp(t.lx, "boolean"))
			{
				pi.er = illegalType;
				pi.tk = t;
				return pi;
			}
		}
		else if (t.tp != ID)
		{
			pi.er = illegalType;
			pi.tk = t;
			return pi;
		}
	}
	// 获取 type 对应的 DataTypes
	DataTypes dataType = parseDataType(t);
	char type_name[128] = "";
	if (dataType == CLASS) {
		strncpy(type_name, t.lx, sizeof(type_name) - 1);
		type_name[sizeof(type_name) - 1] = '\0';
	}

	t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != ID)
	{
		pi.er = idExpected;
		pi.tk = t;
		return pi;
	}
	// 符号表中找到符号
	EnterSubScope(t.lx); // TODO: 使用新函数进入符号的子表？如果是类的对象，应该使用symbol的typename而不是t.lx
	// DEBUG
	// printf("EnterSubScope: %s, ScopeTypeNow: %d\n", t.lx, getCurrentScope());

	t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != SYMBOL || strcmp(t.lx, "("))
	{
		pi.er = openParenExpected;
		pi.tk = t;
		return pi;
	}

	pi = paramList();
	if (pi.er != none)
		return pi;

	t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != SYMBOL || strcmp(t.lx, ")"))
	{
		pi.er = closeParenExpected;
		pi.tk = t;
		return pi;
	}

	pi = subroutineBody();
	if (pi.er != none)
		return pi;


	ExitScope();
	// DEBUG
	// printf("Exit scope, ScopeTypeNow: %d\n", getCurrentScope());
	return pi;
}

// paramList → type identifier {, type identifier} | ε
ParserInfo paramList()
{
	ParserInfo pi;
	pi.er = none;

	Token t = PeekNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;

	if (t.tp == SYMBOL && strcmp(t.lx, ")") == 0)
		return pi;

	if (!((t.tp == RESWORD &&
		   (!strcmp(t.lx, "int") || !strcmp(t.lx, "char") || !strcmp(t.lx, "boolean"))) ||
		  t.tp == ID))
	{
		pi.er = closeParenExpected;
		pi.tk = t;
		return pi;
	}

	while (1)
	{
		pi = parseType();
		if (pi.er != none)
			return pi;

		DataTypes paramType = parseDataType(pi.tk);
        char type_name[128] = "";
        if (paramType == CLASS) {
            strncpy(type_name, pi.tk.lx, sizeof(type_name) - 1);
            type_name[sizeof(type_name) - 1] = '\0';
        }

		t = GetNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;

		if (t.tp != ID)
		{
			pi.er = idExpected;
			pi.tk = t;
			return pi;
		}

		// 检查参数是否重复定义
		if (FindSymbol(t.lx, false)) // 在当前作用域查找
		{
			pi.er = redecIdentifier;
			pi.tk = t;
			return pi;
		}
		InsertSymbol(t.lx, paramType, ARG, type_name);

		t = PeekNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;

		if (t.tp == SYMBOL && strcmp(t.lx, ",") == 0)
		{
			GetNextToken();
			t = PeekNextToken();
			pi = lexerError(t);
			if (pi.er != none)
				return pi;
		}
		else
		{
			break;
		}
	}
	return pi;
}

// subroutineBody → { {statement} }
ParserInfo subroutineBody()
{
	ParserInfo pi;
	pi.er = none;
	Token t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != SYMBOL || strcmp(t.lx, "{"))
	{
		pi.er = openBraceExpected;
		pi.tk = t;
		return pi;
	}

	while (1)
	{
		t = PeekNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
		if (t.tp == SYMBOL && !strcmp(t.lx, "}"))
			break;
		pi = statement();
		if (pi.er != none)
			return pi;
	}
	t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != SYMBOL || strcmp(t.lx, "}"))
	{
		pi.er = closeBraceExpected;
		pi.tk = t;
		return pi;
	}
	return pi;
}

// statement → varDeclarStatement | letStatemnt | ifStatement | whileStatement | doStatement | returnStatemnt
ParserInfo statement()
{
	ParserInfo pi;
	pi.er = none;

	Token t = PeekNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp == RESWORD)
	{
		if (!strcmp(t.lx, "var"))
			return varDeclarStatement();
		else if (!strcmp(t.lx, "let"))
			return letStatement();
		else if (!strcmp(t.lx, "if"))
			return ifStatement();
		else if (!strcmp(t.lx, "while"))
			return whileStatement();
		else if (!strcmp(t.lx, "do"))
			return doStatement();
		else if (!strcmp(t.lx, "return"))
			return returnStatement();
	}

	pi.er = syntaxError;
	pi.tk = t;
	return pi;
}

// varDeclarStatement → var type identifier { , identifier } ;
ParserInfo varDeclarStatement()
{
	ParserInfo pi;
	pi.er = none;
	Token t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != RESWORD || strcmp(t.lx, "var"))
	{
		pi.er = syntaxError;
		pi.tk = t;
		return pi;
	}

	pi = parseType();
	if (pi.er != none)
		return pi;

	// 获取 type 对应的 DataTypes
	DataTypes dataType = parseDataType(pi.tk);
	char type_name[128] = "";  // 创建局部缓冲区
	if (dataType == CLASS) {
		strncpy(type_name, pi.tk.lx, sizeof(type_name) - 1);
		type_name[sizeof(type_name) - 1] = '\0';
	}
	

	t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != ID)
	{
		pi.er = idExpected;
		pi.tk = t;
		return pi;
	}

	// 检查是否重复声明
	if (FindSymbol(t.lx, false))
	{
		pi.er = redecIdentifier;
		pi.tk = t;
		return pi;
	}
	InsertSymbol(t.lx, dataType, VAR, type_name);

	t = PeekNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	while (t.tp == SYMBOL && !strcmp(t.lx, ","))
	{
		GetNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
		t = GetNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
		if (t.tp != ID)
		{
			pi.er = idExpected;
			pi.tk = t;
			return pi;
		}
		
		// 检查是否重复声明
		if (FindSymbol(t.lx, false))
		{
			pi.er = redecIdentifier;
			pi.tk = t;
			return pi;
		}
		InsertSymbol(t.lx, dataType, VAR, type_name);

		t = PeekNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;

	}

	t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != SYMBOL || strcmp(t.lx, ";"))
	{
		pi.er = semicolonExpected;
		pi.tk = t;
		return pi;
	}
	return pi;
}

// letStatemnt → let identifier [ [ expression ] ] = expression ;
ParserInfo letStatement()
{
	ParserInfo pi;
	pi.er = none;

	Token t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != RESWORD || strcmp(t.lx, "let"))
	{
		pi.er = syntaxError;
		pi.tk = t;
		return pi;
	}

	t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;

	if (t.tp != ID)
	{
		pi.er = idExpected;
		pi.tk = t;
		return pi;
	}

	// 检查变量是否存在
	if (FindSymbol(t.lx, true) == NULL)
	{
		pi.er = undecIdentifier;
		pi.tk = t;
		return pi;
	}

	t = PeekNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp == SYMBOL && !strcmp(t.lx, "["))
	{
		GetNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;

		pi = expression();

		if (pi.er != none)
			return pi;

		t = GetNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
		if (t.tp != SYMBOL || strcmp(t.lx, "]"))
		{
			pi.er = closeBracketExpected;
			pi.tk = t;
			return pi;
		}
	}

	t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != SYMBOL || strcmp(t.lx, "="))
	{
		pi.er = equalExpected;
		pi.tk = t;
		return pi;
	}

	pi = expression();
	if (pi.er != none)
		return pi;

	t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != SYMBOL || strcmp(t.lx, ";"))
	{
		pi.er = semicolonExpected;
		pi.tk = t;
		return pi;
	}
	return pi;
}

// ifStatement → if ( expression ) { {statement} } [else { {statement} }]
ParserInfo ifStatement()
{
	ParserInfo pi;
	pi.er = none;
	Token t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != RESWORD || strcmp(t.lx, "if"))
	{
		pi.er = syntaxError;
		pi.tk = t;
		return pi;
	}

	t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != SYMBOL || strcmp(t.lx, "("))
	{
		pi.er = openParenExpected;
		pi.tk = t;
		return pi;
	}
	pi = expression();
	if (pi.er != none)
		return pi;

	t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != SYMBOL || strcmp(t.lx, ")"))
	{
		pi.er = closeParenExpected;
		pi.tk = t;
		return pi;
	}

	t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != SYMBOL || strcmp(t.lx, "{"))
	{
		pi.er = openBraceExpected;
		pi.tk = t;
		return pi;
	}

	while (1)
	{
		t = PeekNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
		if (t.tp == SYMBOL && !strcmp(t.lx, "}"))
			break;
		pi = statement();
		if (pi.er != none)
			return pi;
	}

	t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != SYMBOL || strcmp(t.lx, "}"))
	{
		pi.er = closeBraceExpected;
		pi.tk = t;
		return pi;
	}

	t = PeekNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp == RESWORD && !strcmp(t.lx, "else"))
	{
		GetNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
		t = GetNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
		if (t.tp != SYMBOL || strcmp(t.lx, "{"))
		{
			pi.er = openBraceExpected;
			pi.tk = t;
			return pi;
		}
		while (1)
		{
			t = PeekNextToken();
			pi = lexerError(t);
			if (pi.er != none)
				return pi;
			if (t.tp == SYMBOL && !strcmp(t.lx, "}"))
				break;
			pi = statement();
			if (pi.er != none)
				return pi;
		}
		t = GetNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
		if (t.tp != SYMBOL || strcmp(t.lx, "}"))
		{
			pi.er = closeBraceExpected;
			pi.tk = t;
			return pi;
		}
	}
	return pi;
}
// whileStatement → while ( expression ) { {statement} }
ParserInfo whileStatement()
{
	ParserInfo pi;
	pi.er = none;
	Token t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != RESWORD || strcmp(t.lx, "while"))
	{
		pi.er = syntaxError;
		pi.tk = t;
		return pi;
	}

	t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != SYMBOL || strcmp(t.lx, "("))
	{
		pi.er = openParenExpected;
		pi.tk = t;
		return pi;
	}
	pi = expression();
	if (pi.er != none)
		return pi;

	t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != SYMBOL || strcmp(t.lx, ")"))
	{
		pi.er = closeParenExpected;
		pi.tk = t;
		return pi;
	}

	t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != SYMBOL || strcmp(t.lx, "{"))
	{
		pi.er = openBraceExpected;
		pi.tk = t;
		return pi;
	}

	while (1)
	{
		t = PeekNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
		if (t.tp == SYMBOL && !strcmp(t.lx, "}"))
			break;
		pi = statement();
		if (pi.er != none)
			return pi;
	}

	t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != SYMBOL || strcmp(t.lx, "}"))
	{
		pi.er = closeBraceExpected;
		pi.tk = t;
		return pi;
	}
	return pi;
}

// doStatement → do subroutineCall ;
ParserInfo doStatement()
{
	ParserInfo pi;
	pi.er = none;
	Token t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != RESWORD || strcmp(t.lx, "do"))
	{
		pi.er = syntaxError;
		pi.tk = t;
		return pi;
	}
	pi = subroutineCall();
	if (pi.er != none)
		return pi;
	t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != SYMBOL || strcmp(t.lx, ";"))
	{
		pi.er = semicolonExpected;
		pi.tk = t;
		return pi;
	}
	return pi;
}

// subroutineCall → identifier [ . identifier ] ( expressionList )
ParserInfo subroutineCall()
{
    ParserInfo pi;
    pi.er = none;

    Token t = GetNextToken();
    pi = lexerError(t);
    if (pi.er != none)
        return pi;
    if (t.tp != ID)
    {
        pi.er = idExpected;
        pi.tk = t;
        return pi;
    }

    // 先查找该标识符是否存在
    if (!FindSymbol(t.lx, true))
    {
        pi.er = undecIdentifier;
        pi.tk = t;
        return pi;
    }

    Token next = PeekNextToken();
    pi = lexerError(next);
    if (pi.er != none)
        return pi;

    // 如果遇到点号，立即进入该标识符的子符号表
    if (next.tp == SYMBOL && strcmp(next.lx, ".") == 0)
    {
        GetNextToken(); // 消耗 "."
        // 保存当前所在的子符号表，用于返回时恢复
        SymbolTable *old_table = current_table;
        // 进入第一个ID的子表
        // 查找对象的符号，并获取其类型名
        Symbol *objSym = FindSymbol(t.lx, true);
        if (objSym == NULL)
        {
            pi.er = undecIdentifier;
            pi.tk = t;
            return pi;
        }
        // 使用对象所属类型的名称进入子符号表，而不是直接使用对象名
        EnterSubScope(objSym->type_name);

        // 读取方法名
        t = GetNextToken();
        pi = lexerError(t);
        if (pi.er != none)
            return pi;
        if (t.tp != ID)
        {
            pi.er = idExpected;
            pi.tk = t;
            return pi;
        }

        // 在符号表中查找（METHOD 或 FUNCTION）
        Symbol *method_sym = FindSymbol(t.lx, true);
        if (!method_sym)
        {
            pi.er = undecIdentifier;
            pi.tk = t;
            return pi;
        }

        // 检查并消耗 "("
        t = GetNextToken();
        pi = lexerError(t);
        if (pi.er != none)
            return pi;

        if (t.tp != SYMBOL || strcmp(t.lx, "(") != 0)
        {
            pi.er = openParenExpected;
            pi.tk = t;
            return pi;
        }

        // 进入方法的子作用域（解析参数列表）：
        if (method_sym->sub_tables == NULL)
        {
            pi.er = undecIdentifier;
            pi.tk = t;
            return pi;
        }

        // 完成点调用后退出该子作用域（返回到原始作用域）
        ExitScope();
        // 恢复原来的作用域
        current_table = old_table;

        pi = expressionList();

        if (pi.er != none)
            return pi;

        // 检查并消耗 ")"
        t = GetNextToken();
        pi = lexerError(t);
        if (pi.er != none)
            return pi;
        if (t.tp != SYMBOL || strcmp(t.lx, ")") != 0)
        {
            pi.er = closeParenExpected;
            pi.tk = t;
            return pi;
        }

        return pi;
    }
    // 处理直接调用形式： method() 的情况
    else if (next.tp == SYMBOL && strcmp(next.lx, "(") == 0)
    {
        GetNextToken(); // 消耗 "("
        pi = expressionList();
        if (pi.er != none)
            return pi;
        t = GetNextToken();
        pi = lexerError(t);
        if (pi.er != none)
            return pi;
        if (t.tp != SYMBOL || strcmp(t.lx, ")") != 0)
        {
            pi.er = closeParenExpected;
            pi.tk = t;
            return pi;
        }
        return pi;
    }
    return pi;
}

// expressionList → expression { , expression } | ε
ParserInfo expressionList()
{
	ParserInfo pi;
	pi.er = none;

	Token t = PeekNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;

	if (t.tp == SYMBOL && !strcmp(t.lx, ")"))
		return pi;

	pi = expression();
	if (pi.er != none)
		return pi;
	t = PeekNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	while (t.tp == SYMBOL && !strcmp(t.lx, ","))
	{
		GetNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
		pi = expression();
		if (pi.er != none)
			return pi;
		t = PeekNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
	}
	return pi;
}
// returnStatemnt → return [ expression ] ;
ParserInfo returnStatement()
{
	ParserInfo pi;
	pi.er = none;

	Token t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != RESWORD || strcmp(t.lx, "return"))
	{
		pi.er = syntaxError;
		pi.tk = t;
		return pi;
	}

	t = PeekNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;

	if (t.tp == SYMBOL && !strcmp(t.lx, ";"))
	{
	}
	else if (t.tp == INT || t.tp == STRING ||
			 t.tp == ID ||
			 (t.tp == RESWORD &&
			  (!strcmp(t.lx, "true") || !strcmp(t.lx, "false") ||
			   !strcmp(t.lx, "null") || !strcmp(t.lx, "this"))) ||
			 (t.tp == SYMBOL &&
			  (!strcmp(t.lx, "(") || !strcmp(t.lx, "-") || !strcmp(t.lx, "~"))))
	{
		pi = expression();
		if (pi.er != none)
			return pi;
	}
	else
	{
		pi.er = semicolonExpected;
		pi.tk = t;
		return pi;
	}

	t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp != SYMBOL || strcmp(t.lx, ";"))
	{
		pi.er = semicolonExpected;
		pi.tk = t;
		return pi;
	}
	return pi;
}

// expression → relationalExpression { ( & | | ) relationalExpression }
ParserInfo expression()
{
	ParserInfo pi;
	pi.er = none;
	pi = relationalExpression();
	if (pi.er != none)
		return pi;
	Token t = PeekNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	while (t.tp == SYMBOL && (!strcmp(t.lx, "&") || !strcmp(t.lx, "|")))
	{
		GetNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
		pi = relationalExpression();
		if (pi.er != none)
			return pi;
		t = PeekNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
	}
	return pi;
}
// relationalExpression → ArithmeticExpression { ( = | > | < ) ArithmeticExpression }
ParserInfo relationalExpression()
{
	ParserInfo pi;
	pi.er = none;
	pi = arithmeticExpression();
	if (pi.er != none)
		return pi;
	Token t = PeekNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	while (t.tp == SYMBOL && (!strcmp(t.lx, "=") || !strcmp(t.lx, ">") || !strcmp(t.lx, "<")))
	{
		GetNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
		pi = arithmeticExpression();
		if (pi.er != none)
			return pi;
		t = PeekNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
	}
	return pi;
}

// ArithmeticExpression → term { ( + | - ) term }
ParserInfo arithmeticExpression()
{
	ParserInfo pi;
	pi.er = none;
	pi = term();
	if (pi.er != none)
		return pi;
	Token t = PeekNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	while (t.tp == SYMBOL && (!strcmp(t.lx, "+") || !strcmp(t.lx, "-")))
	{
		GetNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
		pi = term();
		if (pi.er != none)
			return pi;
		t = PeekNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
	}
	return pi;
}

// term → factor { ( * | / ) factor }
ParserInfo term()
{
	ParserInfo pi;
	pi.er = none;
	pi = factor();
	if (pi.er != none)
		return pi;
	Token t = PeekNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	while (t.tp == SYMBOL && (!strcmp(t.lx, "*") || !strcmp(t.lx, "/")))
	{
		GetNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
		pi = factor();
		if (pi.er != none)
			return pi;
		t = PeekNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
	}
	return pi;
}

// factor → ( - | ~ | ε ) operand
ParserInfo factor()
{
	ParserInfo pi;
	pi.er = none;
	Token t = PeekNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp == SYMBOL && (!strcmp(t.lx, "-") || !strcmp(t.lx, "~")))
	{
		GetNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
	}
	pi = operand();
	if (pi.er != none)
		return pi;
	return pi;
}

// operand → integerConstant | identifier [.identifier ] [ [ expression ] | (expressionList ) ] | (expression) | stringLiteral | true | false | null | this
ParserInfo operand()
{
	ParserInfo pi;
	pi.er = none;
	Token t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;

	if (t.tp == INT || t.tp == STRING)
		return pi; // 直接返回整数或字符串字面量
	else if (t.tp == RESWORD)
	{
		// 处理 true/false/null/this， 如果都不是则添加错误
		if (strcmp(t.lx, "true") && strcmp(t.lx, "false") &&
			strcmp(t.lx, "null") && strcmp(t.lx, "this"))
		{
			pi.er = syntaxError;
			pi.tk = t;
		}
		return pi;
	}
	else if (t.tp == ID)
	{
		// 步骤 1: 检查标识符是否已声明
		Symbol *var_sym = FindSymbol(t.lx, true);
		if (!var_sym)
		{
			pi.er = undecIdentifier;
			pi.tk = t;
			return pi;
		}

		Token next = PeekNextToken();
		pi = lexerError(next);
		if (pi.er != none)
			return pi;

		// 处理数组访问（例如 arr[expression]）
		if (next.tp == SYMBOL && strcmp(next.lx, "[") == 0)
		{
			GetNextToken(); // 消耗 [
			pi = expression();
			if (pi.er != none)
				return pi;
			t = GetNextToken();
			if (t.tp != SYMBOL || strcmp(t.lx, "]") != 0)
			{
				pi.er = closeBracketExpected;
				pi.tk = t;
			}
			return pi;
		}
		// 处理对象方法调用（例如 obj.method()）
		else if (next.tp == SYMBOL && strcmp(next.lx, ".") == 0)
		{
			GetNextToken(); // 消耗 .
			Token method_name = GetNextToken();
			if (method_name.tp != ID)
			{
				pi.er = idExpected;
				pi.tk = method_name;
				return pi;
			}

			// 步骤 2: 确定class object对应的class
			Symbol *class_sym = FindSymbol(var_sym->type_name, true);

			if (!class_sym || !class_sym->sub_tables)
			{
				pi.er = undecIdentifier;
				pi.tk = method_name;
				return pi;
			}

			// 步骤 3: 进入class scope查找方法
            // 保存当前所在的子符号表，用于返回时恢复
            SymbolTable *old_table = current_table;
			EnterSubScope(class_sym->type_name);
			Symbol *method_sym = FindSymbol(method_name.lx, false);
			if (!method_sym)
			{
				pi.er = undecIdentifier;
				pi.tk = method_name;
				return pi;
			}
            ExitScope();
            current_table = old_table;

			// 步骤 4: 处理方法调用参数列表
			t = GetNextToken();
			if (t.tp != SYMBOL || strcmp(t.lx, "(") != 0)
			{
				pi.er = openParenExpected;
				pi.tk = t;
				return pi;
			}

			// 处理方法调用参数列表（例如 method(expression)）
			pi = expressionList();

			if (pi.er != none)
				return pi;

			t = GetNextToken();
			if (t.tp != SYMBOL || strcmp(t.lx, ")") != 0)
			{
				pi.er = closeParenExpected;
				pi.tk = t;
			}
			return pi;
		}
		// 处理直接方法调用（例如 method()）
		else if (next.tp == SYMBOL && strcmp(next.lx, "(") == 0)
		{
			GetNextToken(); // 消耗 (
			pi = expressionList();
			if (pi.er != none)
				return pi;
			t = GetNextToken();
			if (t.tp != SYMBOL || strcmp(t.lx, ")") != 0)
			{
				pi.er = closeParenExpected;
				pi.tk = t;
			}
			return pi;
		}
		return pi; // 普通变量引用
	}
	else if (t.tp == SYMBOL && strcmp(t.lx, "(") == 0)
	{
		// 处理括号表达式 (expression)
		pi = expression();
		if (pi.er != none)
			return pi;
		t = GetNextToken();
		if (t.tp != SYMBOL || strcmp(t.lx, ")") != 0)
		{
			pi.er = closeParenExpected;
			pi.tk = t;
		}
		return pi;
	}
	else
	{
		pi.er = syntaxError;
		pi.tk = t;
		return pi;
	}
}

int InitParser(char *file_name)
{
	file = fopen(file_name, "r");
	if (file == NULL)
		return 0;
	return 1;
}

ParserInfo Parse()
{
	ParserInfo pi;
	pi = ClassDeclar();
	return pi;
}

int StopParser()
{
	StopLexer();
	if (file)
		fclose(file);
	return 1;
}

#ifndef TEST_PARSER
void debugParser(const char *filename)
{

	FILE *file = fopen(filename, "r");
	if (file == NULL)
	{
		printf("Error opening file: %s\n", filename);
		return;
	}

	InitLexer(file);

	InitParser(file);

	ParserInfo pi = Parse();

	if (pi.er != none)
	{
		printf("Error type: %d", pi.er);
		printf(", line: %d, token: %s\n", pi.tk.ln, pi.tk.lx);
	}
	else
	{
		printf("Parsing successful, no errors detected.\n");
	}

	StopParser();
	StopLexer();
}
int main()
{
	const char *testFiles[] = {
		"closeParenExpected.jack",
		"semicolonExpected.jack"};

	for (int i = 0; i < sizeof(testFiles) / sizeof(testFiles[0]); i++)
	{
		printf("\nParsing file: %s\n", testFiles[i]);
		debugParser(testFiles[i]);
	}
}
#endif
#include "symbols.h"
SymbolTable *current_table = NULL;

// utils
DataTypes parseDataType(Token t)
{
    if (t.tp == RESWORD)
    {
        if (strcmp(t.lx, "int") == 0)
            return INTEGER;
        if (strcmp(t.lx, "char") == 0)
            return CHAR;
        if (strcmp(t.lx, "boolean") == 0)
            return BOOLEAN;
        if (strcmp(t.lx, "void") == 0)
            return VOID;
    }
    return CLASS; // default
}

Kinds parseKind(Token t)
{
    if (t.tp == RESWORD)
    {
        if (strcmp(t.lx, "static") == 0)
            return STATIC;
        if (strcmp(t.lx, "field") == 0)
            return FIELD;
        if (strcmp(t.lx, "arg") == 0)
            return ARG;
        if (strcmp(t.lx, "var") == 0)
            return VAR;

        if (strcmp(t.lx, "function") == 0)
            return FUNCTION;
        if (strcmp(t.lx, "method") == 0)
            return METHOD;
        if (strcmp(t.lx, "constructor") == 0)
            return CONSTRUCTOR;
    }
}

ScopeType parseScopeType(Token t)
{
    if (t.tp == RESWORD)
    {
        if (strcmp(t.lx, "function") == 0)
            return FUNCTION_SCOPE;
        if (strcmp(t.lx, "class") == 0)
            return CLASS_SCOPE;
        if (strcmp(t.lx, "method") == 0 || strcmp(t.lx, "constructor") == 0)
            return METHOD_SCOPE;
    }
    return PROGRAM_SCOPE;
}

void InitSymbolTable()
{
    SymbolTable *program_table = (SymbolTable *)malloc(sizeof(SymbolTable));
    program_table->scope_type = PROGRAM_SCOPE;
    program_table->parent = NULL;
    program_table->count = 0;
    program_table->static_idx = 0;
    program_table->field_idx = 0;
    program_table->arg_idx = 0;
    program_table->var_idx = 0;

    current_table = program_table;
}


/**
 * Enter the sub-scope of the given symbol name.
 *
 * If the symbol name can be found in the current scope, and the symbol
 * has a sub-table, then this function will enter the sub-table and
 * become the current table. If the sub-table is a method scope, this
 * function will also automatically insert the "this" parameter.
 *
 * If the symbol name cannot be found, or if the symbol does not have a
 * sub-table, this function will print an error message and do nothing.
 *
 * @param[in] symbol_name The name of the symbol to enter the sub-scope of.
 */
void EnterSubScope(const char *symbol_name)
{
    Symbol *sym = FindSymbol(symbol_name, true);
    if (sym && sym->sub_table_count > 0)
    {
        current_table = sym->sub_tables[0];
        // 如果进入的是方法作用域，自动插入 this 参数
        if (current_table->scope_type == METHOD_SCOPE)
            AutoInsertThis();
    }
    else
    {
        printf("Error: Sub-table not found for %s\n", symbol_name);
    }
}

ScopeType getCurrentScope()
{
    return current_table ? current_table->scope_type : PROGRAM_SCOPE;
}

/**
 * Exit the current scope and return to the parent scope.
 *
 * If the current scope has a parent, this function will free the current
 * table and all its sub-tables, and then set the current table to the
 * parent table. Otherwise, this function does nothing.
 */
void ExitScope()
{
    if (current_table && current_table->parent)
    {
        SymbolTable *parent = current_table->parent;
        if (current_table->scope_type == METHOD_SCOPE || current_table->scope_type == FUNCTION_SCOPE)
            FreeSymbolTable(current_table); // 递归释放当前表及其子表
        current_table = parent;
    }
}

// 自动插入 this 参数（类型为当前类）
void AutoInsertThis()
{
    // 获取父表（类作用域）的类名
    SymbolTable *class_table = current_table->parent;
    const char *className = "";
    for (int i = 0; i < class_table->count; i++)
    {
        if (class_table->symbols[i].kind == STATIC &&
            class_table->symbols[i].type == CLASS)
        {
            className = class_table->symbols[i].name;
            break;
        }
    }

    // 插入 this 参数到当前方法作用域
    Symbol this_symbol;
    strcpy(this_symbol.name, "this");
    this_symbol.type = CLASS;
    this_symbol.kind = ARG;
    this_symbol.index = current_table->arg_idx++;
    this_symbol.sub_table_count = 0;
    this_symbol.sub_tables = NULL;

    current_table->symbols[current_table->count++] = this_symbol;
}

// 插入符号到当前表（修正索引分配逻辑）
void InsertSymbol(const char *name, DataTypes type, Kinds kind, const char *type_name)
{
    if (current_table->count >= MAX_SYMBOLS)
    {
        printf("Error: Symbol Table Overflow!\n");
        return;
    }

    Symbol new_symbol;
    memset(&new_symbol, 0, sizeof(Symbol)); // 确保内存清零
    strncpy(new_symbol.name, name, sizeof(new_symbol.name) - 1);
    new_symbol.type = type;
    new_symbol.kind = kind;

    strncpy(new_symbol.type_name, type_name, sizeof(new_symbol.type_name) - 1);
    new_symbol.type_name[sizeof(new_symbol.type_name) - 1] = '\0';
    // Debug
    // printf("name: %s\n", new_symbol.name);
    // printf("type_name: %s\n", new_symbol.type_name);

    // 为函数/方法/构造函数以及类（CLASS 且 kind 为 STATIC）创建子表
    if (kind == FUNCTION || kind == METHOD || kind == CONSTRUCTOR ||
        (type == CLASS && kind == STATIC))
    {
        SymbolTable *sub_table = (SymbolTable *)malloc(sizeof(SymbolTable));
        sub_table->parent = current_table;
        if (kind == FUNCTION)
            sub_table->scope_type = FUNCTION_SCOPE;
        else if (kind == METHOD)
            sub_table->scope_type = METHOD_SCOPE;
        else if (kind == CONSTRUCTOR)
        {
            sub_table->scope_type = METHOD_SCOPE;
        }
        else if (type == CLASS && kind == STATIC)
            sub_table->scope_type = CLASS_SCOPE;

        sub_table->count = 0;
        sub_table->static_idx = 0;
        sub_table->field_idx = 0;
        sub_table->arg_idx = 0;
        sub_table->var_idx = 0;

        new_symbol.sub_table_count = 1;
        new_symbol.sub_tables = (SymbolTable **)malloc(sizeof(SymbolTable *));
        new_symbol.sub_tables[0] = sub_table;
    }

    //分配索引
    if (kind == STATIC)
        new_symbol.index = current_table->static_idx++;
    else if (kind == FIELD)
        new_symbol.index = current_table->field_idx++;
    else if (kind == ARG)
        new_symbol.index = current_table->arg_idx++;
    else if (kind == VAR)
        new_symbol.index = current_table->var_idx++;

    current_table->symbols[current_table->count++] = new_symbol;
}

// 按照符号名字查找符号, 允许向上查找（search_parent = true时）
Symbol *FindSymbol(const char *name, bool search_parent)
{
    SymbolTable *table = current_table;
    while (table)
    {
        for (int i = 0; i < table->count; i++)
        {
            if (strcmp(table->symbols[i].name, name) == 0)
            {
                return &table->symbols[i];
            }
        }
        if (!search_parent)
            break;
        table = table->parent;
    }
    return NULL;
}

// 通过名称查找子表（支持嵌套）
SymbolTable *GetSubTableByName(Symbol *parent, const char *name)
{
    for (int i = 0; i < parent->sub_table_count; i++)
    {
        SymbolTable *sub = parent->sub_tables[i];
        for (int j = 0; j < sub->count; j++)
        {
            if (strcmp(sub->symbols[j].name, name) == 0)
            {
                return sub;
            }
        }
        // 递归查找子表的子表
        for (int j = 0; j < sub->count; j++)
        {
            SymbolTable *nested = GetSubTableByName(&sub->symbols[j], name);
            if (nested)
                return nested;
        }
    }
    return NULL;
}

/**
 * 释放符号表
 * @param[in] table 待释放符号表
 */
void FreeSymbolTable(SymbolTable *table)
{
    if (!table)
        return;

    // 递归释放所有子表
    for (int i = 0; i < table->count; i++)
    {
        Symbol *sym = &table->symbols[i];
        for (int j = 0; j < sym->sub_table_count; j++)
        {
            FreeSymbolTable(sym->sub_tables[j]);
        }
        free(sym->sub_tables); // 释放子表指针数组
    }
    free(table); // 释放符号表自身
}

// 根据当前符号表中下标 r 获取符号的地址（index）
int GetAddressByIndex(int r)
{
    if (current_table == NULL || r < 0 || r >= current_table->count) {
        printf("Invalid symbol index: %d\n", r);
        return -1;
    }
    return current_table->symbols[r].index;
}

// 根据符号名查找符号，并返回其地址（index）；未找到时返回 -1
int GetAddress(const char *name)
{
    Symbol *sym = FindSymbol(name, true);
    if (sym == NULL) {
        printf("Symbol not found: %s\n", name);
        return -1;
    }
    return sym->index;
}

#include "compiler.h"
#include <dirent.h>
#include <string.h>
#include <stdio.h>

FILE *file;

// 判断是否是 .jack 文件
int isJackFile(const char *filename) {
    const char *dot = strrchr(filename, '.');
    return dot && strcmp(dot, ".jack") == 0;
}

// ---------- 以下是预扫描相关函数 ----------


// type → int | char | boolean | identifier
ParserInfo preParseType()
{
	ParserInfo pi;
	pi.er = none;

	Token t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;

	if (t.tp == RESWORD)
	{
		if (strcmp(t.lx, "int") &&
			strcmp(t.lx, "char") &&
			strcmp(t.lx, "boolean"))
		{
			pi.er = illegalType;
			pi.tk = t;
			return pi;
		}
	}
	else if (t.tp != ID)
	{
		pi.er = illegalType;
		pi.tk = t;
		return pi;
	}

	// 检查symbol table
	if (t.tp == ID && FindSymbol(t.lx, true) == NULL)
	{
		pi.er = undecIdentifier;
		pi.tk = t;
		return pi;
	}

	pi.tk = t;
	return pi;
}

// 预扫描类体中字段声明（不解析右边的表达式等，只插入符号）
ParserInfo PreScanClassVarDeclar()
{
    ParserInfo pi;
    pi.er = none;
    // 读取关键字（static 或 field）
    Token kw = GetNextToken();
    pi = lexerError(kw);
    if (pi.er != none)
        return pi;
    if (kw.tp != RESWORD || (strcmp(kw.lx, "static") && strcmp(kw.lx, "field")))
    {
        pi.er = classVarErr;
        pi.tk = kw;
        return pi;
    }
    
    // 解析类型（预先拷贝类型名）
    pi = preParseType();
    if (pi.er != none)
        return pi;
    DataTypes dataType = parseDataType(pi.tk);
    char local_type_name[128] = "";
    if (dataType == CLASS) {
        strncpy(local_type_name, pi.tk.lx, sizeof(local_type_name)-1);
        local_type_name[sizeof(local_type_name)-1] = '\0';
    }
    
    // 读取标识符
    Token idToken = GetNextToken();
    pi = lexerError(idToken);
    if (pi.er != none)
        return pi;
    if (idToken.tp != ID)
    {
        pi.er = idExpected;
        pi.tk = idToken;
        return pi;
    }
    
    // 检查重复声明
    if (FindSymbol(idToken.lx, false))
    {
        pi.er = redecIdentifier;
        pi.tk = idToken;
        return pi;
    }
    // 插入符号，使用正确的 kind
    Kinds kind = parseKind(kw);
    InsertSymbol(idToken.lx, dataType, kind, local_type_name);
    
    // 处理可能的多个变量声明（逗号分隔）
    Token t = PeekNextToken();
    while (t.tp == SYMBOL && strcmp(t.lx, ",") == 0)
    {
        GetNextToken(); // 消耗逗号
        t = GetNextToken();
        pi = lexerError(t);
        if (pi.er != none)
            return pi;
        if (t.tp != ID)
        {
            pi.er = idExpected;
            pi.tk = t;
            return pi;
        }
        if (FindSymbol(t.lx, false))
        {
            pi.er = redecIdentifier;
            pi.tk = t;
            return pi;
        }
        InsertSymbol(t.lx, dataType, kind, local_type_name);
        t = PeekNextToken();
    }
    
    // 消耗分号
    t = GetNextToken();
    pi = lexerError(t);
    if (pi.er != none)
        return pi;
    if (t.tp != SYMBOL || strcmp(t.lx, ";") != 0)
    {
        pi.er = semicolonExpected;
        pi.tk = t;
        return pi;
    }
    return pi;
}

// 预扫描子例程声明：解析签名，插入符号，跳过函数体
ParserInfo PreScanSubroutineDeclar()
{
    ParserInfo pi;
    pi.er = none;
    // 读取子例程关键字（constructor/function/method）
    Token kw = GetNextToken();
    pi = lexerError(kw);
    if (pi.er != none)
        return pi;
    if (kw.tp != RESWORD ||
        (strcmp(kw.lx, "constructor") &&
         strcmp(kw.lx, "function") &&
         strcmp(kw.lx, "method")))
    {
        pi.er = subroutineDeclarErr;
        pi.tk = kw;
        return pi;
    }
    Kinds kind = parseKind(kw);
    ScopeType scope = parseScopeType(kw);
    
    // 解析返回类型（对于 void 或普通类型）
    Token t = GetNextToken();
    pi = lexerError(t);
    if (pi.er != none)
        return pi;
    
    DataTypes dataType; // 修复点：根据 token 类型对 dataType 进行初始化
    if (t.tp == RESWORD && strcmp(t.lx, "void") == 0)
    {
        dataType = VOID;
    }
    else if (t.tp == RESWORD)
    {
        // 如果是保留字，但不是 void，则可能是 int/char/boolean 或构造函数的类名
        if ((strcmp(t.lx, "int") == 0) ||
            (strcmp(t.lx, "char") == 0) ||
            (strcmp(t.lx, "boolean") == 0))
        {
            dataType = parseDataType(t);
        }
        else
        {
            // 如果是 constructor，则允许用户定义类型作为返回类型
            if (kind == CONSTRUCTOR)
                dataType = CLASS;
            else
            {
                pi.er = illegalType;
                pi.tk = t;
                return pi;
            }
        }
    }
    else if (t.tp == ID)
        dataType = CLASS;
    else
    {
        pi.er = illegalType;
        pi.tk = t;
        return pi;
    }
    
    // 若 dataType 为 CLASS，则保存类型名称
    char local_type_name[128] = "";
    if (dataType == CLASS) {
        strncpy(local_type_name, t.lx, sizeof(local_type_name)-1);
        local_type_name[sizeof(local_type_name)-1] = '\0';
    }
    
    // 读取子例程名称
    Token idToken = GetNextToken();
    pi = lexerError(idToken);
    if (pi.er != none)
        return pi;
    if (idToken.tp != ID)
    {
        pi.er = idExpected;
        pi.tk = idToken;
        return pi;
    }

    if (FindSymbol(idToken.lx, false))
    {
        pi.er = redecIdentifier;
        pi.tk = idToken;
        return pi;
    }
    // DEBUG
    // printf("PreScanSubroutineDeclar: %s %d %d %s\n", idToken.lx, dataType, kind, local_type_name);
    // 插入符号
    InsertSymbol(idToken.lx, dataType, kind, local_type_name);
    
    // 预扫描参数列表
    t = GetNextToken();
    pi = lexerError(t);
    if (t.tp != SYMBOL || strcmp(t.lx, "(") != 0)
    {
        pi.er = openParenExpected;
        pi.tk = t;
        return pi;
    }
    // 简单跳过参数列表（假定参数列表内不会出错）
    int parenCount = 1;
    while (parenCount > 0)
    {
        t = GetNextToken();
        pi = lexerError(t);
        if (t.tp == SYMBOL)
        {
            if (strcmp(t.lx, "(") == 0)
                parenCount++;
            else if (strcmp(t.lx, ")") == 0)
                parenCount--;
        }
    }
    
    // 预扫描跳过子例程体（必须是 { ... } 块）
    t = GetNextToken();
    pi = lexerError(t);
    if (t.tp != SYMBOL || strcmp(t.lx, "{") != 0)
    {
        pi.er = openBraceExpected;
        pi.tk = t;
        return pi;
    }
    // 跳过整个函数体块
    int braceCount = 1;
    while (braceCount > 0)
    {
        t = GetNextToken();
        pi = lexerError(t);
        if (t.tp == SYMBOL)
        {
            if (strcmp(t.lx, "{") == 0)
                braceCount++;
            else if (strcmp(t.lx, "}") == 0)
                braceCount--;
        }
    }
    return pi;
}

// 预扫描整个类体成员（当前假定当前文件已通过 InitParser 初始化）
ParserInfo PreScanClassMembers()
{
    ParserInfo pi;
    pi.er = none;

    // 检查 "class" 关键字
    Token t = GetNextToken();
    pi = lexerError(t);
    if (pi.er != none)
        return pi;
    if (t.tp != RESWORD || strcmp(t.lx, "class") != 0)
    {
        pi.er = classExpected;
        pi.tk = t;
        return pi;
    }

    // 检查 class 标识符
    t = GetNextToken();
    pi = lexerError(t);
    if (pi.er != none)
        return pi;
    if (t.tp != ID)
    {
        pi.er = idExpected;
        pi.tk = t;
        return pi;
    }

    // 符号表中加入 class
    if (FindSymbol(t.lx, false))
    {
        pi.er = redecIdentifier;
        pi.tk = t;
        return pi;
    }
    InsertSymbol(t.lx, CLASS, STATIC, t.lx);
    // 进入类作用域
    EnterSubScope(t.lx);
    // DEBUG
    //printf("PreScanClassMembers: %s, Scope now: %d\n", t.lx, getCurrentScope());

    // 检查 "{"
    t = GetNextToken();
    pi = lexerError(t);
    if (pi.er != none)
        return pi;
    if (t.tp != SYMBOL || strcmp(t.lx, "{") != 0)
    {
        pi.er = openBraceExpected;
        pi.tk = t;
        return pi;
    }

    t = PeekNextToken();
    while (!(t.tp == SYMBOL && strcmp(t.lx, "}") == 0))
    {
        if (t.tp == RESWORD &&
            (!strcmp(t.lx, "static") || !strcmp(t.lx, "field")))
        {
            pi = PreScanClassVarDeclar();
            if (pi.er != none)
                return pi;
        }
        else if (t.tp == RESWORD &&
                 (!strcmp(t.lx, "constructor") ||
                  !strcmp(t.lx, "function") ||
                  !strcmp(t.lx, "method")))
        {
            pi = PreScanSubroutineDeclar();
            if (pi.er != none)
                return pi;
        }
        else
        {
            pi.er = memberDeclarErr;
            pi.tk = t;
            return pi;
        }
        t = PeekNextToken();
    }
    ExitScope();
    return pi;
}

// 针对单个文件进行预扫描（预加载类名）
ParserInfo PreScanFile(char *file_path) {
    ParserInfo pi;
    if (InitParser(file_path) != 1 || InitLexer(file_path) != 1) {
        printf("Failed to initialize Lexer or Parser for file: %s\n", file_path);
        pi.er = lexerErr;
        return pi;
    }
    pi = PreScanClassMembers();
    StopParser();
	StopLexer();
    return pi;
}

// 修改 PreloadLibraries：对目录内每个 .jack 文件预扫描（预加载类名）
int PreloadLibraries(const char *dir_name) {
    DIR *dir = opendir(dir_name);
    if (dir == NULL) {
        printf("Failed to open directory %s.\n", dir_name);
        return 0;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // 跳过 '.' 和 '..'
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        if (!isJackFile(entry->d_name))
            continue;
        char file_path[256];
        snprintf(file_path, sizeof(file_path), "%s/%s", dir_name, entry->d_name);
        ParserInfo pi = PreScanFile(file_path);
        if (pi.er != none) {
            printf("Error during pre-scan in file: %s\n", file_path);
            closedir(dir);
            return 0;
        }
    }
    closedir(dir);
    return 1;
}

// 对单个文件进行完整解析
ParserInfo ParseFile(char *file_path) {
    ParserInfo pi;
    if (InitParser(file_path) != 1 || InitLexer(file_path) != 1) {
        printf("Failed to initialize Lexer or Parser for file: %s\n", file_path);
        pi.er = lexerErr;
        return pi;
    }
    pi = Parse();
    StopParser();
	StopLexer();
    return pi;
}

// 修改 compile：先进行预扫描，再完整解析所有 .jack 文件
ParserInfo compile(char *dir_name)
{
    ParserInfo p;
    p.er = none;

    // 第一遍：预加载所有类名到符号表
    DIR *dir = opendir(dir_name);
    if (dir == NULL)
    {
        p.er = lexerErr;
        printf("Failed to open directory %s.\n", dir_name);
        return p;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        // 跳过 '.' 和 '..'
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        if (!isJackFile(entry->d_name))
            continue;
        char file_path[256];
        snprintf(file_path, sizeof(file_path), "%s/%s", dir_name, entry->d_name);
        ParserInfo pi = PreScanFile(file_path);
        if (pi.er != none)
        {
            printf("Failed to pre-scan file: %s\n", file_path);
            closedir(dir);
            return pi;
        }
    }
    closedir(dir);

    // 第二遍：对每个文件进行完整解析
    dir = opendir(dir_name);
    if (dir == NULL) {
        printf("Failed to open directory %s.\n", dir_name);
        p.er = lexerErr;
        return p;
    }
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        if (!isJackFile(entry->d_name))
            continue;
        char file_path[256];
        snprintf(file_path, sizeof(file_path), "%s/%s", dir_name, entry->d_name);
        p = ParseFile(file_path);
        if (p.er != none)
        {
            printf("Error in file: %s\n", file_path);
            closedir(dir);
            return p;
        }
    }
    closedir(dir);
    return p;
}


int InitCompiler() {
    // 初始化符号表
    InitSymbolTable();

    // 预加载system lib
    if (!PreloadLibraries(".")) {
        printf("Failed to preload libraries.\n");
        return 0;
    }
    return 1;
}

int StopCompiler()
{
    StopParser();
    StopLexer();
    return 1;
}

#ifndef TEST_COMPILER

char *ErrorString(SyntaxErrors e)
{
    switch (e)
    {
    case none:
        return "no errors";
    case lexerErr:
        return "lexer error";
    case classExpected:
        return "keyword class expected";
    case idExpected:
        return "identifier expected";
    case openBraceExpected:
        return "{ expected";
    case closeBraceExpected:
        return "} expected";
    case memberDeclarErr:
        return "class member declaration must begin with static, field, constructor, function, or method";
    case classVarErr:
        return "class variables must begin with field or static";
    case illegalType:
        return "a type must be int, char, boolean, or identifier";
    case semicolonExpected:
        return "; expected";
    case subroutineDeclarErr:
        return "subroutine declaration must begin with constructor, function, or method";
    case openParenExpected:
        return "( expected";
    case closeParenExpected:
        return ") expected";
    case closeBracketExpected:
        return "] expected";
    case equalExpected:
        return "= expected";
    case syntaxError:
        return "syntax error";
    // semantic errors
    case undecIdentifier:
        return "undeclared identifier";
    case redecIdentifier:
        return "redeclaration of identifier";
    default:
        return "not a valid error code";
    }
}

void PrintError(ParserInfo pn)
{
    if (pn.er == none)
        printf("No errors\n");
    else
        printf("Error in file %s line %i at or near %s: %s\n", pn.tk.fl, pn.tk.ln, pn.tk.lx, ErrorString(pn.er));
}

int main()
{
    printf("Testing your compiler\n");
    // 初始化符号表
    InitSymbolTable();
    
    // 预加载系统库（假设当前目录为系统库路径）
    if (!PreloadLibraries("."))
    {
        printf("Failed to preload libraries.\n");
        return 0;
    }
    
    printf("Compiling files in Pong directory\n");
    ParserInfo p = compile("Pong");
    printf("Show error\n");
    PrintError(p);
    StopCompiler();
    return 1;
}
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"
#include "parser.h"
#include "compiler.h"

#define JsonStrSize 5000
#define Presubmission 1

// remove this before releasing template
#define NumberTestFiles 10
char* JsonStr;

Token tokenArray[2024];
int numberTokens;

int LineNum;
FILE* CurVMFile;
char CurVMFileName [128];

#define NumCommands  17
char* Commands[NumCommands]= {"add" , "sub" ,"neg" ,"eq" ,"gt" ,"lt" ,"and" ,"or" ,"not" ,"push" ,"pop" ,"label" ,"goto" , "if-goto" , "function" , "call" , "return" };

char* code_files[128];  // list of code files to be checked

char* testPrograms[NumberTestFiles] = {
	"Seven",
	"Fraction",
	"HelloWorld",
	"Square",
	"Average",
	"ArrayTest",
	"MathTest",
	"List",
	"ConvertToBin",
	"Pong"
};

int InitVMLexer (char* file_name)
{
  CurVMFile = fopen (file_name, "r");
  if (CurVMFile == 0)
  {
    printf ("Unable to open code file %s\n", file_name);
    return (0);
  }
  strcpy (CurVMFileName , file_name);
  LineNum = 1;
  return 1;
}

int StopVMLexer ()
{
	fclose (CurVMFile);
	return 1;
}


int InitGraderString ()
{
	JsonStr = (char *) malloc (sizeof (char) * JsonStrSize);
	strcpy (JsonStr, "{\n");   // let's start, bismillah
	strcat (JsonStr, "\t\"output\": \"Graded by CAutoGrader\",\n");
	strcat (JsonStr, "\t\"stdout_visibility\": \"visible\",\n");
	strcat (JsonStr, "\t\"tests\":\n");  // let's start the tests
	strcat (JsonStr, "\t[\n");
	return 1;
}

int AddTestString (int score, int max_score, char* output, int is_final)
{
	char temp[320];

	strcat (JsonStr , "\t\t{\n");
	sprintf (temp, "%i", score);
	strcat (JsonStr , "\t\t\t\"score\": ");
	strcat (JsonStr , temp);
	strcat (JsonStr , ",\n");

	sprintf (temp, "%i", max_score);
	strcat (JsonStr , "\t\t\t\"max_score\": ");
	strcat (JsonStr , temp);
	strcat (JsonStr , ",\n");


	strcat (JsonStr , "\t\t\t\"output\": ");
	strcat (JsonStr , "\"");
	strcat (JsonStr , output);
	strcat (JsonStr , "\"");
	strcat (JsonStr , "\n");

	strcat (JsonStr , "\t\t}");
	if (!is_final)
		strcat (JsonStr, ",");
	strcat (JsonStr, "\n");

	return 1;
}


// get a list of code (vm) files in a compiled jack program given by its directory name (dir_name)
int GetCodeFiles (char* dir_name)
{
	char tmp[256];
	sprintf (tmp, "ls %s > SourceFiles.txt", dir_name);
	system (tmp);
	// use my lexer to get the file names
	InitLexer ("SourceFiles.txt");
	Token t =  PeekNextToken ();
	int i = 0;
	while (t.tp != EOFile)
	{
		t = GetNextToken(); // get the file name without extension
		strcpy (tmp , t.lx);
		t = GetNextToken() ; // get the .
		strcat (tmp, t.lx);
		t = GetNextToken();  // get the extension
		strcat (tmp, t.lx);
		// if the extension is vm then add this file to the list
		if (!strcmp(t.lx, "vm"))
		{
			code_files[i]= (char*) malloc (sizeof (char) * (strlen (tmp)  ));
			strcpy (code_files[i++], tmp);
		}
		t = PeekNextToken();
	}
	StopLexer();
	return i;
}



int IsVMCommand (char* word)
{
  for (int i = 0; i < NumCommands ; i++)
    if (strcmp (Commands[i],word) == 0)
		return 1;
  return 0;
}

// eat white space
int EatSpace ()
{
  int c;
  c = getc (CurVMFile);  // get the next char from the input file
  while (c != EOF && isspace (c) ) // keep eating white space
  {
	if (c == '\n')
		LineNum++;
    c = getc (CurVMFile); // move ahead and get next char
  }
  // when main loop  is broken then c contains next non-space char
  return c;
}


Token GetVMToken ()
{
  int c;
  Token t;
  char s[256];
  int i;

  strcpy (t.fl , CurVMFileName);
  // consume leading white-space
  c =  EatSpace();

  if (c == EOF)
  {
    t.tp = EOFile;
	strcpy (t.lx , "End of File");
	t.ln = LineNum;
    return t;
  }
  else if (isdigit (c)) // a number
  {
    i=0;
    while (c != EOF && isdigit (c))
    {
      s[i++] = c;
      c = getc (CurVMFile);
    }
    ungetc(c, CurVMFile);
    s[i] = '\0';
    t.tp = INT;
    strcpy (t.lx , s);
	t.ln = LineNum;
    return t;
  }
  else if (isalpha (c)) //|| c == '-' || c == '_'
  {
    i=0;
    while (c != EOF && (isalpha (c) || isdigit (c) || c == '-' || c == '_'))
    {
      s[i++] = c;
      c = getc (CurVMFile);
    }
    ungetc(c, CurVMFile);
    s[i] = '\0';
	if (IsVMCommand(s))
      t.tp = RESWORD;
    else
      t.tp = ID;
    strcpy (t.lx , s);
	t.ln = LineNum;
    return t;
  }
  else
  {
	if (c != '.' )
	{
      t.tp = ERR;
	  strcpy (t.lx , "Error: illegal symbol in source file");
	  t.ec = IllSym;
	  t.ln = LineNum;
      return t;
	}
    t.tp = SYMBOL;
    s[0] = c;
    s[1] = '\0';
    strcpy (t.lx , s);
	t.ln = LineNum;
    return t;
  }
}

int CloseGraderString ()
{
	strcat (JsonStr, "	]\n"); // end of tests
	strcat (JsonStr, "}\n");
	return 1;
}

char* TokenTypeString (TokenType t)
{
	switch (t)
	{
		case RESWORD: return "RESWORD";
		case ID: return "ID";
		case INT: return "INT";
		case SYMBOL: return "SYMBOL";
		case STRING: return "STRING";
		case EOFile: return "EOFile";
		case ERR: return "ERR";
		default: return "Not a recognised token type";
	}

}

char* ErrorString (SyntaxErrors e)
{

	switch (e)
	{
		case none: return "no errors";
		case lexerErr: return "lexer error";
		case classExpected: return "keyword class expected";
		case idExpected: return "identifier expected";
		case openBraceExpected:	return "{ expected";
		case closeBraceExpected: return "} expected";
		case memberDeclarErr: return "class member declaration must begin with static, field, constructor , function , or method";
		case classVarErr: return "class variables must begin with field or static";
		case illegalType: return "a type must be int, char, boolean, or identifier";
		case semicolonExpected: return "; expected";
		case subroutineDeclarErr: return "subrouting declaration must begin with constructor, function, or method";
		case openParenExpected: return "( expected";
		case closeParenExpected: return ") expected";
		case closeBracketExpected: return "] expected";
		case equalExpected: return "= expected";
		case syntaxError: return "syntax error";
		// semantic errors
		case undecIdentifier: return "undeclared identifier";
		case redecIdentifier: return "redeclaration of identifier";
		default: return "not a valid error code";
	}
}


void PrintError (ParserInfo pn)
{
	if (pn.er == none)
		printf ("No errors\n");
	else
		printf ("Error in file %s line %i at or near %s: %s\n" , pn.tk.fl , pn.tk.ln , pn.tk.lx , ErrorString(pn.er));
}


void ShowInfo (ParserInfo pn)
{
	if (pn.er == none)
		printf ("none\n");
	else
		printf ("error type: %s, line: %i,token: %s, \n" , ErrorString(pn.er),  pn.tk.ln , pn.tk.lx  );
}


void PrintToken (Token t)
{
	printf ("<%s, %i, %s, %s>\n", t.fl, t.ln , t.lx, TokenTypeString (t.tp));
}


// test the parser
int t_compiler ()
{
	int m=20;
	char s[100];

	printf ("\nTesting your compiler on various JACK programs (2 marks each)\n");
	for (int j = 0 ; j < NumberTestFiles ; j++) // for each test file
	{
		printf ("Running your compiler on program %s \n", testPrograms[j]);
		InitCompiler ();
		ParserInfo p = compile (testPrograms[j]);
		StopCompiler ();
		if (p.er != none)
		{
			printf ("** Oops: your compiler failed to compile the program and returned the following error:\n");
			PrintError (p);
			printf ("Sorry, -2 mark\n");
			m=m-2;
		}
		else
		{
			// get the files of the benchmark compilation
			char full_dir_name[128];
			strcpy (full_dir_name , testPrograms[j]);
			strcat (full_dir_name , "_compiled");
			int num_vm_files = GetCodeFiles (full_dir_name);
			for (int k = 0; k < num_vm_files ; k++)  // compare each vm file to the one generated by the compiler
			{
				printf ("Checking file %s\n", code_files[k]);
				// get the tokens of the benchmark file
				char vm_file_name[128];
				strcpy (vm_file_name , testPrograms[j]);
				strcat (vm_file_name , "_compiled/");
				strcat (vm_file_name , code_files[k]);
				int r  = InitVMLexer (vm_file_name);
				if (r == 0)
				{
					printf ("Fatal autograder error, unable to open a vm file\n");
					exit (0);
				}
				// now extract the tokens of the standard vm file an store in an array
				numberTokens = 0;
				Token t = GetVMToken ();
				while (t.tp != EOFile)
				{
					tokenArray[numberTokens++] = t;
					t = GetVMToken ();
				}
				StopVMLexer();
				//  now compare the tokens with those of the file created by the compiler
				strcpy (vm_file_name , testPrograms[j]);
				strcat (vm_file_name , "/");
				strcat (vm_file_name, code_files[k]);
				r = InitVMLexer (vm_file_name);
				if (r == 0)
				{
					printf ("Unable to open one of your vm files. Did your compiler create all code files?\n");
					m-=2;
					StopVMLexer();
					break;
				}
				int i = 0;
				t = GetVMToken ();
				Token prev_token;
				strcpy (prev_token.lx , "nothing");
				while (t.tp != EOFile)
				{
					//printf ("%s =? %s\n", t.lx, tokenArray[i].lx);
					if (!strcmp (prev_token.lx,"label") || !strcmp (prev_token.lx,"goto") || !strcmp (prev_token.lx,"if-goto") )
					{
						;  // ignore labels
					}
					else if ( strcmp(t.lx , tokenArray[i].lx))
					{
						printf ("Error in code file\n");
						printf ("Expecting %s in line %i\n" , tokenArray[i].lx, tokenArray[i].ln);
						printf ("Found %s in line %i instead\n", t.lx, t.ln);
						printf ("Sorry, -2 marks\n");
						m=m-2;
						break;
					}
					i++;
					prev_token = t;
					t = GetVMToken ();
				}
				StopVMLexer ();
				printf ("*** PASSED *** \n");
			}
		}
	}
	sprintf (s,"%i/20 for code generation", m);
	if (!Presubmission)
		AddTestString (m, 20, s, 1);
	return m;
}


#ifdef TEST_COMPILER
int main (int argc, char* argv[])
{
	FILE* jsonFile;
	int tot = 0;


	if (!Presubmission)
		InitGraderString ();

	printf ("\t$$$ Checking your compiler, behold $$$\n");
	printf ("\t=========================================\n");
	printf ("Started ...\n");

	tot += t_compiler ();

	if (Presubmission)
	{
		printf ("\n---------------------------------------------------\n");
		printf ("\t\tTotal mark = %i/20\n", tot);
		printf ("---------------------------------------------------\n\n");
	}
	printf ("Finished\n");


	if (!Presubmission)  // create the results.json file
	{
		CloseGraderString ();
		//printf ("%s",JsonStr);
		//jsonFile = fopen ("/autograder/results/results.json", "w");
		//fprintf (jsonFile, "%s", JsonStr);
		//fclose (jsonFile);
	}


	return 0;
}
#endif
