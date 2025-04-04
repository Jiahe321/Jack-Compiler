#ifndef SYMBOLS_H
#define SYMBOLS_H

#include "parser.h"
#include "lexer.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define MAX_SYMBOLS 100

typedef enum {
    INTEGER,
    CHAR,
    BOOLEAN,
    CLASS, // 例如用户定义的 Date 类
    VOID
} DataTypes;

typedef enum {
    STATIC, // 静态变量（class）
    FIELD,  // 实例变量（class）
    ARG,    // 参数（method）
    VAR,    // 局部变量（method）

    // 子表
    FUNCTION,
    METHOD,
    CONSTRUCTOR
} Kinds;

typedef enum {
    PROGRAM_SCOPE, // 存储 class 名称
    FUNCTION_SCOPE, // 存储 ARG, VAR
    CLASS_SCOPE,   // 存储 STATIC, FIELD, METHOD, FUNCTION
    METHOD_SCOPE   // 存储 ARG, VAR
} ScopeType;

typedef struct Symbol {
    char name[128];
    DataTypes type;
    char type_name[128];  // 如果 type 为 CLASS，存放类名
    Kinds kind;
    int index;  // 对应分配的下标，作为内存地址偏移使用
    struct SymbolTable** sub_tables;  // 动态保存多个子表
    int sub_table_count; // 子表个数
} Symbol;

typedef struct SymbolTable {
    ScopeType scope_type;
    struct SymbolTable* parent;
    Symbol symbols[MAX_SYMBOLS];
    int count;
    int static_idx;
    int field_idx;
    int arg_idx;
    int var_idx;
} SymbolTable;

extern SymbolTable* current_table;

void InitSymbolTable();
void EnterSubScope(const char* symbol_name);
ScopeType getCurrentScope();
void ExitScope();
void FreeSymbolTable(SymbolTable *table);

void InsertSymbol(const char* name, DataTypes type, Kinds kind, const char *type_name);
Symbol* FindSymbol(const char* name, bool search_parent);
void AutoInsertThis();

// 新增获取地址函数
// 通过当前符号表中的下标 r 返回对应符号的地址（这里的地址用 index 表示）
int GetAddressByIndex(int r);
// 通过符号名查找符号，并返回其地址（index），找不到时返回 -1
int GetAddress(const char *name);

// Utils
DataTypes parseDataType(Token t);
Kinds parseKind(Token t);
ScopeType parseScopeType(Token t);

#endif
