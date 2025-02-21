#include <stdio.h>
#include <stdlib.h>
#include "LISP.H"

Index gc_eval_f(Index args, Index env)
{
  return gc_eval(car(args), car(cdr(args)));
}

Index gc_apply_f(Index args, Index env)
{
  Index func;

  if (!is(args, CELL))
    return error("Not enough arguments.");
  func = gc_eval(car(args), env);
  ec;
  args = gc_eval(car(cdr(args)), env);
  ec;
  return gc_apply(func, args, env);
}

Index quote_f(Index args, Index env)
{
  if (!is(args, CELL))
    return error("Not enough arguments.");
  return car(args);
}

Index gc_car_f(Index args, Index env)
{
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
  ec;
  if (!is(args, CELL))
    return error("Not enough arguments.");
  if (!car(args))
    return 0;
  if (!is(car(args), CELL))
    return error("An argument is not a list.");
  return cdr(car(args));
}

Index gc_cons_f(Index args, Index env)
{
  Index indx;

  ec;
  if (!is(args, CELL) || !is(cdr(args), CELL))
    return error("Not enough arguments.");
  indx = gc_getFreeCell();
  ec;
  car(indx) = car(args);
  cdr(indx) = car(cdr(args));
  return indx;
}

Index gc_atom_f(Index args, Index env)
{
  ec;
  if (!is(args, CELL))
    return error("Not enough arguments.");
  if (!is(car(args), CELL))
    return 1;
  return 0;
}

Index gc_eq_f(Index args, Index env)
{
  ec;
  if (!is(args, CELL) || !is(cdr(args), CELL))
    return error("Not enough arguments.");
  if (car(args) == car(cdr(args)))
    return 1;
  return 0;
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
    key = gc_eval(car(car(clauses)), env);
    ec;
    if (key)
    {
      bodies = cdr(car(clauses));
      if (!is(bodies, CELL))
        return key;
      while (is(bodies, CELL))
      {
        result = gc_eval(car(bodies), env);
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

    cdr(cell) = car(cdr(4)); /* oblist のインデックスは 4 */
    car(cdr(4)) = cell;      /* リストそのものはシンボル oblist の内部にある */
    car(cell) = func;
  }
  pop();
  return func;
}

Index gc_df_f(Index args, Index env)
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
  car(lamb) = 3; /* シンボル nlambda の記憶位置を代入 */
  cdr(lamb) = cdr(args);
  car(cdr(func)) = lamb;
  tag(car(cdr(func))) = CELL;
  /* oblist への追加 */
  if (!getFromOblist(func)) /* すでにあるか検索 */
  {
    Index cell = gc_getFreeCell();

    cdr(cell) = car(cdr(4)); /* oblist のインデックスは 4 */
    car(cdr(4)) = cell;      /* リストそのものはシンボル oblist の内部にある */
    car(cell) = func;
  }
  pop();
  return func;
}

/* (setq var_1 val_1 var_2 val_2 ...) */
Index gc_setq_f(Index args, Index env)
{
  Index var, val, indx, pair;

  while (is(args, CELL))
  {
    if (!is(cdr(args), CELL))
      return error("Not enough arguments.");
    var = car(args);
    val = gc_eval(car(cdr(args)), env);
    ec;
    if (!is(var, SYMBOL))
      return error("An odd-numbered item in the list is not a symbol.");
    /* 環境 env の検索 */
    pair = 0;
    for (indx = env; indx; indx = cdr(indx))
    {
      pair = car(indx);
      if (var == car(pair))
        break;
    }
    if (pair) /* 環境内に var があるなら、環境内の値を書き換える */
      cdr(pair) = val;
    else /* そうでないなら var 自身に値を設定 */
    {
      car(cdr(var)) = val;
      tag(cdr(var)) = CELL;
      /* oblist への追加 */
      if (!getFromOblist(var))
      {
        Index cell = gc_getFreeCell();
        ec;

        cdr(cell) = car(cdr(4)); /* oblist のインデックスは 4 */
        car(cdr(4)) = cell;
        car(cell) = var;
      }
    }
    args = cdr(cdr(args));
  }
  return val;
}

/* (psetq var_1 val_1 var_2 val_2 ...) */
Index gc_psetq_f(Index args, Index env)
{
  Index indx, var, val, pair;

  /* val をすべて評価する */
  for (indx = args; is(indx, CELL); indx = cdr(cdr(indx)))
  {
    if (!is(cdr(indx), CELL))
      return error("Not enough arguments.");
    car(cdr(indx)) = gc_eval(car(cdr(indx)), env);
  }
  /* 各 var に各 val をセット */
  while (is(args, CELL))
  {
    var = car(args);
    val = car(cdr(args));
    if (!is(var, SYMBOL))
      return error("An odd-numbered item in the list is not a symbol.");
    /* 環境 env の検索 */
    pair = 0;
    for (indx = env; indx; indx = cdr(indx))
    {
      pair = car(indx);
      if (var == car(pair))
        break;
    }
    if (pair) /* 環境内に var があるなら、環境内の値を書き換える */
      cdr(pair) = val;
    else /* そうでないなら var 自身に値を設定 */
    {
      car(cdr(var)) = val;
      tag(cdr(var)) = CELL;
      /* oblist への追加 */
      if (!getFromOblist(var))
      {
        Index cell = gc_getFreeCell();
        ec;

        cdr(cell) = car(cdr(4)); /* oblist のインデックスは 4 */
        car(cdr(4)) = cell;
        car(cell) = var;
      }
    }
    args = cdr(cdr(args));
  }
  return val;
}

/* (while condition body_1 body_2 ...) */
Index gc_while_f(Index args, Index env)
{
  Index args2, flag, S, result;

  if (!is(args, CELL))
    return error("Not enough arguments.");
  args2 = gc_cloneS(args);
  ec;
  push(args2);
  flag = gc_eval(car(args2), env);
  ec;
  result = 0;
  while (flag)
  {
    for (S = cdr(args2); S; S = cdr(S))
    {
      result = gc_eval(car(S), env);
      ec;
    }
    pop();
    args2 = gc_cloneS(args);
    ec;
    push(args2);
    flag = gc_eval(car(args2), env);
    ec;
  }
  pop();
  return result;
}

/* (until condition body_1 body_2 ...) */
Index gc_until_f(Index args, Index env)
{
  Index args2, flag, S, result;

  if (!is(args, CELL))
    return error("Not enough arguments.");
  args2 = gc_cloneS(args);
  ec;
  push(args2);
  flag = gc_eval(car(args2), env);
  ec;
  result = 0;
  do
  {
    for (S = cdr(args2); S; S = cdr(S))
    {
      result = gc_eval(car(S), env);
      ec;
    }
    pop();
    args2 = gc_cloneS(args);
    ec;
    push(args2);
    flag = gc_eval(car(args2), env);
    ec;
  } while (!flag);
  pop();
  return result;
}

/* (rplaca '(x.y) a) = (a.y) */
Index gc_rplaca_f(Index args, Index env)
{
  ec;
  if (!is(args, CELL))
    return error("Not enough arguments.");
  if (!is(car(args), CELL))
    return error("The first argument is not a cell.");
  car(car(args)) = car(cdr(args));
  return car(args);
}

/* (rplacd '(x.y) a) = (x.a) */
Index gc_rplacd_f(Index args, Index env)
{
  ec;
  if (!is(args, CELL))
    return error("Not enough arguments.");
  if (!is(car(args), CELL))
    return error("The first argument is not a cell.");
  cdr(car(args)) = car(cdr(args));
  return car(args);
}

Index gc_function_f(Index args, Index env)
{
  Index indx, indx2;

  if (!is(car(args), CELL))
    return error("The first argument is not a cell.");
  indx = gc_getFreeCell();
  ec;
  push(indx);
  indx2 = gc_getFreeCell();
  ec;
  push(indx2);
  car(indx) = 5; /* funarg */
  cdr(indx) = gc_cloneS(args);
  car(indx2) = gc_cloneS(env);
  cdr(cdr(indx)) = indx2;
  pop();
  pop();
  return indx;
}

Index gc_funcall_f(Index args, Index env)
{
  Index func;

  if (!is(args, CELL))
    return error("Not enough arguments.");
  func = gc_eval(car(args), env);
  ec;
  args = gc_eval_args(cdr(args), env);
  ec;
  return gc_apply(func, args, env);
}

Index gc_dm_f(Index args, Index env)
{
  Index mac, form;

  if (!is(args, CELL) || !is(cdr(args), CELL))
    return error("Not enough arguments.");
  mac = car(args);
  if (!is(mac, SYMBOL))
    return error("The first item in the list is not a symbol.");
  form = gc_getFreeCell();
  ec;
  push(form);
  ec;
  car(form) = 6; /* シンボル macro の記憶位置を代入 */
  cdr(form) = cdr(args);
  car(cdr(mac)) = form;
  tag(car(cdr(mac))) = CELL;
  /* oblist への追加 */
  if (!getFromOblist(mac)) /* すでにあるか検索 */
  {
    Index cell = gc_getFreeCell();

    cdr(cell) = car(cdr(4)); /* oblist のインデックスは 4 */
    car(cdr(4)) = cell;      /* リストそのものはシンボル oblist の内部にある */
    car(cell) = mac;
  }
  pop();
  return mac;
}

Index gc_backquote_f(Index args, Index env)
{
  if (!is(args, CELL))
    return error("The backquote statement is invalid.");
  args = car(args);
  if (!is(args, CELL))
    return args;
  else if (car(args) == 7) /* comma */
    return gc_bqev(car(cdr(args)), env);
  else if (car(args) == 8) /* atmark */
    return error("The backquote statement is invalid.");
  else
    return gc_bqapnd(args, env);
}

Index comma_f(Index args, Index env) /* エラーを表示させるためだけ */
{
  return error("The backquote statement is invalid.");
}

Index atmark_f(Index args, Index env) /* エラーを表示させるためだけ */
{
  return error("The backquote statement is invalid.");
}

Index gc_gensym_f(Index args, Index env)
{
  static int i = 0;
  Index indx;

  indx = gc_getFreeCell();
  ec;
  push(indx);
  sprintf(namebuf, "$%03d", i);
  car(indx) = gc_strToName(namebuf);
  cdr(indx) = gc_getFreeCell();
  cdr(cdr(indx)) = 0;
  tag(indx) = SYMBOL;
  if (++i > 999)
    i = 0;
  return indx;
}

Index quit_f(Index args, Index env)
{
  free(cells);
  free(tags);
  exit(0);
}

Index gc_num_f(Index args, Index env)
{
  Index num, indx, result;
  int i;

  if (!is(car(args), SYMBOL))
    error("An argument is not a symbol.");
  nameToStr(car(car(args)), namebuf);
  num = atoi(namebuf);
  result = 0;
  for (i = 0; i < num; i++)
  {
    indx = gc_getFreeCell();
    ec;
    car(indx) = 1;
    cdr(indx) = result;
    result = indx;
  }
  return result;
}

Index len_f(Index args, Index env)
{
  Index indx;
  int i;

  indx = car(args);
  for (i = 0; indx; indx = cdr(indx))
    if (i++ < 0)
      return error("Numeric overflow.");
  sprintf(namebuf, "%d", i);
  return gc_makeSymbol(namebuf);
}

Index cls_f(Index args, Index env)
{
  system("reset");     /* for Linux, Mac */
  /* system("cls"); */ /* for Linux, Mac */
  return 0;
}
