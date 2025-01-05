/*                                   */
/*               Main                */
/*                                   */

#define MAIN
#include <stdio.h>
#include <stdlib.h>
#include "LISP.H"

char textbuf[TEXTBUF_SIZE];
char namebuf[TEXTBUF_SIZE];
char namebuf2[TEXTBUF_SIZE];
Cell *cells;
char *tags;
Index *stack;
char *txtp;
Index freecells, toplevel;
Index symbol_table[SYMBOLTABLE_SIZE];
int err;
char *message;
FILE *ifp;
int sp;

/* インデックス n のセルに name を持つシンボルを作ってテーブルに登録 */
void gc_addSystemSymbol(Index n, char *name)
{
  car(n) = gc_strToName(name);
  cdr(n) = gc_getFreeCell();
  tag(n) = SYMBOL;
  car(cdr(n)) = n;
  cdr(cdr(n)) = 0;
  addSymbol(hash(name), n);
}

void gc_addFunc(char *name, Index (*func)(Index, Index))
{
  Index f, cell;

  f = gc_makeSymbol(name);
  addSymbol(hash(name), f);
  tag(f) = SYMBOL;
  car(cdr(f)) = gc_getFreeCell();
  p_f(car(cdr(f))) = func;
  tag(car(cdr(f))) = POINTER;
  /* oblist に登録 */
  cell = gc_getFreeCell();
  cdr(cell) = car(cdr(3));
  car(cdr(3)) = cell;
  car(cell) = f;
}

void initCells()
{
  Index indx;
  int i;

  /* フリーセル・リストの連結 */
  for (indx = 0; indx < CELLS_SIZE - 1; indx++)
  {
    car(indx) = 0;
    cdr(indx) = indx + 1;
    tag(indx) = CELL;
  }
  car(CELLS_SIZE - 1) = 0;
  cdr(CELLS_SIZE - 1) = 0;

  /* シンボルテーブルの初期化 */
  for (i = 0; i < SYMBOLTABLE_SIZE; i++)
    symbol_table[i] = 0;

  /* フリーセルの先頭位置の初期化 */
  freecells = 4; /* 0 ~ 3 は予約済み */

  /* sp の初期化 */
  sp = 0; /* GC 用スタックポインタ */

  /* nil の登録 */
  tag(0) = NIL;
  car(0) = 0;
  cdr(0) = 0;

  /* システム・シンボルの登録 */
  gc_addSystemSymbol(1, "t");
  gc_addSystemSymbol(2, "lambda");
  gc_addSystemSymbol(3, "oblist");
  car(cdr(3)) = 0;

  /* 基本関数の登録 */
  gc_addFunc("quote", quote_f);
  gc_addFunc("car", gc_car_f);
  gc_addFunc("cdr", gc_cdr_f);
  gc_addFunc("cons", gc_cons_f);
  gc_addFunc("cond", gc_cond_f);
  gc_addFunc("atom", gc_atom_f);
  gc_addFunc("eq", gc_eq_f);
  gc_addFunc("de", gc_de_f);
  gc_addFunc("setq", gc_setq_f); /* シンボルに値を設定 */
  gc_addFunc("gc", gc_f);        /* ガベージ・コレクション */
}

void top_loop()
{
  txtp = textbuf;
  *txtp = '\0';
  while (err != eof)
  {
    err = off;
    toplevel = gc_readS(1);
    if (err != off)
    {
      char *chp;

      printf("%s\n", message);
      printf("> ");
      for (chp = textbuf; chp <= txtp; chp++)
        putchar(*chp);
      putchar('\n');
      *txtp = '\0';
      continue;
    }
    toplevel = gc_eval_f(toplevel, 0);
    if (err == off)
    {
      printS(toplevel);
      putchar('\n');
    }
  }
}

void greeting()
{
  printf("\n");
  printf("        Pure LISP Interpreter\n\n");
  printf("          p u r e  L I S P\n\n");
  printf("           Version 0.0.3\n");
  printf(" This software is released under the\n");
  printf("            MIT License.\n\n");
  printf("                 (C) 2024-2025 Tsugu\n\n");
}

int main()
{
  cells = (Cell *)malloc(sizeof(Cell) * CELLS_SIZE);
  if (cells == NULL)
  {
    printf("Unable to secure a cell area.\n");
    return 0;
  }
  tags = (char *)malloc(sizeof(char) * CELLS_SIZE);
  if (tags == NULL)
  {
    printf("Unable to secure a cell area.\n");
    return 0;
  }
  stack = (Index *)malloc(sizeof(Index) * STACK_SIZE);
  if (stack == NULL)
  {
    printf("Unable to secure a stack.\n");
    return 0;
  }
  ifp = stdin;
  initCells();
  ifp = fopen("init.txt", "r"); /* 起動時に読み込む LISP プログラム */
  if (ifp == NULL)
  {
    printf("\"init.txt\" is missing.\n");
    return 0;
  }
  err = off;
  top_loop();
  fclose(ifp);
  ifp = stdin;
  greeting();
  while (1)
  {
    err = off;
    top_loop();
    if (ifp == stdin)
      rewind(ifp);
    else
      fclose(ifp);
  }
  free(cells);
  free(tags);
  free(stack);
  return 1;
}
