#include "migrate.h"

// File for old irep -> new irep conversions.


bool
migrate_type(const typet &type, type2tc &new_type_ref)
{

  if (type.id() == "bool") {
    bool_type2t *b = new bool_type2t();
    new_type_ref = type2tc(b);
    return true;
  } else if (type.id() == "signedbv") {
    irep_idt width = type.width();
    unsigned int iwidth = strtol(width.as_string().c_str(), NULL, 10);
    signedbv_type2t *s = new signedbv_type2t(iwidth);
    new_type_ref = type2tc(s);
    return true;
  } else if (type.id() == "unsignedbv") {
    irep_idt width = type.width();
    unsigned int iwidth = strtol(width.as_string().c_str(), NULL, 10);
    unsignedbv_type2t *s = new unsignedbv_type2t(iwidth);
    new_type_ref = type2tc(s);
    return true;
  }

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
