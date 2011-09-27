/*******************************************************************\

Module:

Author: Lucas Cordeiro, lcc08r@ecs.soton.ac.uk

\*******************************************************************/

#ifndef REACHABILITY_TREE_H_
#define REACHABILITY_TREE_H_

#include <iostream>
#include <deque>
#include <set>
#include <map>
#include <options.h>
#include "execution_state.h"
#include "basic_symex.h"

#include "crypto_hash.h"

#include <goto-programs/goto_program.h>

class reachability_treet
{
public:
  reachability_treet(
    const goto_functionst &goto_functions,
    const namespacet &ns,
    optionst opts):
    _goto_functions(goto_functions),
    reached_terminal_state(NULL),
    _ns(ns),
    options(opts)
  {
    CS_bound = atoi(options.get_option("context-switch").c_str());
    deadlock_detection = options.get_bool_option("deadlock-check");
    state_hashing = options.get_bool_option("state-hashing");
    directed_interleavings = options.get_bool_option("direct-interleavings");

    if (options.get_bool_option("no-por") || options.get_bool_option("control-flow-test"))
      por = false;
    else
      por = true;

    at_end_of_run = false;
    has_complete_formula = false;
    is_same_mutex=false;
    execution_statet *s = new execution_statet(goto_functions, ns, this, initial_level2, options.get_bool_option("schedule"));
    execution_states.push_back(s);
    _cur_state_it = execution_states.begin();
  };

  virtual ~reachability_treet() { };

  execution_statet & get_cur_state();
  bool reset_to_unexplored_state();
  bool has_more_states();
  bool check_CS_bound();
  int get_CS_bound();
  bool generate_states_base(const exprt & expr);
  bool apply_static_por(execution_statet &ex_state, const exprt &expr, int i);
  bool generate_states_after_start_thread();
  bool generate_states();

  bool get_is_same_mutex(void);
  void check_mutex(const exprt &code, const execution_statet &ex_state);

  bool generate_states_before_read(const exprt &code);
  bool generate_states_before_write(const exprt &code);
  bool generate_states_before_assign(const exprt &code, execution_statet &ex_state);
  bool generate_states_before_function(const code_function_callt &code);
  bool is_global_assign(const exprt &code);

  const symbolt &lookup(const namespacet &ns, const irep_idt &identifier) const;
  bool is_at_end_of_run();
  bool is_has_complete_formula();
  void go_next_state();
  void multi_formulae_go_next_state();
  void set_is_at_end_of_run()
  {
    at_end_of_run = true;
  }

  class dfs_position {
public:
    dfs_position(const reachability_treet &rt);
    dfs_position(const std::string filename);
    bool write_to_file(const std::string filename) const;
protected:
    bool read_from_file(const std::string filename);
public:
    struct dfs_state {
      unsigned int location_number;
      unsigned int num_threads;
      unsigned int cur_thread;
      std::vector<bool> explored;
    };

    static const uint32_t file_magic;

    struct file_hdr {
      uint32_t magic;
      uint32_t checksum;
      uint32_t num_states;
      uint32_t num_ileaves;
    };

    struct file_entry {
      uint32_t location_number;
      uint16_t num_threads;
      uint16_t cur_thread;
      // Followed by bitfield for threads explored state.
    };

    std::vector<struct dfs_state> states;

    // Number of interleavings explored to date.
    unsigned int ileaves;

    // We need to be able to detect when the source files have changed somehow,
    // leading to the checkpoint being invalid. So add a checksum field. Exactly
    // how it's going to be calculated, I don't know yet.
    uint64_t checksum;
  };

  const goto_functionst &_goto_functions;

  // The current terminating execution state that we've reached
  execution_statet* reached_terminal_state;
  // Has complete formula: we have executed up to the end of the program and
  // we have an SSA formula we can verify. When this occurs, we drop back and
  // let the higher level code convert the formula/equation.
  bool has_complete_formula;
  // End of run: where we have executed up to the point where there is a
  // context switch that may be taken.
  bool at_end_of_run;
  bool state_hashing;
private:
  std::list<execution_statet*> execution_states;
  /* This is derefed and returned by get_current_state */
  std::list<execution_statet*>::iterator _cur_state_it;
  int CS_bound;
  bool is_same_mutex, deadlock_detection, por;
  bool directed_interleavings;
  const namespacet &_ns;

    /* jmorse */
  std::set<crypto_hash>hit_hashes;

  optionst options;
  goto_symex_statet::level2t initial_level2;
};

#endif /* REACHABILITY_TREE_H_ */
