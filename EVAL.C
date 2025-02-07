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
    car(args2) = gc_eval(car(args2), env);
    ec;
    args2 = cdr(args2);
  }
  pop();
  return args;
}

Index gc_lambda(Index head, Index args, Index env)
{
  Index formal_args, S, actual_args, key, value, result;

  if (is(car(head), CELL) || !is(cdr(head), CELL))
    return error("Not enough arguments.");
  formal_args = car(cdr(head));
  S = cdr(cdr(head));
  actual_args = args;
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
      print_error(head, "Some formal arguments are not symbols.");
      sp = 0;
      return 0;
    }
    if (!actual_args)
    {
      err = on;
      print_error(args, "Not enough actual arguments.");
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
      print_error(head, "A formal argument is not a symbols.");
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
    result = gc_eval(car(S), env);
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

/* ラムダ項（nlambda, funsrg, macro を含む）を引数に適用 */
Index gc_apply_lambda(Index func, Index args, Index env)
{
  Index result;

  push(func);
  push(args);
  switch (car(func))
  {
  case 2: /* lambda */
  case 3: /* nlambda */
    if (!is(cdr(func), CELL))
      return error("Not enough arguments");
    result = gc_lambda(func, args, env);
    ec;
    break;
  case 5: /* funarg */
    if (!is(cdr(func), CELL))
      return error("Not enough arguments");
    result = gc_lambda(car(cdr(func)), args, car(cdr(cdr(func))));
    ec;
    break;
  case 6: /* macro */
    if (!is(cdr(func), CELL))
      return error("Not enough arguments");
    result = gc_lambda(func, args, env);
    ec;
    result = gc_eval(result, env);
    ec;
    break;
  default:
    return error("The first item in the list is an invalid form.");
  }
  pop();
  pop();
  return result;
}

Index gc_apply(Index func, Index args, Index env)
{
  Index result;
  Index (*f_pointer)(Index, Index);

  switch (abs(tag(func)))
  {
  case SYMBOL:
    switch (abs(tag(car(cdr(func)))))
    {
    case POINTER:
      f_pointer = p_f(car(cdr(func)));
      result = (*f_pointer)(args, env);
      break;
    case CELL:
      func = gc_cloneS(car(cdr(func)));
      ec;
      result = gc_apply_lambda(func, args, env);
      ec;
      break;
    default:
      return error("There is no function difinition.");
    }
    break;
  case CELL:
    result = gc_apply_lambda(func, args, env);
    ec;
    break;
  default:
    return error("The first item in the list is not a function.");
  }
  return result;
}

Index gc_eval(Index form, Index env)
{
  Index form2, indx, func, args;
  Index (*f_pointer)(Index, Index);

  push(form); /* form を ソフトスタックに積んで、処理系から見えるようにする */
  ec;
  push(env); /* eval を を処理系から見えるようにする */
  ec;
  form2 = form;
  switch (abs(tag(form)))
  {
  case CELL:
    func = car(form);
    args = cdr(form);
    if ((is(func, SYMBOL) &&
         is(cdr(func), ARGsEVAL)) ||
        (is(func, SYMBOL) &&
         is(car(cdr(func)), CELL) &&
         car(car(cdr(func))) == 2) ||
        car(func) == 2 ||
        (car(func) == 5 &&
         car(car(cdr(func))) == 2))
      args = gc_eval_args(args, env);
    form = gc_apply(func, args, env);
    break;
  case SYMBOL:
    if (searchAssoc(form, &indx, env)) /* 環境リストに form があるとき */
      form = indx;
    else
      /* シンボルを値へ自動置き換え */
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
  cell = car(cdr(4));
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

Index gc_bqev(Index args, Index env)
{
  Index indx;

  if (!is(args, CELL))
    return gc_eval(args, env);
  else
  {
    indx = gc_bqapnd(args, env);
    ec;
    return gc_eval(indx, env);
  }
}

Index gc_bqev2(Index args, Index env)
{
  Index indx;

  args = gc_bqev(args, env);
  ec;
  if (!args)
    return 0;
  if (!is(args, CELL))
    return error("Not enough arguments.");
  indx = gc_getFreeCell();
  ec;
  push(indx);
  for (;;)
  {
    car(indx) = car(args);
    if (!is(cdr(args), CELL))
      break;
    cdr(indx) = gc_getFreeCell();
    ec;
    indx = cdr(indx);
    args = cdr(args);
  }
  return pop();
}

Index gc_bqapnd(Index args, Index env)
{
  Index indx, indx2;
  int indxp;

  push(0); /* ダミー */
  indxp = sp - 1;
  for (;; args = cdr(args))
  {
    if (is(car(args), CELL))
    {
      if (car(car(args)) == 8) /* atmark */
      {
        stack[indxp] = indx = gc_bqev2(car(cdr(car(args))), env);
        ec;
        if (!indx) /* @() */
          continue;
        else
          while (is(cdr(indx), CELL))
            indx = cdr(indx);
      }
      else if (car(car(args)) == 7) /* comma */
      {
        stack[indxp] = indx = gc_getFreeCell();
        ec;
        car(indx) = gc_bqev(car(cdr(car(args))), env);
        ec;
      }
      else
      {
        stack[indxp] = indx = gc_getFreeCell();
        ec;
        car(indx) = gc_bqapnd(car(args), env);
        ec;
      }
    }
    else
    {
      stack[indxp] = indx = gc_getFreeCell();
      ec;
      car(indx) = car(args);
    }
    break;
  }
  for (args = cdr(args);; args = cdr(args))
  {
    /* ドット対 */
    if (!is(args, CELL))
    {
      cdr(indx) = args;
      return pop();
    }
    else if (car(args) == 7) /* comma */
    {
      cdr(indx) = gc_bqev(cdr(args), env);
      return pop();
    }
    else if (car(args) == 8) /* atmark */
      return error("The backquote statement is invalid.");
    if (is(car(args), CELL))
    {
      /* ,@ */
      if (car(car(args)) == 8)
      {
        cdr(indx) = gc_bqev2(car(cdr(car(args))), env);
        ec;
        while (is(cdr(indx), CELL))
          indx = cdr(indx);
      }
      /* , */
      else if (car(car(args)) == 7)
      {
        indx2 = gc_getFreeCell();
        ec;
        push(indx2);
        car(indx2) = gc_bqev(car(cdr(car(args))), env);
        ec;
        cdr(indx) = indx2;
        indx = cdr(indx);
        pop();
      }
      else
      {
        indx2 = gc_getFreeCell();
        ec;
        push(indx2);
        car(indx2) = gc_bqapnd(car(args), env);
        ec;
        cdr(indx) = indx2;
        indx = cdr(indx);
        pop();
      }
    }
    /* シンボル */
    else
    {
      indx2 = gc_getFreeCell();
      ec;
      car(indx2) = car(args);
      cdr(indx) = indx2;
      indx = cdr(indx);
    }
  }
}
