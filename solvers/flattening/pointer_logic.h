/*******************************************************************\

Module: Pointer Logic

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#ifndef CPROVER_POINTER_LOGIC_H
#define CPROVER_POINTER_LOGIC_H

#include <irep2.h>
#include <mp_arith.h>
#include <hash_cont.h>
#include <expr.h>
#include <numbering.h>

class pointer_logict
{
public:
  // this numbers the objects
  typedef std::map<expr2tc, unsigned int> objectst;
  objectst objects;
  typedef std::vector<expr2tc> obj_lookupt;
  obj_lookupt lookup;

  struct pointert
  {
    unsigned object;
    mp_integer offset;
    
    pointert()
    {
    }
    
    pointert(unsigned _obj, mp_integer _off):object(_obj), offset(_off)
    {
    }
  };
  
  // converts an (object,offset) pair to an expression
  expr2tc pointer_expr(const pointert &pointer, const type2tc &type) const;

  // converts an (object,0) pair to an expression
  expr2tc pointer_expr(unsigned object, const type2tc &type) const;
    
  ~pointer_logict();
  pointer_logict();
  
  unsigned add_object(const expr2tc &expr);

  // number of NULL object  
  unsigned get_null_object() const
  {
    return null_object;
  }

  // number of INVALID object  
  unsigned get_invalid_object() const
  {
    return invalid_object;
  }
  
protected:
  unsigned null_object, invalid_object;  

  expr2tc pointer_expr(const mp_integer &offset, const expr2t &object) const;

  expr2tc object_rec(const mp_integer &offset, const type2tc &pointer_type,
                     const expr2tc &src) const;
};

#endif
