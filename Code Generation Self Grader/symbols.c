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
