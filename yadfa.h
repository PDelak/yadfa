#pragma once

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

#include "tests.h"

#define ASMJIT_STATIC
#include <asmjit/asmjit.h>

enum builtin_type {
  type_int8 = 0,
  type_int16,
  type_int32,
  type_int64,
  type_uint8,
  type_uint16,
  type_uint32,
  type_uint64,
  type_float
};

enum instruction_type {
  op_var = 0,
  op_mov,
  op_push,
  op_pop,
  op_jmp,
  op_if,
  op_call,
  op_add,
  op_sub,
  op_mul,
  op_div,
  op_ret,
  op_new,
  op_delete,
  op_cmp_eq,  // ==
  op_cmp_neq, // !=
  op_cmp_gt,  // >
  op_cmp_lt,  // <
  op_cmp_lte, // <=
  op_cmp_gte, // >=
  op_label,
  op_function,
  op_nop
};

class file_not_found_exception : public std::runtime_error {
 public:
  file_not_found_exception(const char* what) : std::runtime_error(what) {}
};

struct instruction {
  instruction(instruction_type t) : type(t) {}
  virtual std::ostream& dump(std::ostream& out) = 0;
  virtual instruction* clone() = 0;
  virtual bool is_arg_equal(const std::string& value) const = 0;
  virtual ~instruction() = default;

  instruction_type type;

 protected:
  std::ostream& dump_type(std::ostream& out) {
    switch (type) {
      case op_var:
        out << std::string("var");
        break;
      case op_mov:
        out << std::string("mov");
        break;
      case op_push:
        out << std::string("push");
        break;
      case op_pop:
        out << std::string("pop");
        break;
      case op_jmp:
        out << std::string("jmp");
        break;
      case op_if:
        out << std::string("if");
        break;
      case op_call:
        out << std::string("call");
        break;
      case op_ret:
        out << std::string("ret");
        break;
      case op_add:
        out << std::string("add");
        break;
      case op_sub:
        out << std::string("sub");
        break;
      case op_mul:
        out << std::string("mul");
        break;
      case op_div:
        out << std::string("div");
        break;
      case op_new:
        out << std::string("new");
        break;
      case op_delete:
        out << std::string("delete");
        break;
      case op_cmp_eq:
        out << std::string("cmp_eq");
        break;
      case op_cmp_neq:
        out << std::string("cmp_neq");
        break;
      case op_cmp_gt:
        out << std::string("cmp_gt");
        break;
      case op_cmp_lt:
        out << std::string("cmp_lt");
        break;
      case op_cmp_lte:
        out << std::string("cmp_lte");
        break;
      case op_cmp_gte:
        out << std::string("cmp_gte");
        break;
      case op_label:
        out << std::string("label");
        break;
      case op_function:
        out << std::string("function");
        break;
      case op_nop:
        out << std::string("nop");
        break;
      default:
        break;
    }
    return out;
  }
};

using instruction_ptr = std::unique_ptr<instruction>;
using instruction_vec = std::vector<instruction_ptr>;

struct noarg_instruction : public instruction {
  noarg_instruction(instruction_type t) : instruction(t) {}
  std::ostream& dump(std::ostream& out) {
    return dump_type(out) << '\n';
  }
  instruction* clone() {
    return new noarg_instruction(*this);
  }
  bool is_arg_equal(const std::string& value) const {
    return false;
  }
};

struct unary_instruction : public instruction {
  unary_instruction(instruction_type t, std::string arg) : instruction(t), arg_1(arg) {}
  std::string arg_1;
  std::ostream& dump(std::ostream& out) {
    return dump_type(out) << ' ' << arg_1;
  }
  instruction* clone() {
    return new unary_instruction(*this);
  }
  bool is_arg_equal(const std::string& value) const {
    return arg_1 == value;
  }
};

struct binary_instruction : public instruction {
  binary_instruction(instruction_type t, std::string a_1, std::string a_2)
      : instruction(t), arg_1(a_1), arg_2(a_2) {}
  std::string arg_1;
  std::string arg_2;
  std::ostream& dump(std::ostream& out) {
    return dump_type(out) << ' ' << arg_1 << ' ' << arg_2;
  }
  instruction* clone() {
    return new binary_instruction(*this);
  }
  bool is_arg_equal(const std::string& value) const {
    return arg_1 == value || arg_2 == value;
  }
};

struct three_addr_instruction : public instruction {
  three_addr_instruction(instruction_type t, std::string a_1, std::string a_2, std::string a_3)
      : instruction(t), arg_1(a_1), arg_2(a_2), arg_3(a_3) {}
  std::string arg_1;
  std::string arg_2;
  std::string arg_3;
  std::ostream& dump(std::ostream& out) {
    return dump_type(out) << ' ' << arg_1 << ' ' << arg_2 << ' ' << arg_3;
  }
  instruction* clone() {
    return new three_addr_instruction(*this);
  }
  bool is_arg_equal(const std::string& value) const {
    return arg_1 == value || arg_2 == value || arg_3 == value;
  }
};

struct function_instruction : public instruction {
  function_instruction(instruction_type t, std::vector<std::string> a,
                       instruction_vec i_vec)
      : instruction(t), args(a), body(std::move(i_vec)) {}
  function_instruction(const function_instruction &rhs)
      : instruction(rhs.type) {
    args = rhs.args;
    for (const auto &i : rhs.body) {
      std::unique_ptr<instruction> instr(i->clone());
      body.push_back(std::move(instr));
    }
  }
  std::vector<std::string> args;
  instruction_vec body;
  std::ostream& dump(std::ostream& out) {
    int arg_number = 0;
    dump_type(out) << ' ';
    for (const auto a : args) {
      // arguments start from second
      if (arg_number == 1) {
        out << " (";
      }
      if (arg_number > 1) {
        out << ' ';
      }
      out << a;
      ++arg_number;
    }
    // there args[0] is function name
    if (args.size() == 1) {
      out << '(';
    }

    out << ')';
    return out;
  }
  instruction *clone() { return new function_instruction(*this); }
  bool is_arg_equal(const std::string& value) const {
    for (const auto& a : args) {
      if (a == value) return true;
    }
    return false;
  }
};

struct label_table {
  using internal_label_table = std::map<std::string, int>;
  internal_label_table instance;
};

struct scanning_state {
  scanning_state(const std::string& input) : current(input.begin()), end(input.end()) {}
  std::string::const_iterator current;
  std::string::const_iterator end;
  bool eof() const {
    return current == end;
  }
  size_t line_number = 1;
};

bool isbracket(char c);
bool isminus(char c);
bool is_identifier(char c);
bool iscolon(char c);

std::string getNextToken(scanning_state& state);

void parse_var(instruction_vec& i_vec, scanning_state& state);
void parse_mov(instruction_vec& i_vec, scanning_state& state);
void parse_push(instruction_vec& i_vec, scanning_state& state);
void parse_pop(instruction_vec& i_vec, scanning_state& state);
void parse_jmp(instruction_vec& i_vec, scanning_state& state);
void parse_if(instruction_vec& i_vec, scanning_state& state);
void parse_call(instruction_vec& i_vec, scanning_state& state);
void parse_ret(instruction_vec& i_vec, scanning_state& state);
void parse_add(instruction_vec& i_vec, scanning_state& state);
void parse_sub(instruction_vec& i_vec, scanning_state& state);
void parse_mul(instruction_vec &i_vec, scanning_state &state);
void parse_div(instruction_vec &i_vec, scanning_state &state);
void parse_new(instruction_vec& i_vec, scanning_state& state);
void parse_delete(instruction_vec& i_vec, scanning_state& state);
void parse_cmp_eq(instruction_vec& i_vec, scanning_state& state);
void parse_cmp_neq(instruction_vec& i_vec, scanning_state& state);
void parse_cmp_lt(instruction_vec& i_vec, scanning_state& state);
void parse_cmp_lte(instruction_vec& i_vec, scanning_state& state);
void parse_cmp_gt(instruction_vec& i_vec, scanning_state& state);
void parse_cmp_gte(instruction_vec& i_vec, scanning_state& state);
void parse_label(instruction_vec& i_vec, scanning_state& state, label_table& table);
void parse_function(instruction_vec& i_vec, scanning_state& state, label_table& table);

std::string read_file(const std::string file);

instruction_vec parse(const std::string& filename, label_table& table);
std::string parse_instruction(instruction_vec &program, scanning_state &state,
                              label_table &table);

struct in_out_sets {
  std::vector<std::string> in_set;
  std::vector<std::string> out_set;
};

using live_range = std::pair<size_t, size_t>;
using variable_interval_map = std::multimap<std::string, live_range>;

using control_flow_graph = std::multimap<int, int>;
using gen_set = std::map<int, std::vector<std::string>>;   // aka use set
using kill_set = std::map<int, std::vector<std::string>>;  // aka def set
using liveness_sets = std::map<int, in_out_sets>;

control_flow_graph build_cfg(const instruction_vec& i_vec, const label_table& table);

control_flow_graph build_backward_cfg(const control_flow_graph& cfg);

void build_use_def_sets(const instruction_vec& i_vec, gen_set& out_gen_set, kill_set& out_kill_set);

void dump_raw_use_def_set_impl(const std::map<int, std::vector<std::string>>& input_set,
                               std::ostream& out);

void dump_raw_gen_set(const gen_set& input_gen_set, std::ostream& out);

void dump_raw_kill_set(const kill_set& input_kill_set, std::ostream& out);

void dump_raw_cfg(const instruction_vec& i_vec, const control_flow_graph& cfg, std::ostream& out);

void dump_use_def_set_to_dot(const std::string& set_label,
                             const std::map<int, std::vector<std::string>>& input_set,
                             std::ostream& out);

void dump_kill_set_to_dot(const kill_set& input_kill_set, std::ostream& out);

void dump_gen_set_to_dot(const gen_set& input_gen_set, std::ostream& out);

void dump_liveness_sets_to_dot(const std::string& set_label,
                               const liveness_sets& liveness_input_set, std::ostream& out);

void dump_cfg_to_dot(const instruction_vec& i_vec, const control_flow_graph& cfg,
                     const gen_set& input_gen_set, const gen_set& input_kill_set,
                     const liveness_sets& liveness_sets_input, std::ostream& out);

void dump_raw_liveness(const liveness_sets& liveness_sets_input, std::ostream& out);

liveness_sets liveness_analysis(const instruction_vec& i_vec, const control_flow_graph& cfg);

variable_interval_map compute_variables_live_ranges(const liveness_sets& live_sets);

void dump_variable_intervals(const variable_interval_map& variables_intervals, std::ostream& out);

void generate_gnuplot_interval(const variable_interval_map& variables_intervals);
instruction_vec remove_dead_code(const instruction_vec& i_vec,
                                 const variable_interval_map& variables_intervals);

instruction_vec optimize(const instruction_vec& i_vec,
                         const variable_interval_map& variables_intervals);

void dump_program(const instruction_vec& i_vec, std::ostream& out);

using builtin_functions_map = std::map<std::string, void*>;

// Code gen stuff
void gen_x64(const instruction_vec &i_vec, const asmjit::JitRuntime &rt,
             asmjit::CodeHolder &code, const label_table &ltable,
             const builtin_functions_map &builtin_functions);

int exec(const instruction_vec &i_vec, const label_table &ltable,
         const builtin_functions_map &builtin_functions);
void dump_x86_64(const instruction_vec &i_vec, const label_table &ltable,
                 const builtin_functions_map &builtin_functions);