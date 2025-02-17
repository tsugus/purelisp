/*                                   */
/*          Input / Output           */
/*                                   */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "LISP.H"

Index error(char *str)
{
  err = on;
  message = str;
  sp = 0;
  return 0;
}

Index eof_error()
{
  err = eof;
  message = "EOF was entered.";
  sp = 0;
  return 0;
}

Index gc_getFreeCell()
{
  Index indx;

  if (freecells != CELLS_SIZE - 1) /* 最後尾のセルは使わないフリーセル */
  {
    indx = freecells;
    freecells = cdr(freecells);
    cdr(indx) = 0;
  }
  else
  {
    gc_f(0, 0);
    if (freecells == CELLS_SIZE - 1)
      error("There are no cells available.");
    else
    {
      indx = freecells;
      freecells = cdr(freecells);
      cdr(indx) = 0;
    }
  }
  return indx;
}

int isSeparator(char ch)
{
  switch (ch)
  {
  case '\0':
  case '(':
  case ')':
  case '[':
  case ']':
  case '.':
  case ';':
    return 1;
  default:
    if (isspace(ch))
      return 1;
    return 0;
  }
}

Index gc_strToName(char *str)
{
  Index indx, indx2;
  int i, j;

  indx = gc_getFreeCell();
  ec;
  push(indx);
  ec;
  indx2 = indx;
  j = 0;
  for (i = 0; str[i] != '\0'; i++)
  {
    if (j == sizeof(Index))
    {
      cdr(indx2) = gc_getFreeCell();
      ec;
      indx2 = cdr(indx2);
      j = 0;
    }
    (cas(indx2))[j++] = str[i];
  }
  if (j < sizeof(Index))
    for (i = j; i < sizeof(Index); i++)
      (cas(indx2))[i] = '\0';
  cdr(indx2) = 0;
  pop();
  return indx;
}

void nameToStr(Index indx, char *str)
{
  int i = 0, j = 0;
  do
  {
    if (j == sizeof(Index))
    {
      indx = cdr(indx);
      if (indx == 0)
        break;
      j = 0;
    }
    str[i] = (cas(indx))[j++];
  } while (str[i++] != '\0');
  str[i] = '\0';
}

Index gc_makeSymbol(char *str)
{
  Index cell, cell2;

  cell = gc_getFreeCell();
  ec;
  push(cell);
  ec;
  car(cell) = gc_strToName(str);
  cell2 = gc_getFreeCell();
  ec;
  cdr(cell) = cell2;
  car(cell2) = cell;
  cdr(cell2) = 0;
  tag(cell) = SYMBOL;
  pop();
  return cell;
}

Index addSymbol(int hash_n, Index symbol)
{
  cdr(cdr(symbol)) = symbol_table[hash_n];
  symbol_table[hash_n] = symbol;
  return symbol;
}

int hash(char *str)
{
  int hash_n, i;

  hash_n = 0;
  for (i = 0; str[i] != '\0'; i++)
    hash_n = (hash_n + str[i]) % SYMBOLTABLE_SIZE;
  return hash_n;
}

Index gc_getSymbol()
{
  int i, hash_n;
  Index symbol;

  i = 0;
  for (; !isSeparator(*txtp); i++)
  {
    namebuf[i] = *(txtp++);
  }
  namebuf[i] = '\0';
  hash_n = hash(namebuf);
  symbol = symbol_table[hash_n];
  while (symbol)
  {
    nameToStr(car(symbol), namebuf2);
    if (!strcmp(namebuf2, namebuf))
      return symbol;
    symbol = cdr(cdr(symbol));
  }
  symbol = gc_makeSymbol(namebuf);
  ec;
  addSymbol(hash_n, symbol);
  return symbol;
}

void printSymbol(Index atom)
{
  if (!atom) /* nil 表示のため */
  {
    printf("()");
    return;
  }
  nameToStr(car(atom), namebuf);
  printf("%s", namebuf);
}

Index gc_makeatom_sub(char *str)
{
  Index cell, cell2;
  char *txtp2;

  cell = gc_getFreeCell();
  ec;
  push(cell);
  ec;
  cell2 = gc_getFreeCell();
  ec;
  txtp2 = txtp;
  txtp = str; /* 末尾にスペースが必要 */
  car(cell) = gc_getSymbol();
  ec;
  txtp = ++txtp2;
  car(cell2) = gc_readS(0);
  ec;
  cdr(cell) = cell2;
  pop();
  return cell;
}

Index gc_makeAtom()
{
  /* $ は単独でシンボルの先頭に使えない。 */
  if (*txtp == '$' && *(txtp + 1) != '$')
  {
    txtp++;
    return gc_getSymbol();
  }
  /* 省略記法 */
  if (*txtp == '\'')
    return gc_makeatom_sub("quote ");
  if (*txtp == '`')
    return gc_makeatom_sub("backquote ");
  if (*txtp == ',' && *(txtp + 1) == '@')
  {
    txtp++;
    return gc_makeatom_sub("atmark ");
  }
  if (*txtp == ',' && *(txtp + 1) != '@')
    return gc_makeatom_sub("comma ");
  if (*txtp == '#' && '0' <= *(txtp + 1) && *(txtp + 1) <= '9')
    return gc_makeatom_sub("num ");
  if (*txtp == '#' && *(txtp + 1) == '\'')
  {
    txtp++;
    return gc_makeatom_sub("function ");
  }

  return gc_getSymbol();
}

void printAtom(Index indx)
{
  printSymbol(indx);
}

char *getstr()
{
  printf("%% ");
  txtp = textbuf;
  *txtp = '\0';
  return fgets(textbuf, TEXTBUF_SIZE, ifp);
}

int skipspace()
{
  for (;;)
  {
    while (isspace(*txtp))
      txtp++;
    if (*txtp != '\0' && *txtp != ';')
      return 1;
    if (getstr() == NULL)
      return 0;
    ec;
  }
}

Index gc_makeList(int from_top)
{
  Index indx, indx2, indx3;

  int super_bracket;
  if (*txtp++ == '[')
    super_bracket = 1;
  else
    super_bracket = 0;
  if (!skipspace())
    return eof_error();
  if (*txtp == ')')
  {
    txtp++;
    return 0;
  }
  if (*txtp == ']')
  {
    if (super_bracket || from_top)
      txtp++;
    return 0;
  }
  if (*txtp == '.')
    return error("There is no \"car\" of the dotted pair.");
  indx2 = indx = gc_getFreeCell();
  ec;
  push(indx);
  ec;
  car(indx2) = gc_readS(0);
  ec;
  if (!skipspace())
  {
    pop();
    return eof_error();
  }
  while (*txtp != ')' && *txtp != ']')
  {
    if (*txtp == '.')
    {
      txtp++;
      if (!skipspace())
      {
        pop();
        return eof_error();
      }
      if (*txtp == ')' || *txtp == ']')
      {
        pop();
        return error("There is no \"cdr\" of the dotted pair.");
      }
      cdr(indx2) = gc_readS(0);
      ec;
      if (!skipspace())
      {
        pop();
        return eof_error();
      }
      if (*txtp != ')' && *txtp != ']')
      {
        pop();
        return error("There is no ')' immediately after the dotted pair.");
      }
      break;
    }
    indx3 = gc_getFreeCell();
    ec;
    cdr(indx2) = indx3;
    car(indx3) = gc_readS(0);
    ec;
    indx2 = indx3;
    if (!skipspace())
    {
      pop();
      return eof_error();
    }
  }
  if (*txtp == ']')
    if (!super_bracket && !from_top)
    {
      pop();
      return indx;
    }
  txtp++;
  pop();
  return indx;
}

Index gc_readS(Index from_top)
{
  if (!skipspace())
    return eof_error();
  else if (*txtp == '(' || *txtp == '[')
    return gc_makeList(from_top);
  else if (!isSeparator(*txtp))
    return gc_makeAtom();
  else
    return error("Unexpected character.");
}

void printS(Index indx)
{
  if (!is(indx, CELL))
    printAtom(indx);
  else
  {
    putchar('(');
    for (;;)
    {
      printS(car(indx));
      indx = cdr(indx);
      if (!is(indx, CELL))
        break;
      putchar(' ');
    }
    if (indx)
    {
      printf(" . ");
      printAtom(indx);
    }
    putchar(')');
  }
}
