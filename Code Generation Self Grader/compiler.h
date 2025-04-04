#ifndef COMPILER_H
#define COMPILER_H

#define TEST_COMPILER    // uncomment to run the compiler autograder

#include "parser.h"
#include "symbols.h"

int InitCompiler ();
ParserInfo compile (char* dir_name);
ParserInfo lexerError(Token t);
int StopCompiler();

// utils: 写入 vm 指令
void writePush(const char *segment, int index);
void writePop(const char *segment, int index);
void writeArithmetic(const char *command);
void writeLabel(const char *label);
void writeGoto(const char *label);
void writeIf(const char *label);
void writeCall(const char *name, int nArgs);
void writeFunction(const char *name, int nLocals);
void writeReturn();

#endif
