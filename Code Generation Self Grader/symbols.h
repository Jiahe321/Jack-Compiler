#ifndef SYMBOLS_H
#define SYMBOLS_H

#include "parser.h"
#include "lexer.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SYMBOLS 100

typedef enum {
    INTEGER,
    CHAR,
    BOOLEAN,
    CLASS, // for example, Date defined by user
    VOID
} DataTypes;

typedef enum {
    STATIC, // 静态变量（class）
    FIELD, // 实例变量（class）
    ARG, // 参数（method）
    VAR, // 局部变量（method）

    // subtable
    FUNCTION,
    METHOD,
    CONSTRUCTOR
} Kinds;

typedef enum {
    PROGRAM_SCOPE, // 存储class name
    FUNCTION_SCOPE, // 存储ARG, VAR
    CLASS_SCOPE, // 存储STATIC, FIELD, METHOD, , FUNCTION
    METHOD_SCOPE // 存储ARG, VAR
} ScopeType;


typedef struct Symbol {
    char name[128];
    DataTypes type;
    char type_name[128];  // if type is class, store class name
    Kinds kind;
    int index;
    struct SymbolTable** sub_tables;  // dynamically store multiple subtable
    int sub_table_count; // subtable number
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
ScopeType getCurrentScope(); // 获取当前作用域
void ExitScope(); // 退出当前作用域（返回父符号表）
void FreeSymbolTable(SymbolTable *table);

void InsertSymbol(const char* name, DataTypes type, Kinds kind, const char *type_name);  // 插入符号到当前表
Symbol* FindSymbol(const char* name, bool search_parent); // 查找符号（可向上递归）
void AutoInsertThis(); // 自动插入隐含的 this 参数（用于METHOD)

// Utils
DataTypes parseDataType(Token t);
Kinds parseKind(Token t);
ScopeType parseScopeType(Token t);
int GetAddress(int r);
#endif