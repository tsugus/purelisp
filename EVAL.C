/*                                   */
/*       Evaluator & functions       */
/*                                   */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "LISP.H"

/* エラー表示 */
void print_error(Index form, char *msg)
{
  printf("%s\n", msg);
  printf("At ");
  printS(form);
  putchar('\n');
  err = print_no_more; /* これ以上、表示しない */
}

/* 連想リスト検索 */
int searchAssoc(Index key, Index *value, Index assoclist)
{
  Index pair;

  while (assoclist)
  {
    pair = car(assoclist);
    if (key == car(pair))
    {
      *value = cdr(pair);
      return 1;
    }
    assoclist = cdr(assoclist);
  }
  return 0;
}

Index gc_addToAssoc(Index key, Index value, Index assoclist)
{
  Index indx, pair;

  indx = gc_getFreeCell();
  ec;
  push(indx);
  ec;
  pair = gc_getFreeCell();
  ec;
  car(pair) = key;
  cdr(pair) = value;
  car(indx) = pair;
  cdr(indx) = assoclist; /* 先頭に追加する */
  pop();
  return indx; /* indx も連想リスト */
}

Index gc_eval_args(Index args, Index env)
{
  Index args2;

  push(args);
  ec;
  args2 = args;
  while (is(args2, CELL))
  {
    car(args2) = gc_eval_f(car(args2), env);
    ec;
    args2 = cdr(args2);
  }
  pop();
  return args;
}

Index gc_lambda(Index form, Index env)
{
  Index formal_args, S, actual_args, key, value, result;

  if (is(car(car(form)), CELL) || !is(cdr(car(form)), CELL))
    error("Not enough arguments.");
  formal_args = car(cdr(car(form)));
  S = cdr(cdr(car(form)));
  actual_args = cdr(form);
  actual_args = gc_eval_args(actual_args, env);
  ec;
  result = 0;
  /* 仮引数と実引数のペアを環境リストに追加 */
  push(0); /* ダミー */
  ec;
  while (is(formal_args, CELL))
  {
    key = car(formal_args);
    value = car(actual_args);
    if (!is(key, SYMBOL))
    {
      err = on;
      print_error(car(form), "Some formal arguments are not symbols.");
      sp = 0;
      return 0;
    }
    if (!actual_args)
    {
      err = on;
      print_error(form, "Not enough actual arguments.");
      sp = 0;
      return 0;
    }
    env = gc_addToAssoc(key, value, env);
    ec;
    pop(); /* 入れ替え */
    push(env);
    ec;
    formal_args = cdr(formal_args);
    actual_args = cdr(actual_args);
  }
  /* 最後がドット対のとき */
  if (formal_args)
  {
    if (!is(formal_args, SYMBOL))
    {
      err = on;
      print_error(car(form), "A formal argument is not a symbols.");
      sp = 0;
      return 0;
    }
    env = gc_addToAssoc(formal_args, actual_args, env);
    ec;
    pop(); /* 入れ替え */
    push(env);
    ec;
  }
  /* 関数本体の評価 */
  for (; S; S = cdr(S))
  {
    result = gc_eval_f(car(S), env);
    ec;
  }
  pop();
  return result;
}

/* リストのコピー */
Index gc_cloneS(Index indx)
{
  if (!is(indx, CELL))
    return indx; /* printAtom(indx); */
  else
  {
    Index root, cell, cell2;

    root = cell = gc_getFreeCell();
    ec;
    push(root);
    ec;
    /* putchar('('); */
    for (;;)
    {
      car(cell) = gc_cloneS(car(indx)); /* printS(car(indx)); */
      ec;
      indx = cdr(indx); /* indx = cdr(indx); */
      cdr(cell) = gc_getFreeCell();
      ec;
      cell2 = cell;
      cell = cdr(cell);
      if (!is(indx, CELL))
        break;
      /* putchar(' '); */
    }
    /* if (indx) */
    /* { */
    /* printf(" . "); */
    cdr(cell2) = indx; /* printAtom(indx); */
    /* } */
    pop();
    return root; /* putchar(')'); */
  }
}

Index gc_clone_f(Index args, Index env)
{
  return gc_cloneS(car(args));
}

Index gc_eval_f(Index form, Index env)
{
  Index head, args, val, form2;
  Index (*f_pointer)(Index, Index);

  push(form); /* form を ソフトスタックに積んで、処理系から見えるようにする */
  ec;
  push(env); /* eval を を処理系から見えるようにする */
  ec;
  form2 = form;
  switch (abs(tag(form)))
  {
  case CELL:
    head = car(form);
    args = cdr(form);
    if (!is(car(form), CELL))
    {
      if (is(head, SYMBOL))
      {
        switch (abs(tag(car(cdr(head)))))
        {
        case POINTER:
          f_pointer = p_f(car(cdr(head)));
          form = (*f_pointer)(args, env);
          break;
        case CELL:
          car(form) = gc_cloneS(car(cdr(head)));
          if (car(form))
            form = gc_eval_f(form, env);
          break;
        default:
          error("There is no function definition.");
        }
      }
      else
        error("The first item in the list is not a function.");
    }
    else
    {
      if (car(head) == 2) /* lambda */
      {
        if (!is(cdr(head), CELL))
          error("Not enough arguments");
        form = gc_lambda(form, env);
      }
      else
        error("The first item in the list is not a lambda expression.");
    }
    break;
  case SYMBOL:
    if (searchAssoc(form, &val, env)) /* 環境リストに form があるとき */
      form = val;
    else
      /* アトムの値への自動置き換え */
      if (!is(car(cdr(form)), POINTER))
        form = car(cdr(form));
    break;
  case NIL:
    break;
  default:
    error("A Cell with unexpected ID.");
  }
  if (err == on)
  {
    print_error(form2, message);
    return 0;
  }
  pop(); /* push した回数だけ pop する */
  pop();
  return form;
}

Index getFromOblist(Index symbol)
{
  Index cell;

  nameToStr(car(symbol), namebuf);
  cell = car(cdr(3));
  /* リストの検索 */
  while (cell)
  {
    nameToStr(car(car(cell)), namebuf2);
    if (!strcmp(namebuf2, namebuf))
      return car(cell);
    cell = cdr(cell);
  }
  return 0;
}

Index quote_f(Index args, Index env)
{
  if (!is(args, CELL))
    return error("Not enough arguments.");
  return car(args);
}

Index gc_car_f(Index args, Index env)
{
  args = gc_eval_args(args, env);
  ec;
  if (!is(args, CELL))
    return error("Not enough arguments.");
  if (!car(args)) /* nil はリストでもある */
    return 0;
  if (!is(car(args), CELL))
    return error("An argument is not a list.");
  return car(car(args));
}

Index gc_cdr_f(Index args, Index env)
{
  args = gc_eval_args(args, env);
  ec;
  if (!is(args, CELL))
    return error("Not enough arguments.");
  if (!car(args))
    return 0;
  if (!is(car(args), CELL))
    return error("An argument is not a list.");
  return cdr(car(args));
}

Index gc_atom_f(Index args, Index env)
{
  args = gc_eval_args(args, env);
  ec;
  if (!is(args, CELL))
    return error("Not enough arguments.");
  if (!is(car(args), CELL))
    return 1;
  return 0;
}

Index gc_eq_f(Index args, Index env)
{
  args = gc_eval_args(args, env);
  ec;
  if (!is(args, CELL) || !is(cdr(args), CELL))
    return error("Not enough arguments.");
  if (car(args) == car(cdr(args)))
    return 1;
  return 0;
}

Index gc_cons_f(Index args, Index env)
{
  Index indx;

  args = gc_eval_args(args, env);
  ec;
  if (!is(args, CELL) || !is(cdr(args), CELL))
    return error("Not enough arguments.");
  indx = gc_getFreeCell();
  ec;
  car(indx) = car(args);
  cdr(indx) = car(cdr(args));
  return indx;
}

Index gc_cond_f(Index clauses, Index env)
{
  Index key, bodies, result;

  if (!is(clauses, CELL))
    return error("Not enough arguments.");
  while (is(clauses, CELL))
  {
    if (!is(car(clauses), CELL))
      return error("A condition clause is not a list.");
    key = gc_eval_f(car(car(clauses)), env);
    ec;
    if (key)
    {
      bodies = cdr(car(clauses));
      if (!is(bodies, CELL))
        return key;
      while (is(bodies, CELL))
      {
        result = gc_eval_f(car(bodies), env);
        ec;
        bodies = cdr(bodies);
      }
      return result;
    }
    clauses = cdr(clauses);
  }
  return 0;
}

Index gc_de_f(Index args, Index env)
{
  Index func, lamb;

  if (!is(args, CELL) || !is(cdr(args), CELL))
    return error("Not enough arguments.");
  func = car(args);
  if (!is(func, SYMBOL))
    return error("The first item in the list is not a symbol.");
  lamb = gc_getFreeCell();
  ec;
  push(lamb);
  ec;
  car(lamb) = 2; /* シンボル lambda の記憶位置を代入 */
  cdr(lamb) = cdr(args);
  car(cdr(func)) = lamb;
  tag(car(cdr(func))) = CELL;
  /* oblist への追加 */
  if (!getFromOblist(func)) /* すでにあるか検索 */
  {
    Index cell = gc_getFreeCell();

    cdr(cell) = car(cdr(3)); /* oblist のインデックスは 3 */
    car(cdr(3)) = cell;      /* リストそのものはシンボル oblist の内部にある */
    car(cell) = func;
  }
  pop();
  return func;
}

Index gc_set_f(Index args, Index env)
{
  Index symbol;

  if (!is(args, CELL))
    return error("Not enough arguments.");
  symbol = car(args);
  if (!is(symbol, SYMBOL))
    return error("The first item in the list is not a symbol.");
  car(cdr(symbol)) = (car(cdr(args)));
  tag(car(symbol)) = CELL;
  /* oblist への追加 */
  if (!getFromOblist(symbol))
  {
    Index cell = gc_getFreeCell();

    cdr(cell) = car(cdr(3)); /* oblist のインデックスは 3 */
    car(cdr(3)) = cell;
    car(cell) = symbol;
  }
  return symbol;
}
