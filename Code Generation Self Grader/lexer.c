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
