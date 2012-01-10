/*******************************************************************\

Module: C++ Language Type Checking

Author: Daniel Kroening, kroening@cs.cmu.edu

\*******************************************************************/

#include <location.h>
#include <std_types.h>
#include <ansi-c/c_types.h>

#include "irep2name.h"
#include "cpp_declarator_converter.h"
#include "cpp_typecheck.h"

/*******************************************************************\

Function: cpp_declarator_convertert::cpp_declarator_convertert

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

cpp_declarator_convertert::cpp_declarator_convertert(
  class cpp_typecheckt &_cpp_typecheck):
  is_typedef(false),
  is_template(false),
  is_template_argument(false),
  is_friend(false),
  mode(_cpp_typecheck.current_mode),
  cpp_typecheck(_cpp_typecheck)
{
}

/*******************************************************************\

Function: cpp_declarator_convertert::convert

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

symbolt &cpp_declarator_convertert::convert(
  const typet &type,
  const cpp_storage_spect &storage_spec,
  const cpp_member_spect &member_spec,
  cpp_declaratort &declarator)
{
  if(type.id()=="cpp-cast-operator")
  {
    typet type;
    type.swap(declarator.name().get_sub().back());
    declarator.type().subtype()=type;
    std::string tmp;
    cpp_typecheck.typecheck_type(type);
    irep2name(type, tmp);
    tmp="("+tmp+")";
    irept name("name");
    name.set("identifier", tmp);
    declarator.name().get_sub().back().swap(name);
  }

  assert(declarator.id()=="cpp-declarator");
  final_type=declarator.merge_type(type);

  irept template_args;

  {
    cpp_save_scopet save_scope(cpp_typecheck.cpp_scopes);

    cpp_typecheck_resolvet cpp_typecheck_resolve(cpp_typecheck);
    cpp_typecheck_resolve.resolve_scope(
      declarator.name(), base_name, template_args);

    scope=&cpp_typecheck.cpp_scopes.current_scope();
  }

  cpp_typecheck.typecheck_type(final_type);
  is_code=is_code_type(final_type);

  if(scope->is_global_scope())
    cpp_typecheck.check_array_types(final_type);

  get_final_identifier();

  // first see if it is a member
  if(scope->id_class==cpp_idt::CLASS && !is_friend)
  {
    // it's a member! it must be declared already

    typet &method_qualifier=
      static_cast<typet &>(declarator.add("method_qualifier"));

    // adjust template type
    if(final_type.id()=="template")
    {
      assert(0);
      typet tmp;
      tmp.swap(final_type.subtype());
      final_type.swap(tmp);
    }

    // try static first
    symbolst::iterator c_it=
      cpp_typecheck.context.symbols.find(final_identifier);

    if(c_it==cpp_typecheck.context.symbols.end())
    {

      // adjust type if it's a non-static member function
      if(final_type.id()=="code")
        cpp_typecheck.adjust_method_type(
          scope->identifier, final_type, method_qualifier);

      get_final_identifier();

      // try again
      c_it = cpp_typecheck.context.symbols.find(final_identifier);

      if(c_it==cpp_typecheck.context.symbols.end())
      {
        cpp_typecheck.err_location(declarator.name());
        cpp_typecheck.str << "member `" << base_name
                          << "' not found in scope `"
                          << scope->identifier << "'";
        throw 0;
      }
    }

    assert(c_it!=cpp_typecheck.context.symbols.end());

    symbolt &symbol=c_it->second;

    combine_types(declarator.name().location(), final_type, symbol);
    enforce_rules(symbol);

    // If it is a constructor, we take care of the
    // object initialization
    if(final_type.get("return_type")=="constructor")
    {
      const cpp_namet &name = (const cpp_namet&) declarator.find("name");
      exprt symbol_expr;
      cpp_typecheck.resolve(
        name,
        cpp_typecheck_resolvet::TYPE,
        cpp_typecheck_fargst(),
        symbol_expr);

      if(symbol_expr.id()!="type" ||
         symbol_expr.type().id()!="symbol")
      {
        cpp_typecheck.err_location(name.location());
        cpp_typecheck.str << "error: expected type" << std::endl;
        throw 0;
      }

      irep_idt identifier=symbol_expr.type().get("identifier");
      const symbolt &symb=cpp_typecheck.lookup(identifier);
      const typet& type = symb.type;
      assert(type.id()=="struct");

      if(declarator.find("member_initializers").is_nil())
            declarator.set("member_initializers","member_initializers");

      cpp_typecheck.check_member_initializers(
        type.find("bases"),
        type.find("components"),
        declarator.add("member_initializers")
        );

      cpp_typecheck.full_member_initialization(
        to_struct_type(type),
        declarator.add("member_initializers")
       );
    }

    if(!storage_spec.is_extern())
      symbol.is_extern = false;

    // initializer?
    handle_initializer(symbol, declarator);

    return symbol;
  }
  else
  {
    // no, it's no way a method

    // already there?
    symbolst::iterator c_it=
      cpp_typecheck.context.symbols.find(final_identifier);

    if(c_it==cpp_typecheck.context.symbols.end())
      return convert_new_symbol(storage_spec, member_spec, declarator);

    symbolt &symbol=c_it->second;

    if(!storage_spec.is_extern())
      symbol.is_extern = false;

    if(declarator.get_bool("#template_case"))
      return symbol;

    combine_types(declarator.name().location(), final_type, symbol);
    enforce_rules(symbol);

    // initializer?
    handle_initializer(symbol, declarator);

    if(symbol.type.id()=="cpp-template-type")
    {
      cpp_scopet::id_sett id_set;
      scope->lookup_id(symbol.name,cpp_idt::TEMPLATE_ARGUMENT,id_set);
      if(id_set.empty())
      {
        cpp_idt &identifier=
          cpp_typecheck.cpp_scopes.put_into_scope(symbol,*scope);
        identifier.id_class=cpp_idt::TEMPLATE_ARGUMENT;
      }
    }

    return symbol;
  }
}

/*******************************************************************\

Function: cpp_declarator_convertert::combine_types

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void cpp_declarator_convertert::combine_types(
  const locationt &location,
  const typet &decl_type,
  symbolt &symbol)
{
  if(symbol.type.id()==decl_type.id() &&
     decl_type.id()=="code")
  {
    // functions need special treatment due
    // to argument names and default values
    const code_typet &decl_code_type=to_code_type(decl_type);
    code_typet &symbol_code_type=to_code_type(symbol.type);

    if(decl_code_type.return_type()==symbol_code_type.return_type() &&
       decl_code_type.arguments().size()==symbol_code_type.arguments().size())
    {

      for(unsigned i=0; i<decl_code_type.arguments().size(); i++)
      {
        const code_typet::argumentt &decl_argument=decl_code_type.arguments()[i];
        code_typet::argumentt &symbol_argument=symbol_code_type.arguments()[i];

        // first check type
        if(decl_argument.type()!=symbol_argument.type())
        {
          // The 'this' argument of virtual functions mismatches
          if(i!=0 || !symbol_code_type.get_bool("#is_virtual"))
          {
            cpp_typecheck.err_location(location);
            cpp_typecheck.str << "symbol `" << symbol.display_name()
                              << "': argument " << (i+1) << " type mismatch"
                              << std::endl;
            cpp_typecheck.str << "previous type: "
                              << cpp_typecheck.to_string(symbol_argument.type()) << std::endl;
            cpp_typecheck.str << "new type: "
                              << cpp_typecheck.to_string(decl_argument.type());
            throw 0;
          }
        }

        if(symbol.value.is_nil())
        {
          symbol_argument.set_base_name(decl_argument.get_base_name());
          symbol_argument.set_identifier(decl_argument.get_identifier());
          symbol_argument.location()=decl_argument.location();
        }
      }

      // ok
      return;
    }
  }
  else if(symbol.type==decl_type)
    return; // ok

  cpp_typecheck.err_location(location);
  cpp_typecheck.str << "symbol `" << symbol.display_name()
                    << "' already declared with different type"
                    << std::endl;
  cpp_typecheck.str << "previous type: "
                    << cpp_typecheck.to_string(symbol.type) << std::endl;
  cpp_typecheck.str << "new type: "
                    << cpp_typecheck.to_string(final_type);
  throw 0;
}

/*******************************************************************\

Function: cpp_declarator_convertert::enforce_rules

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void cpp_declarator_convertert::enforce_rules(const symbolt &symbol)
{
  // enforce rules for operator overloading
  operator_overloading_rules(symbol);

  // enforce rules about main()
  main_function_rules(symbol);
}

/*******************************************************************\

Function: cpp_declarator_convertert::handle_initializer

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void cpp_declarator_convertert::handle_initializer(
  symbolt &symbol,
  cpp_declaratort &declarator)
{
  exprt &value=declarator.value();

  cpp_typecheck.move_member_initializers(
    declarator.member_initializers(),
    symbol.type,
    value);

  if(value.is_nil())
    return;

  if(symbol.is_extern)
  {
    // the symbol is located here
    symbol.is_extern = false;
  }

  if(symbol.value.is_nil())
  {
    symbol.value.swap(value);

    if(is_code && declarator.type().id()!="template")
      cpp_typecheck.function_bodies.push_back(&symbol);

    if(!is_code)
      cpp_typecheck.convert_initializer(symbol);
  }
  else
  {
    cpp_typecheck.err_location(declarator.name());

    if(is_code)
    {
      cpp_typecheck.str << "body of function `"
                        << symbol.display_name()
                        << "' has already been defined";
    }
    else
      cpp_typecheck.str << "symbol `"
                        << symbol.display_name()
                        << "' already has an initializer";

    throw 0;
  }
}

/*******************************************************************\

Function: cpp_declarator_convertert::get_final_identifier

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void cpp_declarator_convertert::get_final_identifier()
{
  std::string identifier=base_name;

  // main is always "C" linkage
  if(is_code && base_name=="main" && scope->prefix=="")
  {
    mode="C";
  }

  if(is_code &&
     mode!="C")
    identifier+=id2string(cpp_typecheck.function_identifier(final_type));

  final_identifier=
    cpp_identifier_prefix(mode)+"::"+
    scope->prefix+
    identifier;
}

/*******************************************************************\

Function: cpp_declarator_convertert::convert_new_symbol

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

symbolt& cpp_declarator_convertert::convert_new_symbol(
  const cpp_storage_spect &storage_spec,
  const cpp_member_spect &member_spec,
  cpp_declaratort &declarator)
{
  std::string pretty_name(get_pretty_name());

  symbolt symbol;

  symbol.name=final_identifier;
  symbol.base_name=base_name;
  symbol.value = declarator.value();
  symbol.location=declarator.name().location();
  symbol.mode=mode;
  symbol.module=cpp_typecheck.module;
  symbol.type = final_type;
  symbol.is_type=is_typedef;
  symbol.is_macro=is_typedef && !is_template_argument;
  symbol.pretty_name=pretty_name;
  symbol.mode=cpp_typecheck.current_mode;

  if(member_spec.is_inline())
    symbol.type.set("#inlined",true);

  if(!symbol.is_type)
  {
    if(!is_code)
    {
      // it is a variable
      symbol.is_statevar=true;
      symbol.lvalue = !is_reference(symbol.type) &&
                      !(symbol.type.get_bool("#constant") &&
                        is_number(symbol.type) &&
                        symbol.value.id() == "constant");

      if(cpp_typecheck.cpp_scopes.current_scope().is_global_scope())
      {
        symbol.static_lifetime=true;

        if(storage_spec.is_extern())
          symbol.is_extern=true;
      }
      else
      {
        if(storage_spec.is_static())
        {
          symbol.static_lifetime=true;
          symbol.file_local=true;
        }
        else if(storage_spec.is_extern())
        {
          cpp_typecheck.err_location(storage_spec);
          throw "external storage not permitted here";
        }
      }
    }
    else
      symbol.theorem=true;
  }

  if(symbol.static_lifetime)
    cpp_typecheck.dinis.push_back(symbol.name);

  // move early, it must be visible before doing any value
  symbolt *new_symbol;

  if(cpp_typecheck.context.move(symbol, new_symbol))
    throw "cpp_typecheckt::convert_declarator: context.move() failed";


  if(!is_code && cpp_typecheck.cpp_scopes.current_scope().contains(base_name))
  {
    std::string error(base_name.c_str());
    error = "`" + error + "' already in scope";
    throw error.c_str();
  }

  // put into scope
  cpp_idt &identifier=
    cpp_typecheck.cpp_scopes.put_into_scope(*new_symbol, *scope, is_friend);

  if (is_template)
    identifier.id_class=cpp_idt::TEMPLATE;
  else if(is_template_argument)
    identifier.id_class=cpp_idt::TEMPLATE_ARGUMENT;
  else if(is_typedef)
    identifier.id_class=cpp_idt::TYPEDEF;
  else
    identifier.id_class=cpp_idt::SYMBOL;


  // do the value
  if(!new_symbol->is_type)
  {
    if(is_code && declarator.type().id() != "template")
      cpp_typecheck.function_bodies.push_back(new_symbol);

    if(!is_code)
      cpp_typecheck.convert_initializer(*new_symbol);
  }

  enforce_rules(*new_symbol);

  return *new_symbol;
}

/*******************************************************************\

Function: cpp_declarator_convertert::get_pretty_name

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

std::string cpp_declarator_convertert::get_pretty_name()
{
  if(is_code)
  {
    const irept::subt &arguments=
      final_type.find("arguments").get_sub();

    std::string result=scope->prefix+base_name+"(";

    forall_irep(it, arguments)
    {
      const typet &argument_type=((exprt &)*it).type();

      if(it!=arguments.begin())
        result+=", ";

      result+=cpp_typecheck.to_string(argument_type);
    }

    result+=")";

    return result;
  }

  return scope->prefix+base_name;
}

/*******************************************************************\

Function: cpp_declarator_convertert::operator_overloading_rules

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void cpp_declarator_convertert::operator_overloading_rules(
  const symbolt &symbol)
{

}

/*******************************************************************\

Function: cpp_declarator_convertert::main_function_rules

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void cpp_declarator_convertert::main_function_rules(
  const symbolt &symbol)
{
  if(symbol.name=="c::main")
  {
    if(symbol.type.id()!="code")
    {
      cpp_typecheck.err_location(symbol.location);
      throw "main must be function";
    }

    const typet &return_type=
      (const typet &)symbol.type.find("return_type");

    if(return_type!=int_type())
    {
      cpp_typecheck.err_location(symbol.location);
      throw "main must return int";
    }
  }
}
