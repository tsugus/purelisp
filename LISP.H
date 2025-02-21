/*                                     */
/*          p u r e  L I S P           */
/*                                     */
/* This software is released under the */
/*            MIT License.             */
/*                                     */
/*                 (C) 2024-2025 Tsugu */
/*                                     */

#define CELLS_SIZE 0x18000  /* セルの総数 */
#define TEXTBUF_SIZE 100    /* 入力テキスト用バッファ */
#define SYMBOLTABLE_SIZE 64 /* シンボルを割り振るハッシュテーブルのサイズ */
#define STACK_SIZE 0x3000   /* GC 用スタックのサイズ */
#define tag(x) (tags[x])    /* ID を格納するタグ */
#define car(x) (cells[x].pair.car).index
#define cdr(x) (cells[x].pair.cdr).index
#define cas(x) (cells[x].pair.car).string
#define cds(x) (cells[x].pair.cdr).string
#define p_f(x) (cells[x].p_f)                 /* 関数ポインタ */
#define is(x, y) (abs(tag(x)) == (y) ? 1 : 0) /* x のタグの値は y か ? */
/* エラー時の大局脱出用マクロ */
#define ec \
  if (err) \
  return (0)

enum ID
{
  /* アトム用 */
  NIL,
  SYMBOL, /* ただし NIL を含まない */
  /* セル用 */
  CELL,
  POINTER, /* C の関数ポインタ */
  /* アトムの cdr 用 */
  ARGsEVAL,    /* 関数の引数を評価する */
  ARGsNotEVAL, /* 関数の引数を評価しない */
};

enum ERROR_NUM
{
  off,
  on,
  print_no_more,
  eof
};

typedef unsigned int Index;

typedef union WORD
{
  Index index;
  char string[sizeof(Index)];
} Word;

typedef union CELL
{
  Index (*p_f)(Index, Index);
  struct PAIR
  {
    Word car;
    Word cdr;
  } pair;
} Cell;

#ifndef MAIN
extern char textbuf[TEXTBUF_SIZE];
extern char namebuf[TEXTBUF_SIZE];
extern char namebuf2[TEXTBUF_SIZE];
extern char *tags;
extern Cell *cells;
extern Index stack[STACK_SIZE];
extern char *txtp;
extern Index freecells, toplevel;
extern Index symbol_table[SYMBOLTABLE_SIZE];
extern int err;
extern char *message;
extern FILE *ifp;
extern int sp; /* GC 用スタックポインタ */
#endif

/* From "MAIN.C" */
void initCells();
/* From "IO.C" */
Index error(char *);
Index gc_getFreeCell();
Index gc_strToName(char *);
void nameToStr(Index, char *);
Index gc_makeSymbol(char *);
Index addSymbol(int, Index);
int hash(char *);
Index gc_readS(Index);
void printS(Index);
/* From "GC.C" */
Index push(Index);
Index pop();
Index purseS(Index);
/* From "EVAL.C" */
Index gc_eval_args(Index, Index);
Index gc_cloneS(Index);
Index gc_apply(Index, Index, Index);
Index gc_eval(Index, Index);
Index getFromOblist(Index);
Index gc_bqev(Index, Index);
Index gc_bqapnd(Index, Index);
/* From "FUNC.C" */
Index gc_eval_f(Index, Index);
Index gc_apply_f(Index, Index);
Index quote_f(Index, Index);
Index gc_car_f(Index, Index);
Index gc_cdr_f(Index, Index);
Index gc_cons_f(Index, Index);
Index gc_cond_f(Index, Index);
Index gc_atom_f(Index, Index);
Index gc_eq_f(Index, Index);
Index gc_de_f(Index, Index);
Index gc_df_f(Index, Index);
Index gc_setq_f(Index, Index);
Index gc_psetq_f(Index, Index);
Index gc_f(Index, Index);
Index gc_while_f(Index, Index);
Index gc_until_f(Index, Index);
Index gc_rplaca_f(Index, Index);
Index gc_rplacd_f(Index, Index);
Index gc_function_f(Index, Index);
Index gc_funcall_f(Index, Index);
Index gc_dm_f(Index, Index);
Index gc_backquote_f(Index, Index);
Index comma_f(Index, Index);
Index atmark_f(Index, Index);
Index gc_gensym_f(Index, Index);
Index quit_f(Index, Index);
Index gc_num_f(Index, Index);
Index len_f(Index, Index);
Index cls_f(Index, Index);
