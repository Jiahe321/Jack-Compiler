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
