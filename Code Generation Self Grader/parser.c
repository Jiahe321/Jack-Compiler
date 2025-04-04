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