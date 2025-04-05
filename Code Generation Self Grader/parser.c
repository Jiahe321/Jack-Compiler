#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "lexer.h"
#include "parser.h"
#include "symbols.h"
#include "compiler.h"

// you can declare prototypes of parser functions below
extern FILE *file;

// 生成循环开始和结束标签
int whileLabelCount = 0;
int ifLabelCount = 0;
int currentCallArgCount = 0;

// 存储当前类名
static char currentClassName[128] = "";
static char currentSubroutineName[128] = "";
// 在文件开头（例如在 currentSubroutineName 后）新增：
Kinds currentSubroutineKind;

void ResetCounters();
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

void ResetCounters()
{
	whileLabelCount = 0;
	ifLabelCount = 0;
	currentCallArgCount = 0;
}

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
	strncpy(currentClassName, t.lx, sizeof(currentClassName) - 1);
	currentClassName[sizeof(currentClassName) - 1] = '\0';
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
	char type_name[128] = ""; // 创建局部缓冲区
	if (dataType == CLASS)
	{
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

	// 存储当前子例程类型，并保存到全局变量 currentSubroutineKind
	Kinds kind = parseKind(t);
	currentSubroutineKind = kind;
	ScopeType scope = parseScopeType(t);

	t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp == RESWORD && !strcmp(t.lx, "void"))
	{
		// void 无需额外处理
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
	// 获取返回类型对应的 DataTypes
	DataTypes dataType = parseDataType(t);
	char type_name[128] = "";
	if (dataType == CLASS)
	{
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
	// 保存子例程名到全局变量
	strncpy(currentSubroutineName, t.lx, sizeof(currentSubroutineName) - 1);
	currentSubroutineName[sizeof(currentSubroutineName) - 1] = '\0';
	// 进入子作用域
	EnterSubScope(t.lx);
	if (currentSubroutineKind == CONSTRUCTOR)
	{
		// 对于构造函数，不希望自动插入 this，
		// 将参数计数器重置为 0，使得第一个显式参数成为 ARG0
		current_table->arg_idx = 0;
	}
	ResetCounters();

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
		if (paramType == CLASS)
		{
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

	// 处理函数体开始部分的连续 var 声明
	while (1)
	{
		t = PeekNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
		if (t.tp == RESWORD && strcmp(t.lx, "var") == 0)
		{
			pi = varDeclarStatement();
			if (pi.er != none)
				return pi;
		}
		else
		{
			break;
		}
	}

	// 生成 function 命令，函数名格式为 "ClassName.SubroutineName"
	char functionName[256];
	sprintf(functionName, "%s.%s", currentClassName, currentSubroutineName);
	// 当前子符号表中的 var_idx 存储局部变量个数
	writeFunction(functionName, current_table->var_idx);

	// 如果是构造函数，则需要分配内存
	if (currentSubroutineKind == METHOD)
	{
		writePush("argument", 0); // 对吗？
		writePop("pointer", 0);
	}
	else if (currentSubroutineKind == CONSTRUCTOR)
	{
		int field_count = current_table->parent->field_idx;
		writePush("constant", field_count);
		writeCall("Memory.alloc", 1);
		writePop("pointer", 0); // 基地址存入 this
	}

	// 解析其余语句
	while (1)
	{
		t = PeekNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
		if (t.tp == SYMBOL && strcmp(t.lx, "}") == 0)
			break;
		pi = statement();
		if (pi.er != none)
			return pi;
	}
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
	char type_name[128] = ""; // 创建局部缓冲区
	if (dataType == CLASS)
	{
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

	// 获取变量名并查找符号
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
	Symbol *sym = FindSymbol(t.lx, true);
	if (sym == NULL)
	{
		pi.er = undecIdentifier;
		pi.tk = t;
		return pi;
	}

	// 根据符号类型决定对应的VM段
	const char *segment;
	switch (sym->kind)
	{
	case FIELD:
		segment = "this";
		break;
	case VAR:
		segment = "local";
		break;
	case ARG:
		segment = "argument";
		break;
	case STATIC:
		segment = "static";
		break;
	default:
		segment = "local";
		break;
	}

	// 检查是否为数组赋值
	bool isArray = false;
	t = PeekNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp == SYMBOL && strcmp(t.lx, "[") == 0)
	{
		isArray = true;
		GetNextToken(); // 消耗 "["
		// 先计算下标表达式
		pi = expression();
		if (pi.er != none)
			return pi;
		t = GetNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
		if (t.tp != SYMBOL || strcmp(t.lx, "]") != 0)
		{
			pi.er = closeBracketExpected;
			pi.tk = t;
			return pi;
		}
		// 压入数组基地址，并计算出有效地址：基地址 + 下标
		writePush(segment, sym->index);
		writeArithmetic("add");
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

	// 编译右侧表达式，结果留在栈顶
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

	// 生成 VM 代码
	if (isArray)
	{
		// 数组赋值：
		// 1. 此时栈顶为右值，将其保存到 temp 0
		writePop("temp", 0);
		// 2. 之前已计算出数组基地址+下标的目标地址，弹入 pointer 1 (即 THAT)
		writePop("pointer", 1);
		// 3. 将保存的右值从 temp 0 推回，并赋给 THAT 0
		writePush("temp", 0);
		writePop("that", 0);
	}
	else
	{
		// 普通变量赋值：直接将右值弹入对应段的地址中
		writePop(segment, sym->index);
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
	if (t.tp != RESWORD || strcmp(t.lx, "if") != 0)
	{
		pi.er = syntaxError;
		pi.tk = t;
		return pi;
	}

	// 检查 "("
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

	// 解析条件表达式，条件值将留在栈上
	pi = expression();
	if (pi.er != none)
		return pi;

	// 检查 ")"
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

	char trueLabel[32], falseLabel[32], endLabel[32];
	sprintf(trueLabel, "IF_TRUE%d", ifLabelCount);
	sprintf(falseLabel, "IF_FALSE%d", ifLabelCount);
	sprintf(endLabel, "IF_END%d", ifLabelCount);
	ifLabelCount++;

	// 生成 if 语句对应的 VM 代码：
	// 如果条件为真则跳转到 trueLabel，否则跳转到 falseLabel
	writeIf(trueLabel);
	writeGoto(falseLabel);
	// 标记 then 分支入口
	writeLabel(trueLabel);

	// 解析 then 分支，要求后面紧跟 "{"
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
	while (1)
	{
		t = PeekNextToken();
		pi = lexerError(t);
		if (pi.er != none)
			return pi;
		if (t.tp == SYMBOL && strcmp(t.lx, "}") == 0)
			break;
		pi = statement();
		if (pi.er != none)
			return pi;
	}
	// 消耗 then 分支结束的 "}"
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

	// 检查是否有 else 分支
	t = PeekNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;
	if (t.tp == RESWORD && strcmp(t.lx, "else") == 0)
	{
		// 有 else 分支，先消耗 "else"
		GetNextToken();

		// then 分支结束后跳转到 if 语句结尾
		writeGoto(endLabel);
		// 标记 else 分支入口
		writeLabel(falseLabel);

		// 解析 else 分支：要求后面紧跟 "{"
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
		while (1)
		{
			t = PeekNextToken();
			pi = lexerError(t);
			if (pi.er != none)
				return pi;
			if (t.tp == SYMBOL && strcmp(t.lx, "}") == 0)
				break;
			pi = statement();
			if (pi.er != none)
				return pi;
		}
		// 消耗 else 分支结束的 "}"
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
		// 标记 if 语句整体结束位置
		writeLabel(endLabel);
	}
	else
	{
		// 无 else 分支，falseLabel即为 if 语句出口
		writeLabel(falseLabel);
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

	char labelExp[64], labelEnd[64];
	sprintf(labelExp, "WHILE_EXP%d", whileLabelCount);
	sprintf(labelEnd, "WHILE_END%d", whileLabelCount);
	whileLabelCount++;

	// 生成循环入口标签
	writeLabel(labelExp);

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

	writeArithmetic("not");
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

	// 条件为 false 跳转到循环结束
	writeIf(labelEnd);

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
	// 循环结束前无条件跳转回循环入口
	writeGoto(labelExp);
	// 循环结束标签
	writeLabel(labelEnd);
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

	// 调用完成后，丢弃返回值：pop temp 0
	writePop("temp", 0);
	return pi;
}

// subroutineCall → identifier [ . identifier ] ( expressionList )
ParserInfo subroutineCall()
{
	ParserInfo pi;
	pi.er = none;
	// 读取第一个标识符
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
	// 保存左侧标识符（可能是对象变量或类名）
	char leftName[128];
	strncpy(leftName, t.lx, sizeof(leftName) - 1);
	leftName[sizeof(leftName) - 1] = '\0';

	// 先查找该标识符是否存在，但允许未找到（此时视为类名调用）
	Symbol *leftSym = FindSymbol(leftName, true);

	Token next = PeekNextToken();
	pi = lexerError(next);
	if (pi.er != none)
		return pi;

	// 用于构造完整调用函数名
	char fullFunctionName[256] = "";
	// 用于记录参数个数
	int argCount = 0;
	bool isObjectCall = false; // 表示是否为对象调用，需要自动 push 对象指针

	// 点调用形式： identifier . identifier ( expressionList )
	if (next.tp == SYMBOL && strcmp(next.lx, ".") == 0)
	{
		GetNextToken(); // 消耗 "."

		// 判断 leftSym 是否为对象变量
		if (leftSym != NULL && (leftSym->kind == FIELD || leftSym->kind == ARG || leftSym->kind == VAR))
		{
			isObjectCall = true;
			// 根据变量类型选择段：一般 FIELD 存在于当前对象，用 "this"，否则可用 "local"
			if (leftSym->kind == FIELD)
				writePush("this", leftSym->index);
			else
				writePush("local", leftSym->index);
		}
		// 保存当前作用域，用于恢复
		SymbolTable *old_table = current_table;
		// 进入子作用域：如果是对象调用，进入对象所属类型的子作用域；
		// 否则按照 leftName（视为类名）进入子作用域（仅为查找方法符号，不生成 push）
		if (isObjectCall)
			EnterSubScope(leftSym->type_name);
		else
			EnterSubScope(leftName);

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

		// 在子符号表中查找方法（METHOD 或 FUNCTION）
		Symbol *method_sym = FindSymbol(t.lx, true);
		if (!method_sym)
		{
			pi.er = undecIdentifier;
			pi.tk = t;
			return pi;
		}

		// 构造完整调用函数名
		if (isObjectCall)
			snprintf(fullFunctionName, sizeof(fullFunctionName), "%s.%s", leftSym->type_name, t.lx);
		else
			snprintf(fullFunctionName, sizeof(fullFunctionName), "%s.%s", leftName, t.lx);

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

		// 完成点调用后退出子作用域，恢复原作用域
		ExitScope();
		current_table = old_table;

		// 解析参数列表（更新全局 currentCallArgCount）
		pi = expressionList();
		if (pi.er != none)
			return pi;
		argCount = currentCallArgCount;
		// 如果是对象调用，已自动压入对象指针，所以参数数加1
		if (isObjectCall)
			argCount += 1;

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

		// 生成 call 指令
		writeCall(fullFunctionName, argCount);
		return pi;
	}
	// 直接调用形式： identifier ( expressionList )
	else if (next.tp == SYMBOL && strcmp(next.lx, "(") == 0)
	{
		// 对直接调用，视为当前对象的方法调用：先压入当前对象指针
		writePush("pointer", 0);
		isObjectCall = true;

		// 构造完整调用函数名：当前类名 + "." + 方法名
		snprintf(fullFunctionName, sizeof(fullFunctionName), "%s.%s", currentClassName, leftName);

		// 消耗 "("
		GetNextToken(); // 消耗 "("

		pi = expressionList();
		if (pi.er != none)
			return pi;
		argCount = currentCallArgCount;
		// 对于对象调用，加上 this 指针，参数数加1
		argCount += 1;

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

		// 生成 call 指令
		writeCall(fullFunctionName, argCount);
		return pi;
	}
	return pi;
}

// expressionList → expression { , expression } | ε
ParserInfo expressionList()
{
	ParserInfo pi;
	pi.er = none;

	currentCallArgCount = 0; // NEW 重置参数计数

	Token t = PeekNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;

	if (t.tp == SYMBOL && !strcmp(t.lx, ")"))
		return pi;

	pi = expression();
	if (pi.er != none)
		return pi;
	currentCallArgCount++; // new 增加参数

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
		currentCallArgCount++; // new 增加参数
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
		// NEW 无返回值：默认推入 0
		writePush("constant", 0);
	}
	else if (t.tp == INT || t.tp == STRING ||
			 t.tp == ID ||
			 (t.tp == RESWORD &&
			  (!strcmp(t.lx, "true") || !strcmp(t.lx, "false") ||
			   !strcmp(t.lx, "null") || !strcmp(t.lx, "this"))) ||
			 (t.tp == SYMBOL &&
			  (!strcmp(t.lx, "(") || !strcmp(t.lx, "-") || !strcmp(t.lx, "~"))))
	{
		// TODO 有返回值，计算表达式，结果留在栈顶
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
	writeReturn();
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
		// 保存运算符
		char op[3];
		strncpy(op, t.lx, sizeof(op) - 1);
		op[sizeof(op) - 1] = '\0';
		GetNextToken(); // 消耗运算符
		pi = relationalExpression();
		if (pi.er != none)
			return pi;
		// 生成 VM 指令：逻辑与或或
		if (strcmp(op, "&") == 0)
			writeArithmetic("and");
		else if (strcmp(op, "|") == 0)
			writeArithmetic("or");
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
		// 保存运算符
		char op[3];
		strncpy(op, t.lx, sizeof(op) - 1);
		op[sizeof(op) - 1] = '\0';
		GetNextToken(); // 消耗运算符
		pi = arithmeticExpression();
		if (pi.er != none)
			return pi;
		// 生成 VM 指令：等于、大小比较
		if (strcmp(op, "=") == 0)
			writeArithmetic("eq");
		else if (strcmp(op, ">") == 0)
			writeArithmetic("gt");
		else if (strcmp(op, "<") == 0)
			writeArithmetic("lt");
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
		// 保存运算符
		char op[3];
		strncpy(op, t.lx, sizeof(op) - 1);
		op[sizeof(op) - 1] = '\0';
		GetNextToken(); // 消耗运算符
		pi = term();
		if (pi.er != none)
			return pi;
		// 生成 VM 指令：加法或减法
		if (strcmp(op, "+") == 0)
			writeArithmetic("add");
		else if (strcmp(op, "-") == 0)
			writeArithmetic("sub");
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
		// 保存运算符
		char op[3];
		strncpy(op, t.lx, sizeof(op) - 1);
		op[sizeof(op) - 1] = '\0';
		GetNextToken(); // 消耗运算符
		pi = factor();
		if (pi.er != none)
			return pi;
		// 生成 VM 指令：乘法和除法分别使用 Math.multiply 和 Math.divide 函数
		if (strcmp(op, "*") == 0)
			writeCall("Math.multiply", 2);
		else if (strcmp(op, "/") == 0)
			writeCall("Math.divide", 2);
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

	bool unaryOp = false;
	if (t.tp == SYMBOL && (strcmp(t.lx, "-") == 0 || strcmp(t.lx, "~") == 0))
	{
		unaryOp = true;
		GetNextToken(); // 消耗运算符
	}

	pi = operand();
	if (pi.er != none)
		return pi;

	if (unaryOp)
	{
		if (strcmp(t.lx, "-") == 0)
			writeArithmetic("neg");
		else
			writeArithmetic("not");
	}

	return pi;
}

// operand → integerConstant | identifier [.identifier ] [ [ expression ] | (expressionList ) ] | (expression) | stringLiteral | true | false | null | this
// 修改：完成普通标识符引用、数组访问和直接方法调用的 VM 代码生成
ParserInfo operand()
{
	ParserInfo pi;
	pi.er = none;
	Token t = GetNextToken();
	pi = lexerError(t);
	if (pi.er != none)
		return pi;

	// 整数常量
	if (t.tp == INT)
	{
		int value = atoi(t.lx);
		writePush("constant", value);
		return pi;
	}
	// 字符串常量
	else if (t.tp == STRING)
	{
		char *str = t.lx;
		int str_len = strlen(str);
		writePush("constant", str_len);
		writeCall("String.new", 1);
		for (int i = 0; i < str_len; i++)
		{
			writePush("constant", str[i]);
			writeCall("String.appendChar", 2);
		}
		return pi;
	}
	// 处理 true/false/null/this
	else if (t.tp == RESWORD)
	{
		if (strcmp(t.lx, "true") == 0)
		{
			writePush("constant", 0);
			writeArithmetic("not"); // true 是 ~0
		}
		else if (strcmp(t.lx, "false") == 0 || strcmp(t.lx, "null") == 0)
		{
			writePush("constant", 0);
		}
		else if (strcmp(t.lx, "this") == 0)
		{
			writePush("pointer", 0);
		}
		else
		{
			pi.er = syntaxError;
			pi.tk = t;
		}
		return pi;
	}
	// 标识符相关情况
	else if (t.tp == ID)
	{
		// 检查标识符是否已声明
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

		// 处理数组访问，例如 arr[expression]
		if (next.tp == SYMBOL && strcmp(next.lx, "[") == 0)
		{
			const char *segment;
			switch (var_sym->kind)
			{
			case FIELD:
				segment = "this";
				break;
			case VAR:
				segment = "local";
				break;
			case ARG:
				segment = "argument";
				break;
			case STATIC:
				segment = "static";
				break;
			default:
				segment = "local";
				break;
			}
			// 修改后的数组访问代码
			GetNextToken(); // 消耗 "["
			pi = expression();
			if (pi.er != none)
				return pi;
			Token closing = GetNextToken();
			if (closing.tp != SYMBOL || strcmp(closing.lx, "]") != 0)
			{
				pi.er = closeBracketExpected;
				pi.tk = closing;
				return pi;
			}
			writePush(segment, var_sym->index); // 先计算下标，再压入基地址
			writeArithmetic("add");
			writePop("pointer", 1);
			writePush("that", 0);
			return pi;
		}
		// 处理对象方法调用，例如 obj.method(...)
		else if (next.tp == SYMBOL && strcmp(next.lx, ".") == 0)
		{
			GetNextToken(); // 消耗 "."
			Token method_name = GetNextToken();
			if (method_name.tp != ID)
			{
				pi.er = idExpected;
				pi.tk = method_name;
				return pi;
			}
			// 如果 var_sym 是对象变量，则先压入对象指针
			if (var_sym->kind == VAR || var_sym->kind == ARG || var_sym->kind == FIELD)
			{
				const char *segment;
				switch (var_sym->kind)
				{
				case FIELD:
					segment = "this";
					break;
				case VAR:
					segment = "local";
					break;
				case ARG:
					segment = "argument";
					break;
				default:
					segment = "local";
					break;
				}
				writePush(segment, var_sym->index);
				// 保存当前全局参数计数，归零处理调用参数列表
				int savedArgCount = currentCallArgCount;
				currentCallArgCount = 0;
				// 消耗 "("
				Token paren = GetNextToken();
				if (paren.tp != SYMBOL || strcmp(paren.lx, "(") != 0)
				{
					pi.er = openParenExpected;
					pi.tk = paren;
					return pi;
				}
				pi = expressionList();
				if (pi.er != none)
					return pi;
				// 得到本次调用参数数，再恢复外部计数
				int callArgCount = currentCallArgCount;
				currentCallArgCount = savedArgCount;
				// 对象指针已压入，所以总参数数 = 1 + 调用参数数
				int totalArgCount = callArgCount + 1;
				char fullFunctionName[256];
				// 完整调用名：对象所属类型 + "." + 方法名
				snprintf(fullFunctionName, sizeof(fullFunctionName), "%s.%s", var_sym->type_name, method_name.lx);
				Token closeParen = GetNextToken();
				if (closeParen.tp != SYMBOL || strcmp(closeParen.lx, ")") != 0)
				{
					pi.er = closeParenExpected;
					pi.tk = closeParen;
					return pi;
				}
				writeCall(fullFunctionName, totalArgCount);
				return pi;
			}
			else
			{
				// 非对象调用，视为静态调用，不压入对象指针
				int savedArgCount = currentCallArgCount;
				currentCallArgCount = 0;
				Token paren = GetNextToken();
				if (paren.tp != SYMBOL || strcmp(paren.lx, "(") != 0)
				{
					pi.er = openParenExpected;
					pi.tk = paren;
					return pi;
				}
				pi = expressionList();
				if (pi.er != none)
					return pi;
				int callArgCount = currentCallArgCount;
				currentCallArgCount = savedArgCount;
				char fullFunctionName[256];
				snprintf(fullFunctionName, sizeof(fullFunctionName), "%s.%s", t.lx, method_name.lx);
				Token closeParen = GetNextToken();
				if (closeParen.tp != SYMBOL || strcmp(closeParen.lx, ")") != 0)
				{
					pi.er = closeParenExpected;
					pi.tk = closeParen;
					return pi;
				}
				writeCall(fullFunctionName, callArgCount);
				return pi;
			}
		}
		// 处理直接方法调用，例如 method(...)
		else if (next.tp == SYMBOL && strcmp(next.lx, "(") == 0)
		{
			// 直接方法调用视为当前对象方法调用，先压入 this 指针
			writePush("pointer", 0);
			char fullFunctionName[256];
			snprintf(fullFunctionName, sizeof(fullFunctionName), "%s.%s", currentClassName, t.lx);
			int savedArgCount = currentCallArgCount;
			currentCallArgCount = 0;
			// 消耗 "("
			GetNextToken();
			pi = expressionList();
			if (pi.er != none)
				return pi;
			int callArgCount = currentCallArgCount;
			currentCallArgCount = savedArgCount;
			int totalArgCount = callArgCount + 1; // 加入 this 指针
			Token closeParen = GetNextToken();
			if (closeParen.tp != SYMBOL || strcmp(closeParen.lx, ")") != 0)
			{
				pi.er = closeParenExpected;
				pi.tk = closeParen;
				return pi;
			}
			writeCall(fullFunctionName, totalArgCount);
			return pi;
		}
		// 普通变量引用
		else
		{
			const char *segment;
			switch (var_sym->kind)
			{
			case FIELD:
				segment = "this";
				break;
			case VAR:
				segment = "local";
				break;
			case ARG:
				segment = "argument";
				break;
			case STATIC:
				segment = "static";
				break;
			default:
				segment = "local";
				break;
			}
			writePush(segment, var_sym->index);
			return pi;
		}
	}
	else if (t.tp == SYMBOL && strcmp(t.lx, "(") == 0)
	{
		pi = expression();
		if (pi.er != none)
			return pi;
		Token closing = GetNextToken();
		if (closing.tp != SYMBOL || strcmp(closing.lx, ")") != 0)
		{
			pi.er = closeParenExpected;
			pi.tk = closing;
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
	// 归零所有计数器
	ResetCounters();

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
