/*                                   */
/*         Gabage Collection         */
/*                                   */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "LISP.H"

Index push(Index indx)
{
  if (sp >= STACK_SIZE)
    error("The software stack overflowed.");
  stack[sp++] = indx;
  return indx;
}

Index pop()
{
  Index indx;

  if (sp < 1)
    sp = 1;
  indx = stack[--sp];
  return indx;
}

void purseName(Index indx)
{
  for (;;)
  {
    tag(indx) = abs(tag(indx));
    if (!cdr(indx))
      return;
    indx = cdr(indx);
  }
}

Index purseSymbol(Index indx)
{
  tag(indx) = abs(tag(indx));
  purseName(car(indx));
  if (!cdr(indx))
    return indx;
  indx = cdr(indx);
  tag(indx) = abs(tag(indx));
  if (tag(car(indx)) == -CELL) /* 関数定義、あるいは値としての S-式。かつ未訪問 */
    purseS(car(indx));
  else if (is(car(indx), POINTER))
    tag(car(indx)) = POINTER;
  return indx;
}

Index purseAtom(Index indx)
{
  if (indx)
  {
    int hash_n;
    Index symbol;

    purseSymbol(indx);
    ec;
    nameToStr(car(indx), namebuf);
    /* シンボルテーブルの検索（機能が gc_getSymbol と重複している！！） */
    hash_n = hash(namebuf);
    symbol = symbol_table[hash_n];
    while (symbol)
    {
      nameToStr(car(symbol), namebuf2);
      if (!strcmp(namebuf2, namebuf))
      {
        symbol = indx;
        break;
      }
      symbol = cdr(cdr(symbol));
    }
    /* indx がシンボルテーブルにない場合 */
    if (!symbol)
      addSymbol(hash(namebuf), indx);
  }
  return indx;
}

Index purseS(Index indx)
{
  if (!is(indx, CELL))
    purseAtom(indx);
  else
  {
    tag(indx) = abs(tag(indx));
    /* putchar('('); */
    for (;;)
    {
      if (tag(car(indx)) < 0)
        purseS(car(indx));
      indx = cdr(indx);
      tag(indx) = abs(tag(indx));
      if (!is(indx, CELL))
        break;
      /* putchar(' '); */
    }
    if (indx)
      /* { */
      /* printf(" . "); */
      purseAtom(indx);
    /* } */
    /* putchar(')'); */
  }
  return indx;
}

void mark_and_sweep()
{
  Index indx;
  int i;

  printf("... Gabage Collection ! ...\n");
  /* マイナスで印をつける */
  for (indx = 0; indx < CELLS_SIZE; indx++)
    tag(indx) = -abs(tag(indx));
  /* フリーセルの印は取り消す */
  for (indx = freecells; indx; indx = cdr(indx))
    tag(indx) = CELL;
  /* シンボルテーブルはご破算 */
  for (i = 0; i < SYMBOLTABLE_SIZE; i++)
    symbol_table[i] = 0;
  /* S-式をたどって ID がマイナスのセルを取り除く */
  /* 同時に、空にしたテーブルにシンボルを追加し直す */
  purseS(1);
  purseS(2);
  purseS(3);
  for (i = 0; i < sp; i++)
    purseS(stack[i]);
  /* タグの ID がマイナスのセルをフリーセルに戻す */
  for (indx = CELLS_SIZE - 2; indx > 0; indx--) /* 最後尾のセルは「番兵」 */
  {
    if (tag(indx) < 0)
    {
      tag(indx) = CELL;
      car(indx) = 0;
      cdr(indx) = freecells;
      freecells = indx;
    }
  }
}

Index gc_f(Index args, Index env)
{
  mark_and_sweep();
  return 0;
}
