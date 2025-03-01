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
char *tags;
Cell *cells;
Index stack[STACK_SIZE];
char *txtp;
Index freecells, toplevel;
Index symbol_table[SYMBOLTABLE_SIZE];
int err;
char *message;
FILE *ifp;
int sp;
int no_input_after_GC;

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

void gc_addFunc(char *name, Index (*func)(Index, Index), enum ID id)
{
  Index f, cell;

  f = gc_makeSymbol(name);
  addSymbol(hash(name), f);
  tag(f) = SYMBOL;
  car(cdr(f)) = gc_getFreeCell();
  p_f(car(cdr(f))) = func;
  tag(car(cdr(f))) = POINTER;
  tag(cdr(f)) = id;
  /* oblist に登録 */
  cell = gc_getFreeCell();
  cdr(cell) = car(cdr(4));
  car(cdr(4)) = cell;
  car(cell) = f;
}

void gc_addDummyFunc(Index indx, Index (*func)(Index, Index), enum ID id)
{
  Index cell;

  tag(indx) = SYMBOL;
  car(cdr(indx)) = gc_getFreeCell();
  p_f(car(cdr(indx))) = func;
  tag(car(cdr(indx))) = POINTER;
  tag(cdr(indx)) = id;
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
  freecells = 9; /* 0 ~ 8 は予約済み */

  /* sp の初期化 */
  sp = 0; /* GC 用スタックポインタ */

  /* nil の登録 */
  tag(0) = NIL;
  car(0) = 0;
  cdr(0) = 0;

  /* システム・シンボルの登録 */
  gc_addSystemSymbol(1, "t");
  gc_addSystemSymbol(2, "lambda");
  gc_addSystemSymbol(3, "nlambda");
  gc_addSystemSymbol(4, "oblist");
  car(cdr(4)) = 0;
  gc_addSystemSymbol(5, "funarg");
  gc_addSystemSymbol(6, "macro");
  gc_addSystemSymbol(7, "comma");
  gc_addSystemSymbol(8, "atmark");

  /* 基本関数の登録 */
  gc_addFunc("eval", gc_eval_f, ARGsEVAL);
  gc_addFunc("apply", gc_apply_f, ARGsNotEVAL);
  gc_addFunc("quote", quote_f, ARGsNotEVAL);
  gc_addFunc("car", gc_car_f, ARGsEVAL);
  gc_addFunc("cdr", gc_cdr_f, ARGsEVAL);
  gc_addFunc("cons", gc_cons_f, ARGsEVAL);
  gc_addFunc("cond", gc_cond_f, ARGsNotEVAL);
  gc_addFunc("atom", gc_atom_f, ARGsEVAL);
  gc_addFunc("eq", gc_eq_f, ARGsEVAL);
  gc_addFunc("de", gc_de_f, ARGsNotEVAL);
  gc_addFunc("df", gc_df_f, ARGsNotEVAL);
  gc_addFunc("setq", gc_setq_f, ARGsNotEVAL);         /* 値を評価してそれぞれ変数に代入 */
  gc_addFunc("psetq", gc_psetq_f, ARGsNotEVAL);       /* 値を一括評価後、各変数に代入 */
  gc_addFunc("gc", gc_f, ARGsNotEVAL);                /* ガベージ・コレクション */
  gc_addFunc("while", gc_while_f, ARGsNotEVAL);       /* 前置判定ループ */
  gc_addFunc("until", gc_until_f, ARGsNotEVAL);       /* 否定的後置判定ループ */
  gc_addFunc("rplaca", gc_rplaca_f, ARGsEVAL);        /* リスト先頭の cdr を書き換える */
  gc_addFunc("rplacd", gc_rplacd_f, ARGsEVAL);        /* リスト先頭の cdr を書き換える */
  gc_addFunc("function", gc_function_f, ARGsNotEVAL); /* funarg 式を作る */
  gc_addFunc("funcall", gc_funcall_f, ARGsNotEVAL);   /* funarg 式を引数に適用する */
  gc_addFunc("dm", gc_dm_f, ARGsNotEVAL);
  gc_addFunc("backquote", gc_backquote_f, ARGsNotEVAL);
  gc_addDummyFunc(7, comma_f, ARGsNotEVAL);       /* comma (ダミー関数) */
  gc_addDummyFunc(8, atmark_f, ARGsNotEVAL);      /* atmark (ダミー関数) */
  gc_addFunc("gensym", gc_gensym_f, ARGsNotEVAL); /* 変数の自動生成 */
  gc_addFunc("quit", quit_f, ARGsNotEVAL);        /* インタプリタを出る */
  gc_addFunc("num", gc_num_f, ARGsNotEVAL);       /* チャーチ数に変換 */
  gc_addFunc("len", len_f, ARGsEVAL);             /* リストの長さを返す */
  gc_addFunc("cls", cls_f, ARGsNotEVAL);          /* 画面消去 */
}

void top_loop()
{
  txtp = textbuf;
  *txtp = '\0';
  while (err != eof)
  {
    err = off;
    toplevel = gc_readS(1);
    no_input_after_GC = 0;
    if (err != off)
    {
      char *chp;

      printf("%s\n", message);
      if ((*txtp - 1) != EOF) /* init.txt を読み込んだときに > を表示させない */
        printf("> %c\n", *(txtp - 1));
      *txtp = '\0';
      continue;
    }
    toplevel = gc_eval(toplevel, 0);
    if (err == off)
    {
      if (no_input_after_GC)
        putchar('\n');
      printS(toplevel);
      putchar('\n');
    }
  }
}

void greeting()
{
  printf("\n");
  printf("\t      A Pure LISP Interpreter      \n\n");
  printf("\t         P u r e  L I S P          \n\n");
  printf("\t           Version 0.5.8           \n");
  printf("\tThis software is released under the\n");
  printf("\t            MIT License.           \n\n");
  printf("\t                (C) 2024-2025 Tsugu\n\n");
}

int main()
{
  tags = (char *)malloc(sizeof(char) * CELLS_SIZE);
  if (tags == NULL)
  {
    printf("Unable to secure a cell area.\n");
    return 0;
  }
  cells = (Cell *)malloc(sizeof(Cell) * CELLS_SIZE);
  if (cells == NULL)
  {
    printf("Unable to secure a cell area.\n");
    return 0;
  }
  ifp = stdin;
  initCells();
  ifp = fopen("init.txt", "r"); /* 起動時に読み込む LISP プログラム */
  if (ifp == NULL)
  {
    printf("\"init.txt\" is missing. Please prepare an empty init.txt file.\n");
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
}
