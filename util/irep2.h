#ifndef _UTIL_IREP2_H_
#define _UTIL_IREP2_H_

#include <stdarg.h>

#include <vector>

#define BOOST_SP_DISABLE_THREADS 1

#include <boost/mpl/if.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/crc.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/fusion/include/equal_to.hpp>

#include <irep.h>
#include <fixedbv.h>
#include <big-int/bigint.hh>
#include <dstring.h>

// XXXjmorse - abstract, access modifies, need consideration

#define forall_exprs(it, vect) \
  for (std::vector<expr2tc>::const_iterator (it) = (vect).begin();\
       it != (vect).end(); it++)

#define Forall_exprs(it, vect) \
  for (std::vector<expr2tc>::iterator (it) = (vect).begin();\
       it != (vect).end(); it++)

#define forall_types(it, vect) \
  for (std::vector<type2tc>::const_iterator (it) = (vect).begin();\
       it != (vect).end(); it++)

#define Forall_types(it, vect) \
  for (std::vector<type2tc>::iterator (it) = (vect).begin();\
       it != (vect).end(); it++)

#define forall_names(it, vect) \
  for (std::vector<irep_idt>::const_iterator (it) = (vect).begin();\
       it != (vect).end(); it++)

#define Forall_names(it, vect) \
  for (std::vector<std::string>::iterator (it) = (vect).begin();\
       it != (vect).end(); it++)

#define forall_operands2(it, ops, theexpr) \
  expr2t::expr_operands ops; \
  theexpr->list_operands(ops); \
  for (expr2t::expr_operands::const_iterator it = ops.begin(); \
       it != ops.end(); it++)

#define Forall_operands2(it, ops, theexpr) \
  expr2t::Expr_operands ops; \
  theexpr.get()->list_operands(ops); \
  for (expr2t::Expr_operands::iterator it = ops.begin(); \
       it != ops.end(); it++)

class prop_convt;
class type2t;
class expr2t;
class constant_array2t;

template <class T, int expid>
class irep_container : public boost::shared_ptr<T>
{
public:
  irep_container() : boost::shared_ptr<T>() {}

  template<class Y>
  explicit irep_container(Y *p) : boost::shared_ptr<T>(p)
    { assert(expid == -1 || p->expr_id == expid); }

  template<class Y>
  explicit irep_container(const Y *p) : boost::shared_ptr<T>(const_cast<Y *>(p))
    { assert(expid == -1 || p->expr_id == expid); }

  irep_container(const irep_container &ref)
    : boost::shared_ptr<T>(ref) {}

  template <class Y, int I>
  irep_container(const irep_container<Y, I> &ref)
    : boost::shared_ptr<T>(static_cast<const boost::shared_ptr<Y> &>(ref), boost::detail::polymorphic_cast_tag()) {}

  irep_container &operator=(irep_container const &ref)
  {
    boost::shared_ptr<T>::operator=(ref);
    T *p = boost::shared_ptr<T>::get();
    assert(expid == -1 || p == NULL || p->expr_id == expid);
    return *this;
  }

  template<class Y>
  irep_container & operator=(boost::shared_ptr<Y> const & r)
  {
    boost::shared_ptr<T>::operator=(r);
    T *p = boost::shared_ptr<T>::operator->();
    assert(expid == -1 || p == NULL || p->expr_id == expid);
    return *this;
  }

  template <class Y, int I>
  irep_container &operator=(const irep_container<Y, I> &ref)
  {
    *this = boost::shared_polymorphic_cast<T, Y>
            (static_cast<const boost::shared_ptr<Y> &>(ref));
    return *this;
  }

  const T &operator*() const
  {
    return *boost::shared_ptr<T>::get();
  }

  const T * operator-> () const // never throws
  {
    return boost::shared_ptr<T>::operator->();
  }

  const T * get() const // never throws
  {
    return boost::shared_ptr<T>::get();
  }

  T * get() // never throws
  {
    detach();
    return boost::shared_ptr<T>::get();
  }

  void detach(void)
  {
    if (this->use_count() == 1)
      return; // No point remunging oneself if we're the only user of the ptr.

    // Assign-operate ourself into containing a fresh copy of the data. This
    // creates a new reference counted object, and assigns it to ourself,
    // which causes the existing reference to be decremented.
    const T *foo = boost::shared_ptr<T>::get();
    *this = foo->clone();
    return;
  }
};

typedef boost::shared_ptr<type2t> type2tc;
typedef irep_container<expr2t, -1> expr2tc;

typedef std::pair<std::string,std::string> member_entryt;
typedef std::list<member_entryt> list_of_memberst;

template <class T>
static inline std::string type_to_string(const T &theval, int indent);

template <class T>
static inline bool do_type_cmp(const T &side1, const T &side2);

template <class T>
static inline int do_type_lt(const T &side1, const T &side2);

template <class T>
static inline void do_type_crc(const T &theval, boost::crc_32_type &crc);

template <class T>
static inline void do_type_list_operands(const T &theval,
                                         std::list<const expr2tc*> &inp);

template <class T>
static inline void do_type_list_operands(T& theval,
                                         std::list<expr2tc*> &inp);

/** Base class for all types */
class type2t
{
public:
  /** Enumeration identifying each sort of type.
   *  The idea being that we might (for whatever reason) at runtime need to fall
   *  back onto identifying a type through just one field, for some reason. It's
   *  also highly useful for debugging */
  enum type_ids {
    bool_id,
    empty_id,
    symbol_id,
    struct_id,
    union_id,
    code_id,
    array_id,
    pointer_id,
    unsignedbv_id,
    signedbv_id,
    fixedbv_id,
    string_id,
    cpp_name_id,
    end_type_id
  };

  // Class to be thrown when attempting to fetch the width of a symbolic type,
  // such as empty or code. Caller will have to worry about what to do about
  // that.
  class symbolic_type_excp {
  };

protected:
  type2t(type_ids id);
  type2t(const type2t &ref);

public:
  virtual void convert_smt_type(prop_convt &obj, void *&arg) const = 0;
  virtual unsigned int get_width(void) const = 0;
  bool operator==(const type2t &ref) const;
  bool operator!=(const type2t &ref) const;
  bool operator<(const type2t &ref) const;
  int ltchecked(const type2t &ref) const;
  std::string pretty(unsigned int indent = 0) const;
  void dump(void) const;
  uint32_t crc(void) const;
  bool cmpchecked(const type2t &ref) const;
  virtual bool cmp(const type2t &ref) const = 0;
  virtual int lt(const type2t &ref) const;
  virtual list_of_memberst tostring(unsigned int indent) const = 0;
  virtual void do_crc(boost::crc_32_type &crc) const;

  /** Instance of type_ids recording this types type. */
  type_ids type_id;
};


std::string get_type_id(const type2t &type);

static inline std::string get_type_id(const type2tc &type)
{
  return get_type_id(*type);
}

/** Base class for all expressions */
class expr2t
{
public:
  /** Enumeration identifying each sort of expr.
   *  The idea being to permit runtime identification of a type for debugging or
   *  otherwise. See type2t::type_ids. */
  enum expr_ids {
    constant_int_id,
    constant_fixedbv_id,
    constant_bool_id,
    constant_string_id,
    constant_struct_id,
    constant_union_id,
    constant_array_id,
    constant_array_of_id,
    symbol_id,
    typecast_id,
    if_id,
    equality_id,
    notequal_id,
    lessthan_id,
    greaterthan_id,
    lessthanequal_id,
    greaterthanequal_id,
    not_id,
    and_id,
    or_id,
    xor_id,
    implies_id,
    bitand_id,
    bitor_id,
    bitxor_id,
    bitnand_id,
    bitnor_id,
    bitnxor_id,
    bitnot_id,
    lshr_id,
    neg_id,
    abs_id,
    add_id,
    sub_id,
    mul_id,
    div_id,
    modulus_id,
    shl_id,
    ashr_id,
    dynamic_object_id, // Not converted in Z3, only in goto-symex
    same_object_id,
    pointer_offset_id,
    pointer_object_id,
    address_of_id,
    byte_extract_id,
    byte_update_id,
    with_id,
    member_id,
    index_id,
    zero_string_id,
    zero_length_string_id,
    isnan_id,
    overflow_id,
    overflow_cast_id,
    overflow_neg_id,
    unknown_id,
    invalid_id,
    null_object_id,
    dereference_id,
    valid_object_id,
    deallocated_obj_id,
    dynamic_size_id,
    sideeffect_id,
    code_block_id,
    code_assign_id,
    code_init_id,
    code_decl_id,
    code_printf_id,
    code_expression_id,
    code_return_id,
    code_skip_id,
    code_free_id,
    code_goto_id,
    object_descriptor_id,
    code_function_call_id,
    code_comma_id,
    invalid_pointer_id,
    buffer_size_id,
    code_asm_id,
    to_bv_typecast_id,
    from_bv_typecast_id,
    code_cpp_del_array_id,
    code_cpp_delete_id,
    code_cpp_catch_id,
    code_cpp_throw_id,
    end_expr_id
  };

  typedef std::list<const expr2tc*> expr_operands;
  typedef std::list<expr2tc*> Expr_operands;

protected:
  expr2t(const type2tc type, expr_ids id);
  expr2t(const expr2t &ref);

public:
  /** Clone method. Entirely self explanatory */
  virtual expr2tc clone(void) const = 0;

  virtual void convert_smt(prop_convt &obj, void *&arg) const = 0;

  bool operator==(const expr2t &ref) const;
  bool operator<(const expr2t &ref) const;
  bool operator!=(const expr2t &ref) const;
  int ltchecked(const expr2t &ref) const;
  std::string pretty(unsigned int indent = 0) const;
  unsigned long num_nodes(void) const;
  unsigned long depth(void) const;
  void dump(void) const;
  uint32_t crc(void) const;
  virtual bool cmp(const expr2t &ref) const;
  virtual int lt(const expr2t &ref) const;
  virtual list_of_memberst tostring(unsigned int indent) const = 0;
  virtual void do_crc(boost::crc_32_type &crc) const;
  virtual void list_operands(std::list<const expr2tc*> &inp) const = 0;

  // Caution - updating sub operands of an expr2t *must* always preserve type
  // correctness, as there's no way to check that an expr expecting a pointer
  // type operand *always* has a pointer type operand.
  // This list operands method should be protected; however it's required on
  // account of all those places where exprs are rewritten in place. Ideally,
  // "all those places" shouldn't exist in the future.
  virtual void list_operands(std::list<expr2tc*> &inp) = 0;
  virtual expr2t * clone_raw(void) const = 0;

  expr2tc simplify(void) const;
  // Shallow -> one level only. second indicates that this is its second
  // invocation, after a first invocation where all its operands aren't
  // simplified.
  virtual expr2tc do_simplify(bool second = false) const;

  /** Instance of expr_ids recording tihs exprs type. */
  const expr_ids expr_id;

  /** Type of this expr. All exprs have a type. */
  type2tc type;
};

std::string get_expr_id(const expr2t &expr);
static inline std::string get_expr_id(const expr2tc &expr)
{
  return get_expr_id(*expr);
}

// for "ESBMC templates",
namespace esbmct {

  class blank_method_operand {
  };

  const unsigned int num_type_fields = 5;

  template <class derived, class subclass,
     typename field1_type = const expr2t::expr_ids, class field1_class = expr2t,
     field1_type field1_class::*field1_ptr = &field1_class::expr_id,
     typename field2_type = const expr2t::expr_ids, class field2_class = expr2t,
     field2_type field2_class::*field2_ptr = &field2_class::expr_id,
     typename field3_type = const expr2t::expr_ids, class field3_class = expr2t,
     field3_type field3_class::*field3_ptr = &field3_class::expr_id,
     typename field4_type = const expr2t::expr_ids, class field4_class = expr2t,
     field4_type field4_class::*field4_ptr = &field4_class::expr_id,
     typename field5_type = const expr2t::expr_ids, class field5_class = expr2t,
     field5_type field5_class::*field5_ptr = &field5_class::expr_id>
  class expr_methods : public subclass
  {
    class dummy_type_tag {
      typedef int type;
    };

  public:
    template <class arbitary = dummy_type_tag>
    expr_methods(const type2tc &t, expr2t::expr_ids id,
                 typename boost::lazy_enable_if<boost::fusion::result_of::equal_to<subclass,expr2t>, arbitary >::type* = NULL)
      : subclass(t, id) { }

    template <class arbitary = dummy_type_tag>
    expr_methods(const type2tc &t, expr2t::expr_ids id,
                 const field1_type &arg1,
                 typename boost::lazy_disable_if<boost::fusion::result_of::equal_to<field1_type,expr2t::expr_ids>, arbitary >::type* = NULL,
                 typename boost::lazy_disable_if<boost::mpl::not_<boost::fusion::result_of::equal_to<field2_type,expr2t::expr_ids> >, arbitary >::type* = NULL)
      : subclass(t, id, arg1) { }

    template <class arbitary = dummy_type_tag>
    expr_methods(const type2tc &t, expr2t::expr_ids id, const field1_type &arg1,
                 const field2_type &arg2,
                 typename boost::lazy_disable_if<boost::fusion::result_of::equal_to<field2_type,expr2t::expr_ids>, arbitary >::type* = NULL,
                 typename boost::lazy_disable_if<boost::mpl::not_<boost::fusion::result_of::equal_to<field3_type,expr2t::expr_ids> >, arbitary >::type* = NULL)
      : subclass(t, id, arg1, arg2) { }

    template <class arbitary = dummy_type_tag>
    expr_methods(const type2tc &t, expr2t::expr_ids id, const field1_type &arg1,
                 const field2_type &arg2, const field3_type &arg3,
                 typename boost::lazy_disable_if<boost::fusion::result_of::equal_to<field3_type,expr2t::expr_ids>, arbitary >::type* = NULL,
                 typename boost::lazy_disable_if<boost::mpl::not_<boost::fusion::result_of::equal_to<field4_type,expr2t::expr_ids> >, arbitary >::type* = NULL)
      : subclass(t, id, arg1, arg2, arg3) { }

    template <class arbitary = dummy_type_tag>
    expr_methods(const type2tc &t, expr2t::expr_ids id, const field1_type &arg1,
                 const field2_type &arg2, const field3_type &arg3,
                 const field4_type &arg4,
                 typename boost::lazy_disable_if<boost::fusion::result_of::equal_to<field4_type,expr2t::expr_ids>, arbitary >::type* = NULL,
                 typename boost::lazy_disable_if<boost::mpl::not_<boost::fusion::result_of::equal_to<field5_type,expr2t::expr_ids> >, arbitary >::type* = NULL)
      : subclass(t, id, arg1, arg2, arg3, arg4) { }


    template <class arbitary = dummy_type_tag>
    expr_methods(const type2tc &t, expr2t::expr_ids id, const field1_type &arg1,
                 const field2_type &arg2, const field3_type &arg3,
                 const field4_type &arg4, const field5_type &arg5,
                 typename boost::lazy_disable_if<boost::fusion::result_of::equal_to<field4_type,expr2t::expr_ids>, arbitary >::type* = NULL)
      : subclass(t, id, arg1, arg2, arg3, arg4, arg5) { }

    expr_methods(const expr_methods<derived, subclass,
                                    field1_type, field1_class, field1_ptr,
                                    field2_type, field2_class, field2_ptr,
                                    field3_type, field3_class, field3_ptr,
                                    field4_type, field4_class, field4_ptr,
                                    field5_type, field5_class, field5_ptr> &ref)
      : subclass(ref) { }

    virtual void convert_smt(prop_convt &obj, void *&arg) const;
    virtual expr2tc clone(void) const;
    virtual list_of_memberst tostring(unsigned int indent) const;
    virtual bool cmp(const expr2t &ref) const;
    virtual int lt(const expr2t &ref) const;
    virtual void do_crc(boost::crc_32_type &crc) const;
    virtual void list_operands(std::list<const expr2tc*> &inp) const;
  protected:
    virtual void list_operands(std::list<expr2tc*> &inp);
    virtual expr2t *clone_raw(void) const;
  };

  template <class derived, class subclass,
          typename field1_type = type2t::type_ids, class field1_class = type2t,
          field1_type field1_class::*field1_ptr = &field1_class::type_id,
          typename field2_type = type2t::type_ids, class field2_class = type2t,
          field2_type field2_class::*field2_ptr = &field2_class::type_id,
          typename field3_type = type2t::type_ids, class field3_class = type2t,
          field3_type field3_class::*field3_ptr = &field3_class::type_id,
          typename field4_type = type2t::type_ids, class field4_class = type2t,
          field4_type field4_class::*field4_ptr = &field4_class::type_id,
          typename field5_type = type2t::type_ids, class field5_class = type2t,
          field5_type field5_class::*field5_ptr = &field5_class::type_id>
  class type_methods : public subclass
  {
    class dummy_type_tag {
      typedef int type;
    };

  public:
    template <class arbitary = dummy_type_tag>
    type_methods(type2t::type_ids id,
                 typename boost::lazy_enable_if<boost::fusion::result_of::equal_to<subclass,type2t>, arbitary >::type* = NULL)
      : subclass(id) { }

    template <class arbitary = dummy_type_tag>
    type_methods(type2t::type_ids id,
                 const field1_type &arg1,
                 typename boost::lazy_disable_if<boost::fusion::result_of::equal_to<field1_type,type2t::type_ids>, arbitary >::type* = NULL,
                 typename boost::lazy_disable_if<boost::mpl::not_<boost::fusion::result_of::equal_to<field2_type,type2t::type_ids> >, arbitary >::type* = NULL)
      : subclass(id, arg1) { }

    template <class arbitary = dummy_type_tag>
    type_methods(type2t::type_ids id, const field1_type &arg1,
                 const field2_type &arg2,
                 typename boost::lazy_disable_if<boost::fusion::result_of::equal_to<field2_type,type2t::type_ids>, arbitary >::type* = NULL,
                 typename boost::lazy_disable_if<boost::mpl::not_<boost::fusion::result_of::equal_to<field3_type,type2t::type_ids> >, arbitary >::type* = NULL)
      : subclass(id, arg1, arg2) { }

    template <class arbitary = dummy_type_tag>
    type_methods(type2t::type_ids id, const field1_type &arg1,
                 const field2_type &arg2, const field3_type &arg3,
                 typename boost::lazy_disable_if<boost::fusion::result_of::equal_to<field3_type,type2t::type_ids>, arbitary >::type* = NULL,
                 typename boost::lazy_disable_if<boost::mpl::not_<boost::fusion::result_of::equal_to<field4_type,type2t::type_ids> >, arbitary >::type* = NULL)
      : subclass(id, arg1, arg2, arg3) { }

    template <class arbitary = dummy_type_tag>
    type_methods(type2t::type_ids id, const field1_type &arg1,
                 const field2_type &arg2, const field3_type &arg3,
                 const field4_type &arg4,
                 typename boost::lazy_disable_if<boost::fusion::result_of::equal_to<field4_type,type2t::type_ids>, arbitary >::type* = NULL,
                 typename boost::lazy_disable_if<boost::mpl::not_<boost::fusion::result_of::equal_to<field5_type,type2t::type_ids> >, arbitary >::type* = NULL)
      : subclass(id, arg1, arg2, arg3, arg4) { }

    template <class arbitary = dummy_type_tag>
    type_methods(type2t::type_ids id, const field1_type &arg1,
                 const field2_type &arg2, const field3_type &arg3,
                 const field4_type &arg4, const field5_type &arg5,
                 typename boost::lazy_disable_if<boost::fusion::result_of::equal_to<field4_type,type2t::type_ids>, arbitary >::type* = NULL)
      : subclass(id, arg1, arg2, arg3, arg4, arg5) { }

    type_methods(const type_methods<derived, subclass,
                                    field1_type, field1_class, field1_ptr,
                                    field2_type, field2_class, field2_ptr,
                                    field3_type, field3_class, field3_ptr,
                                    field4_type, field4_class, field4_ptr,
                                    field5_type, field5_class, field5_ptr> &ref)
      : subclass(ref) { }

    virtual void convert_smt_type(prop_convt &obj, void *&arg) const;
    virtual type2tc clone(void) const;
    virtual list_of_memberst tostring(unsigned int indent) const;
    virtual bool cmp(const type2t &ref) const;
    virtual int lt(const type2t &ref) const;
    virtual void do_crc(boost::crc_32_type &crc) const;
  };
}; // esbmct

// So - make some type definitions for the different types we're going to be
// working with. This is to avoid the repeated use of template names in later
// definitions.

// Start with forward class definitions

class bool_type2t;
class empty_type2t;
class symbol_type2t;
class struct_type2t;
class union_type2t;
class bv_type2t;
class unsignedbv_type2t;
class signedbv_type2t;
class code_type2t;
class array_type2t;
class pointer_type2t;
class fixedbv_type2t;
class string_type2t;
class cpp_name_type2t;

// We also require in advance, the actual classes that store type data.

class symbol_type_data : public type2t
{
public:
  symbol_type_data(type2t::type_ids id, const dstring sym_name) :
    type2t (id), symbol_name(sym_name) {}
  symbol_type_data(const symbol_type_data &ref) :
    type2t (ref), symbol_name(ref.symbol_name) { }

  irep_idt symbol_name;
};

class struct_union_data : public type2t
{
public:
  struct_union_data(type2t::type_ids id, const std::vector<type2tc> &membs,
                     const std::vector<irep_idt> &names, const irep_idt &n)
    : type2t(id), members(membs), member_names(names), name(n) { }
  struct_union_data(const struct_union_data &ref)
    : type2t(ref), members(ref.members), member_names(ref.member_names),
      name(ref.name) { }

  const std::vector<type2tc> & get_structure_members(void) const;
  const std::vector<irep_idt> & get_structure_member_names(void) const;
  const irep_idt & get_structure_name(void) const;

  std::vector<type2tc> members;
  std::vector<irep_idt> member_names;
  irep_idt name;
};

class bv_data : public type2t
{
public:
  bv_data(type2t::type_ids id, unsigned int w) : type2t(id), width(w) { }
  bv_data(const bv_data &ref) : type2t(ref), width(ref.width) { }

  virtual unsigned int get_width(void) const;

  unsigned int width;
};

class code_data : public type2t
{
public:
  code_data(type2t::type_ids id, const std::vector<type2tc> &args,
            const type2tc &ret, const std::vector<irep_idt> &names, bool e)
    : type2t(id), arguments(args), ret_type(ret), argument_names(names),
      ellipsis(e) { }
  code_data(const code_data &ref)
    : type2t(ref), arguments(ref.arguments), ret_type(ref.ret_type),
      argument_names(ref.argument_names), ellipsis(ref.ellipsis) { }

  virtual unsigned int get_width(void) const;

  std::vector<type2tc> arguments;
  type2tc ret_type;
  std::vector<irep_idt> argument_names;
  bool ellipsis;
};

class array_data : public type2t
{
public:
  array_data(type2t::type_ids id, const type2tc &st, const expr2tc &sz, bool i)
    : type2t(id), subtype(st), array_size(sz), size_is_infinite(i) { }
  array_data(const array_data &ref)
    : type2t(ref), subtype(ref.subtype), array_size(ref.array_size),
      size_is_infinite(ref.size_is_infinite) { }

  type2tc subtype;
  expr2tc array_size;
  bool size_is_infinite;
};

class pointer_data : public type2t
{
public:
  pointer_data(type2t::type_ids id, const type2tc &st)
    : type2t(id), subtype(st) { }
  pointer_data(const pointer_data &ref)
    : type2t(ref), subtype(ref.subtype) { }

  type2tc subtype;
};

class fixedbv_data : public type2t
{
public:
  fixedbv_data(type2t::type_ids id, unsigned int w, unsigned int ib)
    : type2t(id), width(w), integer_bits(ib) { }
  fixedbv_data(const fixedbv_data &ref)
    : type2t(ref), width(ref.width), integer_bits(ref.integer_bits) { }

  unsigned int width;
  unsigned int integer_bits;
};

class string_data : public type2t
{
public:
  string_data(type2t::type_ids id, unsigned int w)
    : type2t(id), width(w) { }
  string_data(const string_data &ref)
    : type2t(ref), width(ref.width) { }

  unsigned int width;
};

class cpp_name_data : public type2t
{
public:
  cpp_name_data(type2t::type_ids id, const irep_idt &n,
                const std::vector<type2tc> &templ_args)
    : type2t(id), name(n), template_args(templ_args) { }
  cpp_name_data(const cpp_name_data &ref)
    : type2t(ref), name(ref.name), template_args(ref.template_args) { }

  irep_idt name;
  std::vector<type2tc> template_args;
};

// Then give them a typedef name

typedef esbmct::type_methods<bool_type2t, type2t> bool_type_methods;
typedef esbmct::type_methods<empty_type2t, type2t> empty_type_methods;
typedef esbmct::type_methods<symbol_type2t, symbol_type_data, irep_idt,
        symbol_type_data, &symbol_type_data::symbol_name> symbol_type_methods;
typedef esbmct::type_methods<struct_type2t, struct_union_data,
    std::vector<type2tc>, struct_union_data, &struct_union_data::members,
    std::vector<irep_idt>, struct_union_data, &struct_union_data::member_names,
    irep_idt, struct_union_data, &struct_union_data::name>
    struct_type_methods;
typedef esbmct::type_methods<union_type2t, struct_union_data,
    std::vector<type2tc>, struct_union_data, &struct_union_data::members,
    std::vector<irep_idt>, struct_union_data, &struct_union_data::member_names,
    irep_idt, struct_union_data, &struct_union_data::name>
    union_type_methods;
typedef esbmct::type_methods<unsignedbv_type2t, bv_data, unsigned int, bv_data,
    &bv_data::width> unsignedbv_type_methods;
typedef esbmct::type_methods<signedbv_type2t, bv_data, unsigned int, bv_data,
    &bv_data::width> signedbv_type_methods;
typedef esbmct::type_methods<code_type2t, code_data,
    std::vector<type2tc>, code_data, &code_data::arguments,
    type2tc, code_data, &code_data::ret_type,
    std::vector<irep_idt>, code_data, &code_data::argument_names,
    bool, code_data, &code_data::ellipsis>
    code_type_methods;
typedef esbmct::type_methods<array_type2t, array_data,
    type2tc, array_data, &array_data::subtype,
    expr2tc, array_data, &array_data::array_size,
    bool, array_data, &array_data::size_is_infinite>
    array_type_methods;
typedef esbmct::type_methods<pointer_type2t, pointer_data,
    type2tc, pointer_data, &pointer_data::subtype>
    pointer_type_methods;
typedef esbmct::type_methods<fixedbv_type2t, fixedbv_data,
    unsigned int, fixedbv_data, &fixedbv_data::width,
    unsigned int, fixedbv_data, &fixedbv_data::integer_bits>
    fixedbv_type_methods;
typedef esbmct::type_methods<string_type2t, string_data,
    unsigned int, string_data, &string_data::width>
    string_type_methods;
typedef esbmct::type_methods<cpp_name_type2t, cpp_name_data,
        irep_idt, cpp_name_data, &cpp_name_data::name,
        std::vector<type2tc>, cpp_name_data, &cpp_name_data::template_args>
        cpp_name_type_methods;

/** Boolean type. No additional data */
class bool_type2t : public bool_type_methods
{
public:
  bool_type2t(void) : bool_type_methods (bool_id) {}
  bool_type2t(const bool_type2t &ref) : bool_type_methods(ref) {}
  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Empty type. For void pointers and the like, with no type. No extra data */
class empty_type2t : public empty_type_methods
{
public:
  empty_type2t(void) : empty_type_methods(empty_id) {}
  empty_type2t(const empty_type2t &ref) : empty_type_methods(ref) { }
  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Symbol type. Temporary, prior to linking up types after parsing, or when
 *  a struct/array contains a recursive pointer to its own type. */

class symbol_type2t : public symbol_type_methods
{
public:
  symbol_type2t(const dstring sym_name) :
    symbol_type_methods(symbol_id, sym_name) { }
  symbol_type2t(const symbol_type2t &ref) :
    symbol_type_methods(ref) { }
  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

class struct_type2t : public struct_type_methods
{
public:
  struct_type2t(std::vector<type2tc> &members, std::vector<irep_idt> memb_names,
                irep_idt name)
    : struct_type_methods(struct_id, members, memb_names, name) {}
  struct_type2t(const struct_type2t &ref) : struct_type_methods(ref) {}
  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

class union_type2t : public union_type_methods
{
public:
  union_type2t(std::vector<type2tc> &members, std::vector<irep_idt> memb_names,
                irep_idt name)
    : union_type_methods(union_id, members, memb_names, name) {}
  union_type2t(const union_type2t &ref) : union_type_methods(ref) {}
  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

class unsignedbv_type2t : public unsignedbv_type_methods
{
public:
  unsignedbv_type2t(unsigned int width)
    : unsignedbv_type_methods(unsignedbv_id, width) { }
  unsignedbv_type2t(const unsignedbv_type2t &ref)
    : unsignedbv_type_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class signedbv_type2t : public signedbv_type_methods
{
public:
  signedbv_type2t(signed int width)
    : signedbv_type_methods(signedbv_id, width) { }
  signedbv_type2t(const signedbv_type2t &ref)
    : signedbv_type_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

/** Empty type. For void pointers and the like, with no type. No extra data */
class code_type2t : public code_type_methods
{
public:
  code_type2t(const std::vector<type2tc> &args, const type2tc &ret_type,
              const std::vector<irep_idt> &names, bool e)
    : code_type_methods(code_id, args, ret_type, names, e)
  { assert(args.size() == names.size()); }
  code_type2t(const code_type2t &ref) : code_type_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

/** Array type. Comes with a subtype of the array and a size that might be
 *  constant, might be nondeterministic. */
class array_type2t : public array_type_methods
{
public:
  array_type2t(const type2tc subtype, const expr2tc size, bool inf)
    : array_type_methods (array_id, subtype, size, inf) { }
  array_type2t(const array_type2t &ref)
    : array_type_methods(ref) { }

  virtual unsigned int get_width(void) const;

  // Exception for invalid manipulations of an infinitely sized array. No actual
  // data stored.
  class inf_sized_array_excp {
  };

  // Exception for invalid manipultions of dynamically sized arrays. No actual
  // data stored.
  class dyn_sized_array_excp {
  public:
    dyn_sized_array_excp(const expr2tc _size) : size(_size) {}
    expr2tc size;
  };

  static std::string field_names[esbmct::num_type_fields];
};

/** Pointer type. Simply has a subtype, of what it points to. No other
 *  attributes */
class pointer_type2t : public pointer_type_methods
{
public:
  pointer_type2t(const type2tc subtype)
    : pointer_type_methods(pointer_id, subtype) { }
  pointer_type2t(const pointer_type2t &ref)
    : pointer_type_methods(ref) { }
  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

class fixedbv_type2t : public fixedbv_type_methods
{
public:
  fixedbv_type2t(unsigned int width, unsigned int integer)
    : fixedbv_type_methods(fixedbv_id, width, integer) { }
  fixedbv_type2t(const fixedbv_type2t &ref)
    : fixedbv_type_methods(ref) { }
  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

class string_type2t : public string_type_methods
{
public:
  string_type2t(unsigned int elements)
    : string_type_methods(string_id, elements) { }
  string_type2t(const string_type2t &ref)
    : string_type_methods(ref) { }
  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

class cpp_name_type2t : public cpp_name_type_methods
{
public:
  cpp_name_type2t(const irep_idt &n, const std::vector<type2tc> &ta)
    : cpp_name_type_methods(cpp_name_id, n, ta){}
  cpp_name_type2t(const cpp_name_type2t &ref)
    : cpp_name_type_methods(ref) { }

  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

// Generate some "is-this-a-blah" macros, and type conversion macros. This is
// fine in terms of using/ keywords in syntax, because the preprocessor
// preprocesses everything out. One more used to C++ templates might raise their
// eyebrows at using the preprocessor; nuts to you, this works.
#ifdef NDEBUG
#define dynamic_cast static_cast
#endif
#define type_macros(name) \
  inline bool is_##name##_type(const type2tc &t) \
    { return t->type_id == type2t::name##_id; } \
  inline const name##_type2t & to_##name##_type(const type2tc &t) \
    { return dynamic_cast<const name##_type2t &> (*t.get()); } \
  inline name##_type2t & to_##name##_type(type2tc &t) \
    { return dynamic_cast<name##_type2t &> (*t.get()); }

type_macros(bool);
type_macros(empty);
type_macros(symbol);
type_macros(struct);
type_macros(union);
type_macros(code);
type_macros(array);
type_macros(pointer);
type_macros(unsignedbv);
type_macros(signedbv);
type_macros(fixedbv);
type_macros(string);
type_macros(cpp_name);
#undef type_macros
#ifdef dynamic_cast
#undef dynamic_cast
#endif

inline bool is_bv_type(const type2tc &t) \
{ return (t->type_id == type2t::unsignedbv_id ||
          t->type_id == type2t::signedbv_id); }

inline bool is_number_type(const type2tc &t) \
{ return (t->type_id == type2t::unsignedbv_id ||
          t->type_id == type2t::signedbv_id ||
          t->type_id == type2t::fixedbv_id); }

// And now, some more utilities.
class type_poolt {
public:
  type_poolt(void);

  type2tc bool_type;
  type2tc empty_type;

  const type2tc &get_bool() const { return bool_type; }
  const type2tc &get_empty() const { return empty_type; }

  // For other types, have a pool of them for quick lookup.
  std::map<const typet, type2tc> struct_map;
  std::map<const typet, type2tc> union_map;
  std::map<const typet, type2tc> array_map;
  std::map<const typet, type2tc> pointer_map;
  std::map<const typet, type2tc> unsignedbv_map;
  std::map<const typet, type2tc> signedbv_map;
  std::map<const typet, type2tc> fixedbv_map;
  std::map<const typet, type2tc> string_map;
  std::map<const typet, type2tc> symbol_map;
  std::map<const typet, type2tc> code_map;

  // And refs to some of those for /really/ quick lookup;
  const type2tc *uint8;
  const type2tc *uint16;
  const type2tc *uint32;
  const type2tc *uint64;
  const type2tc *int8;
  const type2tc *int16;
  const type2tc *int32;
  const type2tc *int64;

  // Some accessors.
  const type2tc &get_struct(const typet &val);
  const type2tc &get_union(const typet &val);
  const type2tc &get_array(const typet &val);
  const type2tc &get_pointer(const typet &val);
  const type2tc &get_unsignedbv(const typet &val);
  const type2tc &get_signedbv(const typet &val);
  const type2tc &get_fixedbv(const typet &val);
  const type2tc &get_string(const typet &val);
  const type2tc &get_symbol(const typet &val);
  const type2tc &get_code(const typet &val);

  const type2tc &get_uint(unsigned int size);
  const type2tc &get_int(unsigned int size);

  const type2tc &get_uint8() const { return *uint8; }
  const type2tc &get_uint16() const { return *uint16; }
  const type2tc &get_uint32() const { return *uint32; }
  const type2tc &get_uint64() const { return *uint64; }
  const type2tc &get_int8() const { return *int8; }
  const type2tc &get_int16() const { return *int16; }
  const type2tc &get_int32() const { return *int32; }
  const type2tc &get_int64() const { return *int64; }
};

extern type_poolt type_pool;

// Start of definitions for expressions. Forward decs,

class constant2t;
class constant_int2t;
class constant_fixedbv2t;
class constant_bool2t;
class constant_string2t;
class constant_datatype2t;
class constant_struct2t;
class constant_union2t;
class constant_array2t;
class constant_array_of2t;
class symbol2t;
class typecast2t;
class to_bv_typecast2t;
class from_bv_typecast2t;
class if2t;
class equality2t;
class notequal2t;
class lessthan2t;
class greaterthan2t;
class lessthanequal2t;
class greaterthanequal2t;
class not2t;
class and2t;
class or2t;
class xor2t;
class implies2t;
class bitand2t;
class bitor2t;
class bitxor2t;
class bitnand2t;
class bitnor2t;
class bitnxor2t;
class lshr2t;
class bitnot2t;
class neg2t;
class abs2t;
class add2t;
class sub2t;
class mul2t;
class div2t;
class modulus2t;
class shl2t;
class ashr2t;
class same_object2t;
class pointer_offset2t;
class pointer_object2t;
class address_of2t;
class byte_extract2t;
class byte_update2t;
class with2t;
class member2t;
class index2t;
class zero_string2t;
class zero_length_string2t;
class isnan2t;
class overflow2t;
class overflow_cast2t;
class overflow_neg2t;
class unknown2t;
class invalid2t;
class null_object2t;
class dynamic_object2t;
class dereference2t;
class valid_object2t;
class deallocated_obj2t;
class dynamic_size2t;
class sideeffect2t;
class code_block2t;
class code_assign2t;
class code_init2t;
class code_decl2t;
class code_printf2t;
class code_expression2t;
class code_return2t;
class code_skip2t;
class code_free2t;
class code_goto2t;
class object_descriptor2t;
class code_function_call2t;
class code_comma2t;
class invalid_pointer2t;
class buffer_size2t;
class code_asm2t;
class code_cpp_del_array2t;
class code_cpp_delete2t;
class code_cpp_catch2t;
class code_cpp_throw2t;

// Data definitions.

class constant2t : public expr2t
{
public:
  constant2t(const type2tc &t, expr2t::expr_ids id) : expr2t(t, id) { }
  constant2t(const constant2t &ref) : expr2t(ref) { }
};

class constant_int_data : public constant2t
{
public:
  constant_int_data(const type2tc &t, expr2t::expr_ids id, const BigInt &bint)
    : constant2t(t, id), constant_value(bint) { }
  constant_int_data(const constant_int_data &ref)
    : constant2t(ref), constant_value(ref.constant_value) { }

  BigInt constant_value;
};

class constant_fixedbv_data : public constant2t
{
public:
  constant_fixedbv_data(const type2tc &t, expr2t::expr_ids id,
                        const fixedbvt &fbv)
    : constant2t(t, id), value(fbv) { }
  constant_fixedbv_data(const constant_fixedbv_data &ref)
    : constant2t(ref), value(ref.value) { }

  fixedbvt value;
};

class constant_datatype_data : public constant2t
{
public:
  constant_datatype_data(const type2tc &t, expr2t::expr_ids id,
                         const std::vector<expr2tc> &m)
    : constant2t(t, id), datatype_members(m) { }
  constant_datatype_data(const constant_datatype_data &ref)
    : constant2t(ref), datatype_members(ref.datatype_members) { }

  std::vector<expr2tc> datatype_members;
};

class constant_bool_data : public constant2t
{
public:
  constant_bool_data(const type2tc &t, expr2t::expr_ids id, bool value)
    : constant2t(t, id), constant_value(value) { }
  constant_bool_data(const constant_bool_data &ref)
    : constant2t(ref), constant_value(ref.constant_value) { }

  bool constant_value;
};

class constant_array_of_data : public constant2t
{
public:
  constant_array_of_data(const type2tc &t, expr2t::expr_ids id, expr2tc value)
    : constant2t(t, id), initializer(value) { }
  constant_array_of_data(const constant_array_of_data &ref)
    : constant2t(ref), initializer(ref.initializer) { }

  expr2tc initializer;
};

class constant_string_data : public constant2t
{
public:
  constant_string_data(const type2tc &t, expr2t::expr_ids id, const irep_idt &v)
    : constant2t(t, id), value(v) { }
  constant_string_data(const constant_string_data &ref)
    : constant2t(ref), value(ref.value) { }

  irep_idt value;
};

class symbol_data : public expr2t
{
public:
  symbol_data(const type2tc &t, expr2t::expr_ids id, const irep_idt &v)
    : expr2t(t, id), name(v) { }
  symbol_data(const symbol_data &ref)
    : expr2t(ref), name(ref.name) { }

  irep_idt name;
};

class typecast_data : public expr2t
{
public:
  typecast_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &v)
    : expr2t(t, id), from(v) { }
  typecast_data(const typecast_data &ref)
    : expr2t(ref), from(ref.from) { }

  expr2tc from;
};

class if_data : public expr2t
{
public:
  if_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &c,
                const expr2tc &tv, const expr2tc &fv)
    : expr2t(t, id), cond(c), true_value(tv), false_value(fv) { }
  if_data(const if_data &ref)
    : expr2t(ref), cond(ref.cond), true_value(ref.true_value),
      false_value(ref.false_value) { }

  expr2tc cond;
  expr2tc true_value;
  expr2tc false_value;
};

class relation_data : public expr2t
{
  public:
  relation_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &s1,
                const expr2tc &s2)
    : expr2t(t, id), side_1(s1), side_2(s2) { }
  relation_data(const relation_data &ref)
    : expr2t(ref), side_1(ref.side_1), side_2(ref.side_2) { }

  expr2tc side_1;
  expr2tc side_2;
};

class logical_ops : public expr2t
{
public:
  logical_ops(const type2tc &t, expr2t::expr_ids id)
    : expr2t(t, id) { }
  logical_ops(const logical_ops &ref)
    : expr2t(ref) { }
};

class not_data : public logical_ops
{
public:
  not_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &v)
    : logical_ops(t, id), value(v) { }
  not_data(const not_data &ref)
    : logical_ops(ref), value(ref.value) { }

  expr2tc value;
};

class logic_2ops : public logical_ops
{
public:
  logic_2ops(const type2tc &t, expr2t::expr_ids id, const expr2tc &s1,
             const expr2tc &s2)
    : logical_ops(t, id), side_1(s1), side_2(s2) { }
  logic_2ops(const logic_2ops &ref)
    : logical_ops(ref), side_1(ref.side_1), side_2(ref.side_2) { }

  expr2tc side_1;
  expr2tc side_2;
};

class bitops : public expr2t
{
public:
  bitops(const type2tc &t, expr2t::expr_ids id)
    : expr2t(t, id) { }
  bitops(const bitops &ref)
    : expr2t(ref) { }
};

class bitnot_data : public bitops
{
public:
  bitnot_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &v)
    : bitops(t, id), value(v) { }
  bitnot_data(const bitnot_data &ref)
    : bitops(ref), value(ref.value) { }

  expr2tc value;
};

class bit_2ops : public bitops
{
public:
  bit_2ops(const type2tc &t, expr2t::expr_ids id, const expr2tc &s1,
           const expr2tc &s2)
    : bitops(t, id), side_1(s1), side_2(s2) { }
  bit_2ops(const bit_2ops &ref)
    : bitops(ref), side_1(ref.side_1), side_2(ref.side_2) { }

  expr2tc side_1;
  expr2tc side_2;
};

class arith_ops : public expr2t
{
public:
  arith_ops(const type2tc &t, expr2t::expr_ids id)
    : expr2t(t, id) { }
  arith_ops(const arith_ops &ref)
    : expr2t(ref) { }
};

class arith_1op : public arith_ops
{
public:
  arith_1op(const type2tc &t, arith_ops::expr_ids id, const expr2tc &v)
    : arith_ops(t, id), value(v) { }
  arith_1op(const arith_1op &ref)
    : arith_ops(ref), value(ref.value) { }

  expr2tc value;
};

class arith_2ops : public arith_ops
{
public:
  arith_2ops(const type2tc &t, arith_ops::expr_ids id, const expr2tc &v1,
             const expr2tc &v2)
    : arith_ops(t, id), side_1(v1), side_2(v2) { }
  arith_2ops(const arith_2ops &ref)
    : arith_ops(ref), side_1(ref.side_1), side_2(ref.side_2) { }

  expr2tc side_1;
  expr2tc side_2;
};

class same_object_data : public expr2t
{
public:
  same_object_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &v1,
                   const expr2tc &v2)
    : expr2t(t, id), side_1(v1), side_2(v2) { }
  same_object_data(const same_object_data &ref)
    : expr2t(ref), side_1(ref.side_1), side_2(ref.side_2) { }

  expr2tc side_1;
  expr2tc side_2;
};

class pointer_ops : public expr2t
{
public:
  pointer_ops(const type2tc &t, expr2t::expr_ids id, const expr2tc &p)
    : expr2t(t, id), ptr_obj(p) { }
  pointer_ops(const pointer_ops &ref)
    : expr2t(ref), ptr_obj(ref.ptr_obj) { }

  expr2tc ptr_obj;
};

class byte_ops : public expr2t
{
public:
  byte_ops(const type2tc &t, expr2t::expr_ids id)
    : expr2t(t, id){ }
  byte_ops(const byte_ops &ref)
    : expr2t(ref) { }
};

class byte_extract_data : public byte_ops
{
public:
  byte_extract_data(const type2tc &t, expr2t::expr_ids id, bool be,
                    const expr2tc &s, const expr2tc &o)
    : byte_ops(t, id), big_endian(be), source_value(s), source_offset(o) { }
  byte_extract_data(const byte_extract_data &ref)
    : byte_ops(ref), big_endian(ref.big_endian), source_value(ref.source_value),
      source_offset(ref.source_offset) { }

  bool big_endian;
  expr2tc source_value;
  expr2tc source_offset;
};

class byte_update_data : public byte_ops
{
public:
  byte_update_data(const type2tc &t, expr2t::expr_ids id, bool be,
                    const expr2tc &s, const expr2tc &o, const expr2tc &v)
    : byte_ops(t, id), big_endian(be), source_value(s), source_offset(o),
      update_value(v) { }
  byte_update_data(const byte_update_data &ref)
    : byte_ops(ref), big_endian(ref.big_endian), source_value(ref.source_value),
      source_offset(ref.source_offset), update_value(ref.update_value) { }

  bool big_endian;
  expr2tc source_value;
  expr2tc source_offset;
  expr2tc update_value;
};

class datatype_ops : public expr2t
{
public:
  datatype_ops(const type2tc &t, expr2t::expr_ids id)
    : expr2t(t, id) { }
  datatype_ops(const datatype_ops &ref)
    : expr2t(ref) { }
};

class with_data : public datatype_ops
{
public:
  with_data(const type2tc &t, datatype_ops::expr_ids id, const expr2tc &sv,
            const expr2tc &uf, const expr2tc &uv)
    : datatype_ops(t, id), source_value(sv), update_field(uf), update_value(uv)
      { }
  with_data(const with_data &ref)
    : datatype_ops(ref), source_value(ref.source_value),
      update_field(ref.update_field), update_value(ref.update_value)
      { }

  expr2tc source_value;
  expr2tc update_field;
  expr2tc update_value;
};

class member_data : public datatype_ops
{
public:
  member_data(const type2tc &t, datatype_ops::expr_ids id, const expr2tc &sv,
              const irep_idt &m)
    : datatype_ops(t, id), source_value(sv), member(m) { }
  member_data(const member_data &ref)
    : datatype_ops(ref), source_value(ref.source_value), member(ref.member) { }

  expr2tc source_value;
  irep_idt member;
};

class index_data : public datatype_ops
{
public:
  index_data(const type2tc &t, datatype_ops::expr_ids id, const expr2tc &sv,
              const expr2tc &i)
    : datatype_ops(t, id), source_value(sv), index(i) { }
  index_data(const index_data &ref)
    : datatype_ops(ref), source_value(ref.source_value), index(ref.index) { }

  expr2tc source_value;
  expr2tc index;
};

class string_ops : public expr2t
{
public:
  string_ops(const type2tc &t, datatype_ops::expr_ids id, const expr2tc &s)
    : expr2t(t, id), string(s) { }
  string_ops(const string_ops &ref)
    : expr2t(ref), string(ref.string) { }

  expr2tc string;
};

class isnan_data : public expr2t
{
public:
  isnan_data(const type2tc &t, datatype_ops::expr_ids id, const expr2tc &in)
    : expr2t(t, id), value(in) { }
  isnan_data(const isnan_data &ref)
    : expr2t(ref), value(ref.value) { }

  expr2tc value;
};

class overflow_ops : public expr2t
{
public:
  overflow_ops(const type2tc &t, datatype_ops::expr_ids id, const expr2tc &v)
    : expr2t(t, id), operand(v) { }
  overflow_ops(const overflow_ops &ref)
    : expr2t(ref), operand(ref.operand) { }

  expr2tc operand;
};

class overflow_cast_data : public overflow_ops
{
public:
  overflow_cast_data(const type2tc &t, datatype_ops::expr_ids id,
                     const expr2tc &v, unsigned int b)
    : overflow_ops(t, id, v), bits(b) { }
  overflow_cast_data(const overflow_cast_data &ref)
    : overflow_ops(ref), bits(ref.bits) { }

  unsigned int bits;
};

class dynamic_object_data : public expr2t
{
public:
  dynamic_object_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &i,
                      bool inv, bool unk)
    : expr2t(t, id), instance(i), invalid(inv), unknown(unk) { }
  dynamic_object_data(const dynamic_object_data &ref)
    : expr2t(ref), instance(ref.instance), invalid(ref.invalid),
      unknown(ref.unknown) { }

  expr2tc instance;
  bool invalid;
  bool unknown;
};

class dereference_data : public expr2t
{
public:
  dereference_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &v)
    : expr2t(t, id), value(v) { }
  dereference_data(const dereference_data &ref)
    : expr2t(ref), value(ref.value) { }

  expr2tc value;
};

class object_ops : public expr2t
{
public:
  object_ops(const type2tc &t, expr2t::expr_ids id, const expr2tc &v)
    : expr2t(t, id), value(v) { }
  object_ops(const object_ops &ref)
    : expr2t(ref), value(ref.value) { }

  expr2tc value;
};

class sideeffect_data : public expr2t
{
public:
  sideeffect_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &op,
                  const expr2tc &sz, const type2tc &tp, unsigned int k,
                  const std::vector<expr2tc> &args)
    : expr2t(t, id), operand(op), size(sz), alloctype(tp), kind(k),
      arguments(args) { }
  sideeffect_data(const sideeffect_data &ref)
    : expr2t(ref), operand(ref.operand), size(ref.size),
      alloctype(ref.alloctype), kind(ref.kind), arguments(ref.arguments) { }

  expr2tc operand;
  expr2tc size;
  type2tc alloctype;
  unsigned int kind;
  std::vector<expr2tc> arguments;
};

class code_base : public expr2t
{
public:
  code_base(const type2tc &t, expr2t::expr_ids id)
    : expr2t(t, id) { }
  code_base(const code_base &ref)
    : expr2t(ref) { }
};

class code_block_data : public code_base
{
public:
  code_block_data(const type2tc &t, expr2t::expr_ids id,
                  const std::vector<expr2tc> &v)
    : code_base(t, id), operands(v) { }
  code_block_data(const code_block_data &ref)
    : code_base(ref), operands(ref.operands) { }

  std::vector<expr2tc> operands;
};

class code_assign_data : public code_base
{
public:
  code_assign_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &ta,
                   const expr2tc &s)
    : code_base(t, id), target(ta), source(s) { }
  code_assign_data(const code_assign_data &ref)
    : code_base(ref), target(ref.target), source(ref.source) { }

  expr2tc target;
  expr2tc source;
};

class code_decl_data : public code_base
{
public:
  code_decl_data(const type2tc &t, expr2t::expr_ids id, const irep_idt &v)
    : code_base(t, id), value(v) { }
  code_decl_data(const code_decl_data &ref)
    : code_base(ref), value(ref.value) { }

  irep_idt value;
};

class code_printf_data : public code_base
{
public:
  code_printf_data(const type2tc &t, expr2t::expr_ids id,
                   const std::vector<expr2tc> &v)
    : code_base(t, id), operands(v) { }
  code_printf_data(const code_printf_data &ref)
    : code_base(ref), operands(ref.operands) { }

  std::vector<expr2tc> operands;
};

class code_expression_data : public code_base
{
public:
  code_expression_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &o)
    : code_base(t, id), operand(o) { }
  code_expression_data(const code_expression_data &ref)
    : code_base(ref), operand(ref.operand) { }

  expr2tc operand;
};

class code_goto_data : public code_base
{
public:
  code_goto_data(const type2tc &t, expr2t::expr_ids id, const irep_idt &tg)
    : code_base(t, id), target(tg) { }
  code_goto_data(const code_goto_data &ref)
    : code_base(ref), target(ref.target) { }

  irep_idt target;
};

class object_desc_data : public expr2t
{
  public:
    object_desc_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &o,
                     const expr2tc &offs)
      : expr2t(t, id), object(o), offset(offs) { }
    object_desc_data(const object_desc_data &ref)
      : expr2t(ref), object(ref.object), offset(ref.offset) { }

    expr2tc object;
    expr2tc offset;
};

class code_funccall_data : public code_base
{
public:
  code_funccall_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &r,
                     const expr2tc &func, const std::vector<expr2tc> &ops)
    : code_base(t, id), ret(r), function(func), operands(ops) { }
  code_funccall_data(const code_funccall_data &ref)
    : code_base(ref), ret(ref.ret), function(ref.function),
      operands(ref.operands) { }

  expr2tc ret;
  expr2tc function;
  std::vector<expr2tc> operands;
};

class code_comma_data : public code_base
{
public:
  code_comma_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &s1,
                  const expr2tc &s2)
    : code_base(t, id), side_1(s1), side_2(s2) { }
  code_comma_data(const code_comma_data &ref)
    : code_base(ref), side_1(ref.side_1), side_2(ref.side_2) { }

  expr2tc side_1;
  expr2tc side_2;
};

class buffer_size_data : public expr2t
{
public:
  buffer_size_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &v)
    : expr2t(t, id), value(v) { }
  buffer_size_data(const buffer_size_data &ref)
    : expr2t(ref), value(ref.value) { }

  expr2tc value;
};

class code_asm_data : public code_base
{
public:
  code_asm_data(const type2tc &t, expr2t::expr_ids id, const irep_idt &v)
    : code_base(t, id), value(v) { }
  code_asm_data(const code_asm_data &ref)
    : code_base(ref), value(ref.value) { }

  irep_idt value;
};

class code_cpp_catch_data : public code_base
{
public:
  code_cpp_catch_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &o,
                      const std::vector<unsigned int> &el)
    : code_base(t, id), operand(o), excp_list(el) { }
  code_cpp_catch_data(const code_cpp_catch_data &ref)
    : code_base(ref), operand(ref.operand), excp_list(ref.excp_list) { }

  expr2tc operand;
  std::vector<unsigned int> excp_list;
};

class code_cpp_throw_data : public code_base
{
public:
  code_cpp_throw_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &o)
    : code_base(t, id), operand(o) { }
  code_cpp_throw_data(const code_cpp_throw_data &ref)
    : code_base(ref), operand(ref.operand) { }

  expr2tc operand;
};

// Give everything a typedef name

typedef esbmct::expr_methods<constant_int2t, constant_int_data,
        BigInt, constant_int_data, &constant_int_data::constant_value>
        constant_int_expr_methods;
typedef esbmct::expr_methods<constant_fixedbv2t, constant_fixedbv_data,
        fixedbvt, constant_fixedbv_data, &constant_fixedbv_data::value>
        constant_fixedbv_expr_methods;
typedef esbmct::expr_methods<constant_struct2t, constant_datatype_data,
        std::vector<expr2tc>, constant_datatype_data,
        &constant_datatype_data::datatype_members>
        constant_struct_expr_methods;
typedef esbmct::expr_methods<constant_union2t, constant_datatype_data,
        std::vector<expr2tc>, constant_datatype_data,
        &constant_datatype_data::datatype_members>
        constant_union_expr_methods;
typedef esbmct::expr_methods<constant_array2t, constant_datatype_data,
        std::vector<expr2tc>, constant_datatype_data,
        &constant_datatype_data::datatype_members>
        constant_array_expr_methods;
typedef esbmct::expr_methods<constant_bool2t, constant_bool_data,
        bool, constant_bool_data, &constant_bool_data::constant_value>
        constant_bool_expr_methods;
typedef esbmct::expr_methods<constant_array_of2t, constant_array_of_data,
        expr2tc, constant_array_of_data, &constant_array_of_data::initializer>
        constant_array_of_expr_methods;
typedef esbmct::expr_methods<constant_string2t, constant_string_data,
        irep_idt, constant_string_data, &constant_string_data::value>
        constant_string_expr_methods;
typedef esbmct::expr_methods<symbol2t, symbol_data,
        irep_idt, symbol_data, &symbol_data::name>
        symbol_expr_methods;
typedef esbmct::expr_methods<typecast2t, typecast_data,
        expr2tc, typecast_data, &typecast_data::from>
        typecast_expr_methods;
typedef esbmct::expr_methods<to_bv_typecast2t, typecast_data,
        expr2tc, typecast_data, &typecast_data::from>
        to_bv_typecast_expr_methods;
typedef esbmct::expr_methods<from_bv_typecast2t, typecast_data,
        expr2tc, typecast_data, &typecast_data::from>
        from_bv_typecast_expr_methods;
typedef esbmct::expr_methods<if2t, if_data,
        expr2tc, if_data, &if_data::cond,
        expr2tc, if_data, &if_data::true_value,
        expr2tc, if_data, &if_data::false_value>
        if_expr_methods;
typedef esbmct::expr_methods<equality2t, relation_data,
        expr2tc, relation_data, &relation_data::side_1,
        expr2tc, relation_data, &relation_data::side_2>
        equality_expr_methods;
typedef esbmct::expr_methods<notequal2t, relation_data,
        expr2tc, relation_data, &relation_data::side_1,
        expr2tc, relation_data, &relation_data::side_2>
        notequal_expr_methods;
typedef esbmct::expr_methods<lessthan2t, relation_data,
        expr2tc, relation_data, &relation_data::side_1,
        expr2tc, relation_data, &relation_data::side_2>
        lessthan_expr_methods;
typedef esbmct::expr_methods<greaterthan2t, relation_data,
        expr2tc, relation_data, &relation_data::side_1,
        expr2tc, relation_data, &relation_data::side_2>
        greaterthan_expr_methods;
typedef esbmct::expr_methods<lessthanequal2t, relation_data,
        expr2tc, relation_data, &relation_data::side_1,
        expr2tc, relation_data, &relation_data::side_2>
        lessthanequal_expr_methods;
typedef esbmct::expr_methods<greaterthanequal2t, relation_data,
        expr2tc, relation_data, &relation_data::side_1,
        expr2tc, relation_data, &relation_data::side_2>
        greaterthanequal_expr_methods;
typedef esbmct::expr_methods<not2t, not_data,
        expr2tc, not_data, &not_data::value>
        not_expr_methods;
typedef esbmct::expr_methods<and2t, logic_2ops,
        expr2tc, logic_2ops, &logic_2ops::side_1,
        expr2tc, logic_2ops, &logic_2ops::side_2>
        and_expr_methods;
typedef esbmct::expr_methods<or2t, logic_2ops,
        expr2tc, logic_2ops, &logic_2ops::side_1,
        expr2tc, logic_2ops, &logic_2ops::side_2>
        or_expr_methods;
typedef esbmct::expr_methods<xor2t, logic_2ops,
        expr2tc, logic_2ops, &logic_2ops::side_1,
        expr2tc, logic_2ops, &logic_2ops::side_2>
        xor_expr_methods;
typedef esbmct::expr_methods<implies2t, logic_2ops,
        expr2tc, logic_2ops, &logic_2ops::side_1,
        expr2tc, logic_2ops, &logic_2ops::side_2>
        implies_expr_methods;
typedef esbmct::expr_methods<bitand2t, bit_2ops,
        expr2tc, bit_2ops, &bit_2ops::side_1,
        expr2tc, bit_2ops, &bit_2ops::side_2>
        bitand_expr_methods;
typedef esbmct::expr_methods<bitor2t, bit_2ops,
        expr2tc, bit_2ops, &bit_2ops::side_1,
        expr2tc, bit_2ops, &bit_2ops::side_2>
        bitor_expr_methods;
typedef esbmct::expr_methods<bitxor2t, bit_2ops,
        expr2tc, bit_2ops, &bit_2ops::side_1,
        expr2tc, bit_2ops, &bit_2ops::side_2>
        bitxor_expr_methods;
typedef esbmct::expr_methods<bitnand2t, bit_2ops,
        expr2tc, bit_2ops, &bit_2ops::side_1,
        expr2tc, bit_2ops, &bit_2ops::side_2>
        bitnand_expr_methods;
typedef esbmct::expr_methods<bitnor2t, bit_2ops,
        expr2tc, bit_2ops, &bit_2ops::side_1,
        expr2tc, bit_2ops, &bit_2ops::side_2>
        bitnor_expr_methods;
typedef esbmct::expr_methods<bitnxor2t, bit_2ops,
        expr2tc, bit_2ops, &bit_2ops::side_1,
        expr2tc, bit_2ops, &bit_2ops::side_2>
        bitnxor_expr_methods;
typedef esbmct::expr_methods<lshr2t, bit_2ops,
        expr2tc, bit_2ops, &bit_2ops::side_1,
        expr2tc, bit_2ops, &bit_2ops::side_2>
        lshr_expr_methods;
typedef esbmct::expr_methods<bitnot2t, bitnot_data,
        expr2tc, bitnot_data, &bitnot_data::value>
        bitnot_expr_methods;
typedef esbmct::expr_methods<neg2t, arith_1op,
        expr2tc, arith_1op, &arith_1op::value>
        neg_expr_methods;
typedef esbmct::expr_methods<abs2t, arith_1op,
        expr2tc, arith_1op, &arith_1op::value>
        abs_expr_methods;
typedef esbmct::expr_methods<add2t, arith_2ops,
        expr2tc, arith_2ops, &arith_2ops::side_1,
        expr2tc, arith_2ops, &arith_2ops::side_2>
        add_expr_methods;
typedef esbmct::expr_methods<sub2t, arith_2ops,
        expr2tc, arith_2ops, &arith_2ops::side_1,
        expr2tc, arith_2ops, &arith_2ops::side_2>
        sub_expr_methods;
typedef esbmct::expr_methods<mul2t, arith_2ops,
        expr2tc, arith_2ops, &arith_2ops::side_1,
        expr2tc, arith_2ops, &arith_2ops::side_2>
        mul_expr_methods;
typedef esbmct::expr_methods<div2t, arith_2ops,
        expr2tc, arith_2ops, &arith_2ops::side_1,
        expr2tc, arith_2ops, &arith_2ops::side_2>
        div_expr_methods;
typedef esbmct::expr_methods<modulus2t, arith_2ops,
        expr2tc, arith_2ops, &arith_2ops::side_1,
        expr2tc, arith_2ops, &arith_2ops::side_2>
        modulus_expr_methods;
typedef esbmct::expr_methods<shl2t, arith_2ops,
        expr2tc, arith_2ops, &arith_2ops::side_1,
        expr2tc, arith_2ops, &arith_2ops::side_2>
        shl_expr_methods;
typedef esbmct::expr_methods<ashr2t, arith_2ops,
        expr2tc, arith_2ops, &arith_2ops::side_1,
        expr2tc, arith_2ops, &arith_2ops::side_2>
        ashr_expr_methods;
typedef esbmct::expr_methods<same_object2t, same_object_data,
        expr2tc, same_object_data, &same_object_data::side_1,
        expr2tc, same_object_data, &same_object_data::side_2>
        same_object_expr_methods;
typedef esbmct::expr_methods<pointer_offset2t, pointer_ops,
        expr2tc, pointer_ops, &pointer_ops::ptr_obj>
        pointer_offset_expr_methods;
typedef esbmct::expr_methods<pointer_object2t, pointer_ops,
        expr2tc, pointer_ops, &pointer_ops::ptr_obj>
        pointer_object_expr_methods;
typedef esbmct::expr_methods<address_of2t, pointer_ops,
        expr2tc, pointer_ops, &pointer_ops::ptr_obj>
        address_of_expr_methods;
typedef esbmct::expr_methods<byte_extract2t, byte_extract_data,
        bool, byte_extract_data, &byte_extract_data::big_endian,
        expr2tc, byte_extract_data, &byte_extract_data::source_value,
        expr2tc, byte_extract_data, &byte_extract_data::source_offset>
        byte_extract_expr_methods;
typedef esbmct::expr_methods<byte_update2t, byte_update_data,
        bool, byte_update_data, &byte_update_data::big_endian,
        expr2tc, byte_update_data, &byte_update_data::source_value,
        expr2tc, byte_update_data, &byte_update_data::source_offset,
        expr2tc, byte_update_data, &byte_update_data::update_value>
        byte_update_expr_methods;
typedef esbmct::expr_methods<with2t, with_data,
        expr2tc, with_data, &with_data::source_value,
        expr2tc, with_data, &with_data::update_field,
        expr2tc, with_data, &with_data::update_value>
        with_expr_methods;
typedef esbmct::expr_methods<member2t, member_data,
        expr2tc, member_data, &member_data::source_value,
        irep_idt, member_data, &member_data::member>
        member_expr_methods;
typedef esbmct::expr_methods<index2t, index_data,
        expr2tc, index_data, &index_data::source_value,
        expr2tc, index_data, &index_data::index>
        index_expr_methods;
typedef esbmct::expr_methods<zero_string2t, string_ops,
        expr2tc, string_ops, &string_ops::string>
        zero_string_expr_methods;
typedef esbmct::expr_methods<zero_length_string2t, string_ops,
        expr2tc, string_ops, &string_ops::string>
        zero_length_string_expr_methods;
typedef esbmct::expr_methods<isnan2t, isnan_data,
        expr2tc, isnan_data, &isnan_data::value>
        isnan_expr_methods;
typedef esbmct::expr_methods<overflow2t, overflow_ops,
        expr2tc, overflow_ops, &overflow_ops::operand>
        overflow_expr_methods;
typedef esbmct::expr_methods<overflow_cast2t, overflow_cast_data,
        expr2tc, overflow_ops, &overflow_ops::operand,
        unsigned int, overflow_cast_data, &overflow_cast_data::bits>
        overflow_cast_expr_methods;
typedef esbmct::expr_methods<overflow_neg2t, overflow_ops,
        expr2tc, overflow_ops, &overflow_ops::operand>
        overflow_neg_expr_methods;
typedef esbmct::expr_methods<unknown2t, expr2t>
        unknown_expr_methods;
typedef esbmct::expr_methods<invalid2t, expr2t>
        invalid_expr_methods;
typedef esbmct::expr_methods<null_object2t, expr2t>
        null_object_expr_methods;
typedef esbmct::expr_methods<dynamic_object2t, dynamic_object_data,
        expr2tc, dynamic_object_data, &dynamic_object_data::instance,
        bool, dynamic_object_data, &dynamic_object_data::invalid,
        bool, dynamic_object_data, &dynamic_object_data::unknown>
        dynamic_object_expr_methods;
typedef esbmct::expr_methods<dereference2t, dereference_data,
        expr2tc, dereference_data, &dereference_data::value>
        dereference_expr_methods;
typedef esbmct::expr_methods<valid_object2t, object_ops,
        expr2tc, object_ops, &object_ops::value>
        valid_object_expr_methods;
typedef esbmct::expr_methods<deallocated_obj2t, object_ops,
        expr2tc, object_ops, &object_ops::value>
        deallocated_obj_expr_methods;
typedef esbmct::expr_methods<dynamic_size2t, object_ops,
        expr2tc, object_ops, &object_ops::value>
        dynamic_size_expr_methods;
typedef esbmct::expr_methods<sideeffect2t, sideeffect_data,
        expr2tc, sideeffect_data, &sideeffect_data::operand,
        expr2tc, sideeffect_data, &sideeffect_data::size,
        type2tc, sideeffect_data, &sideeffect_data::alloctype,
        unsigned int, sideeffect_data, &sideeffect_data::kind,
        std::vector<expr2tc>, sideeffect_data, &sideeffect_data::arguments>
        sideeffect_expr_methods;
typedef esbmct::expr_methods<code_block2t, code_block_data,
        std::vector<expr2tc>, code_block_data, &code_block_data::operands>
        code_block_expr_methods;
typedef esbmct::expr_methods<code_assign2t, code_assign_data,
        expr2tc, code_assign_data, &code_assign_data::target,
        expr2tc, code_assign_data, &code_assign_data::source>
        code_assign_expr_methods;
typedef esbmct::expr_methods<code_init2t, code_assign_data,
        expr2tc, code_assign_data, &code_assign_data::target,
        expr2tc, code_assign_data, &code_assign_data::source>
        code_init_expr_methods;
typedef esbmct::expr_methods<code_decl2t, code_decl_data,
        irep_idt, code_decl_data, &code_decl_data::value>
        code_decl_expr_methods;
typedef esbmct::expr_methods<code_printf2t, code_printf_data,
        std::vector<expr2tc>, code_printf_data, &code_printf_data::operands>
        code_printf_expr_methods;
typedef esbmct::expr_methods<code_expression2t, code_expression_data,
        expr2tc, code_expression_data, &code_expression_data::operand>
        code_expression_expr_methods;
typedef esbmct::expr_methods<code_return2t, code_expression_data,
        expr2tc, code_expression_data, &code_expression_data::operand>
        code_return_expr_methods;
typedef esbmct::expr_methods<code_skip2t, expr2t>
        code_skip_expr_methods;
typedef esbmct::expr_methods<code_free2t, code_expression_data,
        expr2tc, code_expression_data, &code_expression_data::operand>
        code_free_expr_methods;
typedef esbmct::expr_methods<code_goto2t, code_goto_data,
        irep_idt, code_goto_data, &code_goto_data::target>
        code_goto_expr_methods;
typedef esbmct::expr_methods<object_descriptor2t, object_desc_data,
        expr2tc, object_desc_data, &object_desc_data::object,
        expr2tc, object_desc_data, &object_desc_data::offset>
        object_desc_expr_methods;
typedef esbmct::expr_methods<code_function_call2t, code_funccall_data,
        expr2tc, code_funccall_data, &code_funccall_data::ret,
        expr2tc, code_funccall_data, &code_funccall_data::function,
        std::vector<expr2tc>, code_funccall_data, &code_funccall_data::operands>
        code_function_call_expr_methods;
typedef esbmct::expr_methods<code_comma2t, code_comma_data,
        expr2tc, code_comma_data, &code_comma_data::side_1,
        expr2tc, code_comma_data, &code_comma_data::side_2>
        code_comma_expr_methods;
typedef esbmct::expr_methods<invalid_pointer2t, pointer_ops,
        expr2tc, pointer_ops, &pointer_ops::ptr_obj>
        invalid_pointer_expr_methods;
typedef esbmct::expr_methods<buffer_size2t, buffer_size_data,
        expr2tc, buffer_size_data, &buffer_size_data::value>
        buffer_size_expr_methods;
typedef esbmct::expr_methods<code_asm2t, code_asm_data,
        irep_idt, code_asm_data, &code_asm_data::value>
        code_asm_expr_methods;
typedef esbmct::expr_methods<code_cpp_del_array2t, code_expression_data,
        expr2tc, code_expression_data, &code_expression_data::operand>
        code_cpp_del_array_expr_methods;
typedef esbmct::expr_methods<code_cpp_delete2t, code_expression_data,
        expr2tc, code_expression_data, &code_expression_data::operand>
        code_cpp_delete_expr_methods;
typedef esbmct::expr_methods<code_cpp_catch2t, code_cpp_catch_data,
        expr2tc, code_cpp_catch_data, &code_cpp_catch_data::operand,
        std::vector<unsigned int>, code_cpp_catch_data,
        &code_cpp_catch_data::excp_list>
        code_cpp_catch_expr_methods;
typedef esbmct::expr_methods<code_cpp_throw2t, code_cpp_throw_data,
        expr2tc, code_cpp_throw_data, &code_cpp_throw_data::operand>
        code_cpp_throw_expr_methods;

/** Constant integer class. Records a constant integer of an arbitary
 *  precision */
class constant_int2t : public constant_int_expr_methods
{
public:
  constant_int2t(const type2tc &type, const BigInt &input)
    : constant_int_expr_methods(type, constant_int_id, input) { }
  constant_int2t(const constant_int2t &ref)
    : constant_int_expr_methods(ref) { }

  /** Accessor for fetching native int of this constant */
  unsigned long as_ulong(void) const;
  long as_long(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Constant fixedbv class. Records a floating point number in what I assume
 *  to be mantissa/exponent form, but which is described throughout CBMC code
 *  as fraction/integer parts. */
class constant_fixedbv2t : public constant_fixedbv_expr_methods
{
public:
  constant_fixedbv2t(const type2tc &type, const fixedbvt &value)
    : constant_fixedbv_expr_methods(type, constant_fixedbv_id, value) { }
  constant_fixedbv2t(const constant_fixedbv2t &ref)
    : constant_fixedbv_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class constant_bool2t : public constant_bool_expr_methods
{
public:
  constant_bool2t(bool value)
    : constant_bool_expr_methods(type_pool.get_bool(), constant_bool_id, value)
      { }
  constant_bool2t(const constant_bool2t &ref)
    : constant_bool_expr_methods(ref) { }

  bool is_true(void) const;
  bool is_false(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Constant class for string constants. */
class constant_string2t : public constant_string_expr_methods
{
public:
  constant_string2t(const type2tc &type, const irep_idt &stringref)
    : constant_string_expr_methods(type, constant_string_id, stringref) { }
  constant_string2t(const constant_string2t &ref)
    : constant_string_expr_methods(ref) { }

  /** Convert string to a constant length array */
  expr2tc to_array(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

class constant_struct2t : public constant_struct_expr_methods
{
public:
  constant_struct2t(const type2tc &type, const std::vector<expr2tc> &members)
    : constant_struct_expr_methods (type, constant_struct_id, members) { }
  constant_struct2t(const constant_struct2t &ref)
    : constant_struct_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class constant_union2t : public constant_union_expr_methods
{
public:
  constant_union2t(const type2tc &type, const std::vector<expr2tc> &members)
    : constant_union_expr_methods (type, constant_union_id, members) { }
  constant_union2t(const constant_union2t &ref)
    : constant_union_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class constant_array2t : public constant_array_expr_methods
{
public:
  constant_array2t(const type2tc &type, const std::vector<expr2tc> &members)
    : constant_array_expr_methods(type, constant_array_id, members) { }
  constant_array2t(const constant_array2t &ref)
    : constant_array_expr_methods(ref){}

  static std::string field_names[esbmct::num_type_fields];
};

class constant_array_of2t : public constant_array_of_expr_methods
{
public:
  constant_array_of2t(const type2tc &type, const expr2tc &init)
    : constant_array_of_expr_methods(type, constant_array_of_id, init) { }
  constant_array_of2t(const constant_array_of2t &ref)
    : constant_array_of_expr_methods(ref){}

  static std::string field_names[esbmct::num_type_fields];
};

class symbol2t : public symbol_expr_methods
{
public:
  symbol2t(const type2tc &type, const irep_idt &init)
    : symbol_expr_methods(type, symbol_id, init) { }
  symbol2t(const symbol2t &ref)
    : symbol_expr_methods(ref){}

  static std::string field_names[esbmct::num_type_fields];
};

class typecast2t : public typecast_expr_methods
{
public:
  typecast2t(const type2tc &type, const expr2tc &from)
    : typecast_expr_methods(type, typecast_id, from) { }
  typecast2t(const typecast2t &ref)
    : typecast_expr_methods(ref){}
  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

// Typecast, but explicitly either to or from a bit vector. This prevents any
// semantic conversion of floats to/from bits.
class to_bv_typecast2t : public to_bv_typecast_expr_methods
{
public:
  to_bv_typecast2t(const type2tc &type, const expr2tc &from)
    : to_bv_typecast_expr_methods(type, to_bv_typecast_id, from) { }
  to_bv_typecast2t(const to_bv_typecast2t &ref)
    : to_bv_typecast_expr_methods(ref){}

  static std::string field_names[esbmct::num_type_fields];
};

class from_bv_typecast2t : public from_bv_typecast_expr_methods
{
public:
  from_bv_typecast2t(const type2tc &type, const expr2tc &from)
    : from_bv_typecast_expr_methods(type, from_bv_typecast_id, from) { }
  from_bv_typecast2t(const from_bv_typecast2t &ref)
    : from_bv_typecast_expr_methods(ref){}

  static std::string field_names[esbmct::num_type_fields];
};

class if2t : public if_expr_methods
{
public:
  if2t(const type2tc &type, const expr2tc &cond, const expr2tc &trueval,
       const expr2tc &falseval)
    : if_expr_methods(type, if_id, cond, trueval, falseval) {}
  if2t(const if2t &ref)
    : if_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class equality2t : public equality_expr_methods
{
public:
  equality2t(const expr2tc &v1, const expr2tc &v2)
    : equality_expr_methods(type_pool.get_bool(), equality_id, v1, v2) {}
  equality2t(const equality2t &ref)
    : equality_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class notequal2t : public notequal_expr_methods
{
public:
  notequal2t(const expr2tc &v1, const expr2tc &v2)
    : notequal_expr_methods(type_pool.get_bool(), notequal_id, v1, v2) {}
  notequal2t(const notequal2t &ref)
    : notequal_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class lessthan2t : public lessthan_expr_methods
{
public:
  lessthan2t(const expr2tc &v1, const expr2tc &v2)
    : lessthan_expr_methods(type_pool.get_bool(), lessthan_id, v1, v2) {}
  lessthan2t(const lessthan2t &ref)
    : lessthan_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class greaterthan2t : public greaterthan_expr_methods
{
public:
  greaterthan2t(const expr2tc &v1, const expr2tc &v2)
    : greaterthan_expr_methods(type_pool.get_bool(), greaterthan_id, v1, v2) {}
  greaterthan2t(const greaterthan2t &ref)
    : greaterthan_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class lessthanequal2t : public lessthanequal_expr_methods
{
public:
  lessthanequal2t(const expr2tc &v1, const expr2tc &v2)
  : lessthanequal_expr_methods(type_pool.get_bool(), lessthanequal_id, v1, v2){}
  lessthanequal2t(const lessthanequal2t &ref)
  : lessthanequal_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class greaterthanequal2t : public greaterthanequal_expr_methods
{
public:
  greaterthanequal2t(const expr2tc &v1, const expr2tc &v2)
    : greaterthanequal_expr_methods(type_pool.get_bool(), greaterthanequal_id,
                                    v1, v2) {}
  greaterthanequal2t(const greaterthanequal2t &ref)
    : greaterthanequal_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class not2t : public not_expr_methods
{
public:
  not2t(const expr2tc &val)
  : not_expr_methods(type_pool.get_bool(), not_id, val) {}
  not2t(const not2t &ref)
  : not_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class and2t : public and_expr_methods
{
public:
  and2t(const expr2tc &s1, const expr2tc &s2)
  : and_expr_methods(type_pool.get_bool(), and_id, s1, s2) {}
  and2t(const and2t &ref)
  : and_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class or2t : public or_expr_methods
{
public:
  or2t(const expr2tc &s1, const expr2tc &s2)
  : or_expr_methods(type_pool.get_bool(), or_id, s1, s2) {}
  or2t(const or2t &ref)
  : or_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class xor2t : public xor_expr_methods
{
public:
  xor2t(const expr2tc &s1, const expr2tc &s2)
  : xor_expr_methods(type_pool.get_bool(), xor_id, s1, s2) {}
  xor2t(const xor2t &ref)
  : xor_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class implies2t : public implies_expr_methods
{
public:
  implies2t(const expr2tc &s1, const expr2tc &s2)
  : implies_expr_methods(type_pool.get_bool(), implies_id, s1, s2) {}
  implies2t(const implies2t &ref)
  : implies_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class bitand2t : public bitand_expr_methods
{
public:
  bitand2t(const type2tc &t, const expr2tc &s1, const expr2tc &s2)
  : bitand_expr_methods(t, bitand_id, s1, s2) {}
  bitand2t(const bitand2t &ref)
  : bitand_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class bitor2t : public bitor_expr_methods
{
public:
  bitor2t(const type2tc &t, const expr2tc &s1, const expr2tc &s2)
  : bitor_expr_methods(t, bitor_id, s1, s2) {}
  bitor2t(const bitor2t &ref)
  : bitor_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class bitxor2t : public bitxor_expr_methods
{
public:
  bitxor2t(const type2tc &t, const expr2tc &s1, const expr2tc &s2)
  : bitxor_expr_methods(t, bitxor_id, s1, s2) {}
  bitxor2t(const bitxor2t &ref)
  : bitxor_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class bitnand2t : public bitnand_expr_methods
{
public:
  bitnand2t(const type2tc &t, const expr2tc &s1, const expr2tc &s2)
  : bitnand_expr_methods(t, bitnand_id, s1, s2) {}
  bitnand2t(const bitnand2t &ref)
  : bitnand_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class bitnor2t : public bitnor_expr_methods
{
public:
  bitnor2t(const type2tc &t, const expr2tc &s1, const expr2tc &s2)
  : bitnor_expr_methods(t, bitnor_id, s1, s2) {}
  bitnor2t(const bitnor2t &ref)
  : bitnor_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class bitnxor2t : public bitnxor_expr_methods
{
public:
  bitnxor2t(const type2tc &t, const expr2tc &s1, const expr2tc &s2)
  : bitnxor_expr_methods(t, bitnxor_id, s1, s2) {}
  bitnxor2t(const bitnxor2t &ref)
  : bitnxor_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class bitnot2t : public bitnot_expr_methods
{
public:
  bitnot2t(const type2tc &type, const expr2tc &v)
    : bitnot_expr_methods(type, bitnot_id, v) {}
  bitnot2t(const bitnot2t &ref)
    : bitnot_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class lshr2t : public lshr_expr_methods
{
public:
  lshr2t(const type2tc &t, const expr2tc &s1, const expr2tc &s2)
  : lshr_expr_methods(t, lshr_id, s1, s2) {}
  lshr2t(const lshr2t &ref)
  : lshr_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class neg2t : public neg_expr_methods
{
public:
  neg2t(const type2tc &type, const expr2tc &val)
    : neg_expr_methods(type, neg_id, val) {}
  neg2t(const neg2t &ref)
    : neg_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class abs2t : public abs_expr_methods
{
public:
  abs2t(const type2tc &type, const expr2tc &val)
    : abs_expr_methods(type, abs_id, val) {}
  abs2t(const abs2t &ref)
    : abs_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class add2t : public add_expr_methods
{
public:
  add2t(const type2tc &type, const expr2tc &v1, const expr2tc &v2)
    : add_expr_methods(type, add_id, v1, v2) {}
  add2t(const add2t &ref)
    : add_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class sub2t : public sub_expr_methods
{
public:
  sub2t(const type2tc &type, const expr2tc &v1, const expr2tc &v2)
    : sub_expr_methods(type, sub_id, v1, v2) {}
  sub2t(const sub2t &ref)
    : sub_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class mul2t : public mul_expr_methods
{
public:
  mul2t(const type2tc &type, const expr2tc &v1, const expr2tc &v2)
    : mul_expr_methods(type, mul_id, v1, v2) {}
  mul2t(const mul2t &ref)
    : mul_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class div2t : public div_expr_methods
{
public:
  div2t(const type2tc &type, const expr2tc &v1, const expr2tc &v2)
    : div_expr_methods(type, div_id, v1, v2) {}
  div2t(const div2t &ref)
    : div_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class modulus2t : public modulus_expr_methods
{
public:
  modulus2t(const type2tc &type, const expr2tc &v1, const expr2tc &v2)
    : modulus_expr_methods(type, modulus_id, v1, v2) {}
  modulus2t(const modulus2t &ref)
    : modulus_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class shl2t : public shl_expr_methods
{
public:
  shl2t(const type2tc &type, const expr2tc &v1, const expr2tc &v2)
    : shl_expr_methods(type, shl_id, v1, v2) {}
  shl2t(const shl2t &ref)
    : shl_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class ashr2t : public ashr_expr_methods
{
public:
  ashr2t(const type2tc &type, const expr2tc &v1, const expr2tc &v2)
    : ashr_expr_methods(type, ashr_id, v1, v2) {}
  ashr2t(const ashr2t &ref)
    : ashr_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class same_object2t : public same_object_expr_methods
{
public:
  same_object2t(const expr2tc &v1, const expr2tc &v2)
    : same_object_expr_methods(type_pool.get_bool(), same_object_id, v1, v2) {}
  same_object2t(const same_object2t &ref)
    : same_object_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class pointer_offset2t : public pointer_offset_expr_methods
{
public:
  pointer_offset2t(const type2tc &type, const expr2tc &ptrobj)
    : pointer_offset_expr_methods(type, pointer_offset_id, ptrobj) {}
  pointer_offset2t(const pointer_offset2t &ref)
    : pointer_offset_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class pointer_object2t : public pointer_object_expr_methods
{
public:
  pointer_object2t(const type2tc &type, const expr2tc &ptrobj)
    : pointer_object_expr_methods(type, pointer_object_id, ptrobj) {}
  pointer_object2t(const pointer_object2t &ref)
    : pointer_object_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class address_of2t : public address_of_expr_methods
{
public:
  address_of2t(const type2tc &subtype, const expr2tc &ptrobj)
    : address_of_expr_methods(type2tc(new pointer_type2t(subtype)),
                              address_of_id, ptrobj) {}
  address_of2t(const address_of2t &ref)
    : address_of_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class byte_extract2t : public byte_extract_expr_methods
{
public:
  byte_extract2t(const type2tc &type, bool is_big_endian, const expr2tc &source,
                 const expr2tc &offset)
    : byte_extract_expr_methods(type, byte_extract_id, is_big_endian,
                               source, offset) {}
  byte_extract2t(const byte_extract2t &ref)
    : byte_extract_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class byte_update2t : public byte_update_expr_methods
{
public:
  byte_update2t(const type2tc &type, bool is_big_endian, const expr2tc &source,
                 const expr2tc &offset, const expr2tc &updateval)
    : byte_update_expr_methods(type, byte_update_id, is_big_endian,
                               source, offset, updateval) {}
  byte_update2t(const byte_update2t &ref)
    : byte_update_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class with2t : public with_expr_methods
{
public:
  with2t(const type2tc &type, const expr2tc &source, const expr2tc &field,
         const expr2tc &value)
    : with_expr_methods(type, with_id, source, field, value) {}
  with2t(const with2t &ref)
    : with_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class member2t : public member_expr_methods
{
public:
  member2t(const type2tc &type, const expr2tc &source, const irep_idt &memb)
    : member_expr_methods(type, member_id, source, memb) {}
  member2t(const member2t &ref)
    : member_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class index2t : public index_expr_methods
{
public:
  index2t(const type2tc &type, const expr2tc &source, const expr2tc &index)
    : index_expr_methods(type, index_id, source, index) {}
  index2t(const index2t &ref)
    : index_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class zero_string2t : public zero_string_expr_methods
{
public:
  zero_string2t(const expr2tc &string)
    : zero_string_expr_methods(type_pool.get_bool(), zero_string_id, string) {}
  zero_string2t(const zero_string2t &ref)
    : zero_string_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class zero_length_string2t : public zero_length_string_expr_methods
{
public:
  zero_length_string2t(const expr2tc &string)
    : zero_length_string_expr_methods(type_pool.get_bool(),
                                      zero_length_string_id, string) {}
  zero_length_string2t(const zero_length_string2t &ref)
    : zero_length_string_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class isnan2t : public isnan_expr_methods
{
public:
  isnan2t(const expr2tc &value)
    : isnan_expr_methods(type_pool.get_bool(), isnan_id, value) {}
  isnan2t(const isnan2t &ref)
    : isnan_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

/** Check whether operand overflows. Operand must be either add, subtract,
 *  or multiply. XXXjmorse - in the future we should ensure the type of the
 *  operand is the expected type result of the operation. That way we can tell
 *  whether to do a signed or unsigned over/underflow test. */

class overflow2t : public overflow_expr_methods
{
public:
  overflow2t(const expr2tc &operand)
    : overflow_expr_methods(type_pool.get_bool(), overflow_id, operand) {}
  overflow2t(const overflow2t &ref)
    : overflow_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class overflow_cast2t : public overflow_cast_expr_methods
{
public:
  overflow_cast2t(const expr2tc &operand, unsigned int bits)
    : overflow_cast_expr_methods(type_pool.get_bool(), overflow_cast_id,
                                 operand, bits) {}
  overflow_cast2t(const overflow_cast2t &ref)
    : overflow_cast_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class overflow_neg2t : public overflow_neg_expr_methods
{
public:
  overflow_neg2t(const expr2tc &operand)
    : overflow_neg_expr_methods(type_pool.get_bool(), overflow_neg_id,
                                operand) {}
  overflow_neg2t(const overflow_neg2t &ref)
    : overflow_neg_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class unknown2t : public unknown_expr_methods
{
public:
  unknown2t(const type2tc &type)
    : unknown_expr_methods(type, unknown_id) {}
  unknown2t(const unknown2t &ref)
    : unknown_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class invalid2t : public invalid_expr_methods
{
public:
  invalid2t(const type2tc &type)
    : invalid_expr_methods(type, invalid_id) {}
  invalid2t(const invalid2t &ref)
    : invalid_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class null_object2t : public null_object_expr_methods
{
public:
  null_object2t(const type2tc &type)
    : null_object_expr_methods(type, null_object_id) {}
  null_object2t(const null_object2t &ref)
    : null_object_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class dynamic_object2t : public dynamic_object_expr_methods
{
public:
  dynamic_object2t(const type2tc &type, const expr2tc inst,
                   bool inv, bool uknown)
    : dynamic_object_expr_methods(type, dynamic_object_id, inst, inv, uknown) {}
  dynamic_object2t(const dynamic_object2t &ref)
    : dynamic_object_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class dereference2t : public dereference_expr_methods
{
public:
  dereference2t(const type2tc &type, const expr2tc &operand)
    : dereference_expr_methods(type, dereference_id, operand) {}
  dereference2t(const dereference2t &ref)
    : dereference_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class valid_object2t : public valid_object_expr_methods
{
public:
  valid_object2t(const expr2tc &operand)
    : valid_object_expr_methods(type_pool.get_bool(), valid_object_id, operand)
      {}
  valid_object2t(const valid_object2t &ref)
    : valid_object_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class deallocated_obj2t : public deallocated_obj_expr_methods
{
public:
  deallocated_obj2t(const expr2tc &operand)
    : deallocated_obj_expr_methods(type_pool.get_bool(), deallocated_obj_id,
                                   operand) {}
  deallocated_obj2t(const deallocated_obj2t &ref)
    : deallocated_obj_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class dynamic_size2t : public dynamic_size_expr_methods
{
public:
  dynamic_size2t(const expr2tc &operand)
    : dynamic_size_expr_methods(type_pool.get_bool(), dynamic_size_id, operand)
      {}
  dynamic_size2t(const dynamic_size2t &ref)
    : dynamic_size_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class sideeffect2t : public sideeffect_expr_methods
{
public:
  enum allockind {
    malloc,
    cpp_new,
    cpp_new_arr,
    nondet,
    function_call
  };

  sideeffect2t(const type2tc &t, const expr2tc &oper, const expr2tc &sz,
               const type2tc &alloct, allockind k, std::vector<expr2tc> &a)
    : sideeffect_expr_methods(t, sideeffect_id, oper, sz, alloct, k, a) {}
  sideeffect2t(const sideeffect2t &ref)
    : sideeffect_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class code_block2t : public code_block_expr_methods
{
public:
  code_block2t(const std::vector<expr2tc> &operands)
    : code_block_expr_methods(type_pool.get_empty(), code_block_id, operands) {}
  code_block2t(const code_block2t &ref)
    : code_block_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class code_assign2t : public code_assign_expr_methods
{
public:
  code_assign2t(const expr2tc &target, const expr2tc &source)
    : code_assign_expr_methods(type_pool.get_empty(), code_assign_id,
                               target, source) {}
  code_assign2t(const code_assign2t &ref)
    : code_assign_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

// NB: code_init2t is a specialization of code_assign2t
class code_init2t : public code_init_expr_methods
{
public:
  code_init2t(const expr2tc &target, const expr2tc &source)
    : code_init_expr_methods(type_pool.get_empty(), code_init_id,
                               target, source) {}
  code_init2t(const code_init2t &ref)
    : code_init_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class code_decl2t : public code_decl_expr_methods
{
public:
  code_decl2t(const type2tc &t, const irep_idt &name)
    : code_decl_expr_methods(t, code_decl_id, name){}
  code_decl2t(const code_decl2t &ref)
    : code_decl_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class code_printf2t : public code_printf_expr_methods
{
public:
  code_printf2t(const std::vector<expr2tc> &opers)
    : code_printf_expr_methods(type_pool.get_empty(), code_printf_id, opers) {}
  code_printf2t(const code_printf2t &ref)
    : code_printf_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class code_expression2t : public code_expression_expr_methods
{
public:
  code_expression2t(const expr2tc &oper)
    : code_expression_expr_methods(type_pool.get_empty(), code_expression_id,
                                   oper) {}
  code_expression2t(const code_expression2t &ref)
    : code_expression_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class code_return2t : public code_return_expr_methods
{
public:
  code_return2t(const expr2tc &oper)
    : code_return_expr_methods(type_pool.get_empty(), code_return_id, oper) {}
  code_return2t(const code_return2t &ref)
    : code_return_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class code_skip2t : public code_skip_expr_methods
{
public:
  code_skip2t()
    : code_skip_expr_methods(type_pool.get_empty(), code_skip_id) {}
  code_skip2t(const code_skip2t &ref)
    : code_skip_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class code_free2t : public code_free_expr_methods
{
public:
  code_free2t(const expr2tc &oper)
    : code_free_expr_methods(type_pool.get_empty(), code_free_id, oper) {}
  code_free2t(const code_free2t &ref)
    : code_free_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class code_goto2t : public code_goto_expr_methods
{
public:
  code_goto2t(const irep_idt &targ)
    : code_goto_expr_methods(type_pool.get_empty(), code_goto_id, targ) {}
  code_goto2t(const code_goto2t &ref)
    : code_goto_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class object_descriptor2t : public object_desc_expr_methods
{
public:
  object_descriptor2t(const type2tc &t, const expr2tc &root,const expr2tc &offs)
    : object_desc_expr_methods(t, object_descriptor_id, root, offs) {}
  object_descriptor2t(const object_descriptor2t &ref)
    : object_desc_expr_methods(ref) {}

  const expr2tc &get_root_object(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

class code_function_call2t : public code_function_call_expr_methods
{
public:
  code_function_call2t(const expr2tc &r, const expr2tc &func,
                       const std::vector<expr2tc> args)
    : code_function_call_expr_methods(type_pool.get_empty(),
                                      code_function_call_id, r, func, args) {}
  code_function_call2t(const code_function_call2t &ref)
    : code_function_call_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class code_comma2t : public code_comma_expr_methods
{
public:
  code_comma2t(const type2tc &t, const expr2tc &s1, const expr2tc &s2)
    : code_comma_expr_methods(t, code_comma_id, s1, s2) {}
  code_comma2t(const code_comma2t &ref)
    : code_comma_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class invalid_pointer2t : public invalid_pointer_expr_methods
{
public:
  invalid_pointer2t(const expr2tc &obj)
    : invalid_pointer_expr_methods(type_pool.get_bool(), invalid_pointer_id,
                                   obj) {}
  invalid_pointer2t(const invalid_pointer2t &ref)
    : invalid_pointer_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class buffer_size2t : public buffer_size_expr_methods
{
public:
  buffer_size2t(const type2tc &t, const expr2tc &obj)
    : buffer_size_expr_methods(t, buffer_size_id, obj) {}
  buffer_size2t(const buffer_size2t &ref)
    : buffer_size_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class code_asm2t : public code_asm_expr_methods
{
public:
  code_asm2t(const type2tc &type, const irep_idt &stringref)
    : code_asm_expr_methods(type, code_asm_id, stringref) { }
  code_asm2t(const code_asm2t &ref)
    : code_asm_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class code_cpp_del_array2t : public code_cpp_del_array_expr_methods
{
public:
  code_cpp_del_array2t(const expr2tc &v)
    : code_cpp_del_array_expr_methods(type_pool.get_empty(),
                                      code_cpp_del_array_id, v) { }
  code_cpp_del_array2t(const code_cpp_del_array2t &ref)
    : code_cpp_del_array_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class code_cpp_delete2t : public code_cpp_delete_expr_methods
{
public:
  code_cpp_delete2t(const expr2tc &v)
    : code_cpp_delete_expr_methods(type_pool.get_empty(),
                                   code_cpp_delete_id, v) { }
  code_cpp_delete2t(const code_cpp_delete2t &ref)
    : code_cpp_delete_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class code_cpp_catch2t : public code_cpp_catch_expr_methods
{
public:
  code_cpp_catch2t(const expr2tc &o, const std::vector<unsigned int> &el)
    : code_cpp_catch_expr_methods(type_pool.get_empty(),
                                   code_cpp_catch_id, o, el) { }
  code_cpp_catch2t(const code_cpp_catch2t &ref)
    : code_cpp_catch_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class code_cpp_throw2t : public code_cpp_throw_expr_methods
{
public:
  code_cpp_throw2t(const expr2tc &o)
    : code_cpp_throw_expr_methods(type_pool.get_empty(), code_cpp_throw_id, o){}
  code_cpp_throw2t(const code_cpp_throw2t &ref)
    : code_cpp_throw_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

inline bool operator==(boost::shared_ptr<type2t> const & a, boost::shared_ptr<type2t> const & b)
{
  return (*a.get() == *b.get());
}

inline bool operator!=(boost::shared_ptr<type2t> const & a, boost::shared_ptr<type2t> const & b)
{
  return !(a == b);
}

inline bool operator<(boost::shared_ptr<type2t> const & a, boost::shared_ptr<type2t> const & b)
{
  return (*a.get() < *b.get());
}

inline bool operator>(boost::shared_ptr<type2t> const & a, boost::shared_ptr<type2t> const & b)
{
  return (*b.get() < *a.get());
}

inline bool operator==(const expr2tc& a, const expr2tc& b)
{
  return (*a.get() == *b.get());
}

inline bool operator!=(const expr2tc& a, const expr2tc& b)
{
  return (*a.get() != *b.get());
}

inline bool operator<(const expr2tc& a, const expr2tc& b)
{
  return (*a.get() < *b.get());
}

inline bool operator>(const expr2tc& a, const expr2tc& b)
{
  return (*b.get() < *a.get());
}

inline std::ostream& operator<<(std::ostream &out, const expr2tc& a)
{
  out << a->pretty(0);
  return out;
}

struct irep2_hash
{
  size_t operator()(const expr2tc &ref) const { return ref->crc(); }
};

struct type2_hash
{
  size_t operator()(const type2tc &ref) const { return ref->crc(); }
};

// Same deal as for "type_macros".
#ifdef NDEBUG
#define dynamic_cast static_cast
#endif
#define expr_macros(name) \
  inline bool is_##name##2t(const expr2tc &t) \
    { return t->expr_id == expr2t::name##_id; } \
  inline bool is_##name##2t(const expr2t &r) \
    { return r.expr_id == expr2t::name##_id; } \
  inline const name##2t & to_##name##2t(const expr2tc &t) \
    { return dynamic_cast<const name##2t &> (*t); } \
  inline name##2t & to_##name##2t(expr2tc &t) \
    { return dynamic_cast<name##2t &> (*t.get()); }

expr_macros(constant_int);
expr_macros(constant_fixedbv);
expr_macros(constant_bool);
expr_macros(constant_string);
expr_macros(constant_struct);
expr_macros(constant_union);
expr_macros(constant_array);
expr_macros(constant_array_of);
expr_macros(symbol);
expr_macros(typecast);
expr_macros(if);
expr_macros(equality);
expr_macros(notequal);
expr_macros(lessthan);
expr_macros(greaterthan);
expr_macros(lessthanequal);
expr_macros(greaterthanequal);
expr_macros(not);
expr_macros(and);
expr_macros(or);
expr_macros(xor);
expr_macros(implies);
expr_macros(bitand);
expr_macros(bitor);
expr_macros(bitxor);
expr_macros(bitnand);
expr_macros(bitnor);
expr_macros(bitnxor);
expr_macros(bitnot);
expr_macros(lshr);
expr_macros(neg);
expr_macros(abs);
expr_macros(add);
expr_macros(sub);
expr_macros(mul);
expr_macros(div);
expr_macros(modulus);
expr_macros(shl);
expr_macros(ashr);
expr_macros(same_object);
expr_macros(pointer_offset);
expr_macros(pointer_object);
expr_macros(address_of);
expr_macros(byte_extract);
expr_macros(byte_update);
expr_macros(with);
expr_macros(member);
expr_macros(index);
expr_macros(zero_string);
expr_macros(zero_length_string);
expr_macros(isnan);
expr_macros(overflow);
expr_macros(overflow_cast);
expr_macros(overflow_neg);
expr_macros(unknown);
expr_macros(invalid);
expr_macros(null_object);
expr_macros(dynamic_object);
expr_macros(dereference);
expr_macros(valid_object);
expr_macros(deallocated_obj);
expr_macros(dynamic_size);
expr_macros(sideeffect);
expr_macros(code_block);
expr_macros(code_assign);
expr_macros(code_init);
expr_macros(code_decl);
expr_macros(code_printf);
expr_macros(code_expression);
expr_macros(code_return);
expr_macros(code_skip);
expr_macros(code_free);
expr_macros(code_goto);
expr_macros(object_descriptor);
expr_macros(code_function_call);
expr_macros(code_comma);
expr_macros(invalid_pointer);
expr_macros(buffer_size);
expr_macros(code_asm);
expr_macros(code_cpp_del_array);
expr_macros(code_cpp_delete);
expr_macros(code_cpp_catch);
expr_macros(code_cpp_throw);
#undef expr_macros
#ifdef dynamic_cast
#undef dynamic_cast
#endif

inline bool is_constant_expr(const expr2tc &t)
{
  return t->expr_id == expr2t::constant_int_id ||
         t->expr_id == expr2t::constant_fixedbv_id ||
         t->expr_id == expr2t::constant_bool_id ||
         t->expr_id == expr2t::constant_string_id ||
         t->expr_id == expr2t::constant_struct_id ||
         t->expr_id == expr2t::constant_union_id ||
         t->expr_id == expr2t::constant_array_id ||
         t->expr_id == expr2t::constant_array_of_id;
}

inline bool is_structure_type(const type2tc &t)
{
  return t->type_id == type2t::struct_id || t->type_id == type2t::union_id;
}

inline bool is_nil_expr(const expr2tc &exp)
{
  if (exp.get() == NULL)
    return true;
  return false;
}

inline bool is_nil_type(const type2tc &t)
{
  if (t.get() == NULL)
    return true;
  return false;
}

typedef irep_container<constant_int2t, expr2t::constant_int_id> constant_int2tc;
typedef irep_container<constant_fixedbv2t, expr2t::constant_fixedbv_id>
                       constant_fixedbv2tc;
typedef irep_container<constant_bool2t, expr2t::constant_bool_id>
                       constant_bool2tc;
typedef irep_container<constant_string2t, expr2t::constant_string_id>
                       constant_string2tc;
typedef irep_container<constant_struct2t, expr2t::constant_struct_id>
                       constant_struct2tc;
typedef irep_container<constant_union2t, expr2t::constant_union_id>
                       constant_union2tc;
typedef irep_container<constant_array2t, expr2t::constant_array_id>
                       constant_array2tc;
typedef irep_container<constant_array_of2t, expr2t::constant_array_of_id>
                       constant_array_of2tc;
typedef irep_container<symbol2t, expr2t::symbol_id> symbol2tc;
typedef irep_container<typecast2t, expr2t::typecast_id> typecast2tc;
typedef irep_container<if2t, expr2t::if_id> if2tc;
typedef irep_container<equality2t, expr2t::equality_id> equality2tc;
typedef irep_container<notequal2t, expr2t::notequal_id> notequal2tc;
typedef irep_container<lessthan2t, expr2t::lessthan_id> lessthan2tc;
typedef irep_container<greaterthan2t, expr2t::greaterthan_id> greaterthan2tc;
typedef irep_container<lessthanequal2t, expr2t::lessthanequal_id>
                       lessthanequal2tc;
typedef irep_container<greaterthanequal2t, expr2t::greaterthanequal_id>
                       greaterthanequal2tc;
typedef irep_container<not2t, expr2t::not_id> not2tc;
typedef irep_container<and2t, expr2t::and_id> and2tc;
typedef irep_container<or2t, expr2t::or_id> or2tc;
typedef irep_container<xor2t, expr2t::xor_id> xor2tc;
typedef irep_container<implies2t, expr2t::implies_id> implies2tc;
typedef irep_container<bitand2t, expr2t::bitand_id> bitand2tc;
typedef irep_container<bitor2t, expr2t::bitor_id> bitor2tc;
typedef irep_container<bitxor2t, expr2t::bitxor_id> bitxor2tc;
typedef irep_container<bitnand2t, expr2t::bitnand_id> bitnand2tc;
typedef irep_container<bitnor2t, expr2t::bitnor_id> bitnor2tc;
typedef irep_container<bitnxor2t, expr2t::bitnxor_id> bitnxor2tc;
typedef irep_container<bitnot2t, expr2t::bitnot_id> bitnot2tc;
typedef irep_container<lshr2t, expr2t::lshr_id> lshr2tc;
typedef irep_container<neg2t, expr2t::neg_id> neg2tc;
typedef irep_container<abs2t, expr2t::abs_id> abs2tc;
typedef irep_container<add2t, expr2t::add_id> add2tc;
typedef irep_container<sub2t, expr2t::sub_id> sub2tc;
typedef irep_container<mul2t, expr2t::mul_id> mul2tc;
typedef irep_container<div2t, expr2t::div_id> div2tc;
typedef irep_container<modulus2t, expr2t::modulus_id> modulus2tc;
typedef irep_container<shl2t, expr2t::shl_id> shl2tc;
typedef irep_container<ashr2t, expr2t::ashr_id> ashr2tc;
typedef irep_container<same_object2t, expr2t::same_object_id> same_object2tc;
typedef irep_container<pointer_offset2t, expr2t::pointer_offset_id>
                       pointer_offset2tc;
typedef irep_container<pointer_object2t, expr2t::pointer_object_id>
                       pointer_object2tc;
typedef irep_container<address_of2t, expr2t::address_of_id> address_of2tc;
typedef irep_container<byte_extract2t, expr2t::byte_extract_id> byte_extract2tc;
typedef irep_container<byte_update2t, expr2t::byte_update_id> byte_update2tc;
typedef irep_container<with2t, expr2t::with_id> with2tc;
typedef irep_container<member2t, expr2t::member_id> member2tc;
typedef irep_container<index2t, expr2t::index_id> index2tc;
typedef irep_container<zero_string2t, expr2t::zero_string_id> zero_string2tc;
typedef irep_container<zero_length_string2t, expr2t::zero_length_string_id>
                       zero_length_string2tc;
typedef irep_container<isnan2t, expr2t::isnan_id> isnan2tc;
typedef irep_container<overflow2t, expr2t::overflow_id> overflow2tc;
typedef irep_container<overflow_cast2t, expr2t::overflow_cast_id>
                       overflow_cast2tc;
typedef irep_container<overflow_neg2t, expr2t::overflow_neg_id>overflow_neg2tc;
typedef irep_container<unknown2t, expr2t::unknown_id> unknown2tc;
typedef irep_container<invalid2t, expr2t::invalid_id> invalid2tc;
typedef irep_container<dynamic_object2t, expr2t::dynamic_object_id>
                       dynamic_object2tc;
typedef irep_container<dereference2t, expr2t::dereference_id> dereference2tc;
typedef irep_container<valid_object2t, expr2t::valid_object_id> vaild_object2tc;
typedef irep_container<deallocated_obj2t, expr2t::deallocated_obj_id>
                       deallocated_obj2tc;
typedef irep_container<dynamic_size2t, expr2t::dynamic_size_id> dynamic_size2tc;
typedef irep_container<sideeffect2t, expr2t::sideeffect_id> sideeffect2tc;
typedef irep_container<code_block2t, expr2t::code_block_id> code_block2tc;
typedef irep_container<code_assign2t, expr2t::code_assign_id> code_assign2tc;
typedef irep_container<code_init2t, expr2t::code_init_id> code_init2tc;
typedef irep_container<code_decl2t, expr2t::code_decl_id> code_decl2tc;
typedef irep_container<code_printf2t, expr2t::code_printf_id> code_printf2tc;
typedef irep_container<code_expression2t, expr2t::code_expression_id>
                       code_expression2tc;
typedef irep_container<code_return2t, expr2t::code_return_id> code_return2tc;
typedef irep_container<code_skip2t, expr2t::code_skip_id> code_skip2tc;
typedef irep_container<code_free2t, expr2t::code_free_id> code_free2tc;
typedef irep_container<code_goto2t, expr2t::code_goto_id> code_goto2tc;
typedef irep_container<object_descriptor2t, expr2t::object_descriptor_id>
                       object_descriptor2tc;
typedef irep_container<code_function_call2t, expr2t::code_function_call_id>
                       code_function_call2tc;
typedef irep_container<code_comma2t, expr2t::code_comma_id> code_comma2tc;
typedef irep_container<invalid_pointer2t, expr2t::invalid_pointer_id>
                       code_invalid_pointer2tc;
typedef irep_container<buffer_size2t, expr2t::buffer_size_id> buffer_size2tc;
typedef irep_container<code_asm2t, expr2t::code_asm_id> code_asm2tc;
typedef irep_container<code_cpp_del_array2t, expr2t::code_cpp_del_array_id>
                       code_cpp_del_array2tc;
typedef irep_container<code_cpp_delete2t, expr2t::code_cpp_delete_id>
                       code_cpp_delete2tc;
typedef irep_container<code_cpp_catch2t, expr2t::code_cpp_catch_id>
                       code_cpp_catch2tc;
typedef irep_container<code_cpp_throw2t, expr2t::code_cpp_throw_id>
                       code_cpp_throw2tc;

// XXXjmorse - to be moved into struct union superclass when it exists.
inline unsigned int
get_component_number(const type2tc &type, const irep_idt &name)
{
  const std::vector<irep_idt> &member_names = (is_struct_type(type))
    ? to_struct_type(type).member_names : to_union_type(type).member_names;

  unsigned int i = 0;
  forall_names(it, member_names) {
    if (*it == name)
      return i;
    i++;
  }

  assert(0);
}

#endif /* _UTIL_IREP2_H_ */
