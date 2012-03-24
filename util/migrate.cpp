#include "migrate.h"

// File for old irep -> new irep conversions.


bool
migrate_type(const typet &type, type2tc &new_type_ref)
{

  return false;
}

bool
migrate_expr(const exprt &expr, expr2tc &new_expr_ref)
{
  type2tc type;

  if (expr.id() == "symbol") {
    if (!migrate_type(expr.type(), type))
      return false;

    expr2t *new_expr = new symbol2t(type, expr.identifier().as_string());
    new_expr_ref = expr2tc(new_expr);
    return true;
  } else {
    return false;
  }
}
