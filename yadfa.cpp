#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

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
  op_ret,
  op_new,
  op_delete,
  op_cmp_eq,   // ==
  op_cmp_neq,  // !=
  op_cmp_gt,   // >
  op_cmp_lt,   // <
  op_cmp_lte,  // <=
  op_cmp_gte,  // >=
  op_label
};

struct instruction {
  instruction(instruction_type t) : type(t) {}
  virtual std::ostream& dump(std::ostream& out) = 0;
  virtual instruction* clone() = 0;
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
      default:
        break;
    }
    return out;
  }
};

using instruction_ptr = std::unique_ptr<instruction>;

struct noarg_instruction : public instruction {
  noarg_instruction(instruction_type t) : instruction(t) {}
  std::ostream& dump(std::ostream& out) {
    return dump_type(out) << '\n';
  }
  instruction* clone() {
    return new noarg_instruction(*this);
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
};

struct label_table {
  using internal_label_table = std::map<std::string, int>;
  internal_label_table instance;
};

using instruction_vec = std::vector<instruction_ptr>;

struct scanning_state {
  scanning_state(const std::string& input) : current(input.begin()), end(input.end()) {}
  std::string::const_iterator current;
  std::string::const_iterator end;
  bool eof() const {
    return current == end;
  }
};

bool isbracket(char c) {
  return c == '(' || c == ')';
}

bool isminus(char c) {
  return c == '-';
}

bool is_identifier(char c) {
  return isalpha(c) || c == '_';
}

bool iscolon(char c) {
  return c == ':';
}

std::string getNextToken(scanning_state& state) {
  if (!state.eof() && isspace(*state.current)) {
    auto end = std::find_if_not(state.current, state.end, isspace);
    state.current = end;
  }
  if (!state.eof() && is_identifier(*state.current)) {
    auto token_end = std::find_if_not(state.current, state.end, is_identifier);
    std::string token = std::string(state.current, token_end);
    state.current = token_end;

    return token;
  }
  if (!state.eof() && isdigit(*state.current)) {
    auto token_end = std::find_if_not(state.current, state.end, isdigit);
    std::string token = std::string(state.current, token_end);
    state.current = token_end;

    return token;
  }

  if (!state.eof() && isbracket(*state.current)) {
    auto token_end = state.current + 1;
    std::string token = std::string(state.current, token_end);
    state.current = token_end;
    return token;
  }

  if (!state.eof() && isminus(*state.current)) {
    auto token_end = state.current + 1;
    std::string token = std::string(state.current, token_end);
    state.current = token_end;
    return token;
  }
  if (!state.eof() && iscolon(*state.current)) {
    auto token_end = state.current + 1;
    std::string token = std::string(state.current, token_end);
    state.current = token_end;
    return token;
  }
  return "";
}

void parse_var(instruction_vec& i_vec, scanning_state& state) {
  auto arg = getNextToken(state);
  auto type = getNextToken(state);
  auto type_size = getNextToken(state);
  i_vec.push_back(std::make_unique<binary_instruction>(op_var, arg, type + type_size));
}

void parse_mov(instruction_vec& i_vec, scanning_state& state) {
  auto arg_1 = getNextToken(state);
  auto arg_2 = getNextToken(state);
  i_vec.push_back(std::make_unique<binary_instruction>(op_mov, arg_1, arg_2));
}

void parse_push(instruction_vec& i_vec, scanning_state& state) {
  auto arg = getNextToken(state);
  i_vec.push_back(std::make_unique<unary_instruction>(op_push, arg));
}

void parse_pop(instruction_vec& i_vec, scanning_state& state) {
  auto arg = getNextToken(state);
  i_vec.push_back(std::make_unique<unary_instruction>(op_pop, arg));
}

void parse_jmp(instruction_vec& i_vec, scanning_state& state) {
  auto arg = getNextToken(state);
  if (arg == "-") {
    arg = getNextToken(state);
    arg = "-" + arg;
  }
  i_vec.push_back(std::make_unique<unary_instruction>(op_jmp, arg));
}

void parse_if(instruction_vec& i_vec, scanning_state& state) {
  auto arg_1 = getNextToken(state);
  auto arg_2 = getNextToken(state);
  if (arg_2 == "-") {
    arg_2 = getNextToken(state);
    arg_2 = "-" + arg_2;
  }

  i_vec.push_back(std::make_unique<binary_instruction>(op_if, arg_1, arg_2));
}

void parse_call(instruction_vec& i_vec, scanning_state& state) {
  auto arg = getNextToken(state);
  if (arg == "-") {
    arg = getNextToken(state);
    arg = "-" + arg;
  }
  i_vec.push_back(std::make_unique<unary_instruction>(op_call, arg));
}

void parse_ret(instruction_vec& i_vec, scanning_state& state) {
  i_vec.push_back(std::make_unique<noarg_instruction>(op_ret));
}

void parse_add(instruction_vec& i_vec, scanning_state& state) {
  auto arg_1 = getNextToken(state);
  auto arg_2 = getNextToken(state);
  auto arg_3 = getNextToken(state);
  i_vec.push_back(std::make_unique<three_addr_instruction>(op_add, arg_1, arg_2, arg_3));
}

void parse_sub(instruction_vec& i_vec, scanning_state& state) {
  auto arg_1 = getNextToken(state);
  auto arg_2 = getNextToken(state);
  auto arg_3 = getNextToken(state);
  i_vec.push_back(std::make_unique<three_addr_instruction>(op_sub, arg_1, arg_2, arg_3));
}

void parse_new(instruction_vec& i_vec, scanning_state& state) {
  auto arg = getNextToken(state);
  i_vec.push_back(std::make_unique<unary_instruction>(op_new, arg));
}

void parse_delete(instruction_vec& i_vec, scanning_state& state) {
  auto arg = getNextToken(state);
  i_vec.push_back(std::make_unique<unary_instruction>(op_delete, arg));
}

void parse_cmp_eq(instruction_vec& i_vec, scanning_state& state) {
  auto arg_1 = getNextToken(state);
  auto arg_2 = getNextToken(state);
  auto arg_3 = getNextToken(state);
  i_vec.push_back(std::make_unique<three_addr_instruction>(op_cmp_eq, arg_1, arg_2, arg_3));
}

void parse_cmp_neq(instruction_vec& i_vec, scanning_state& state) {
  auto arg_1 = getNextToken(state);
  auto arg_2 = getNextToken(state);
  auto arg_3 = getNextToken(state);
  i_vec.push_back(std::make_unique<three_addr_instruction>(op_cmp_neq, arg_1, arg_2, arg_3));
}

void parse_cmp_lt(instruction_vec& i_vec, scanning_state& state) {
  auto arg_1 = getNextToken(state);
  auto arg_2 = getNextToken(state);
  auto arg_3 = getNextToken(state);
  i_vec.push_back(std::make_unique<three_addr_instruction>(op_cmp_lt, arg_1, arg_2, arg_3));
}

void parse_cmp_lte(instruction_vec& i_vec, scanning_state& state) {
  auto arg_1 = getNextToken(state);
  auto arg_2 = getNextToken(state);
  auto arg_3 = getNextToken(state);
  i_vec.push_back(std::make_unique<three_addr_instruction>(op_cmp_lte, arg_1, arg_2, arg_3));
}

void parse_cmp_gt(instruction_vec& i_vec, scanning_state& state) {
  auto arg_1 = getNextToken(state);
  auto arg_2 = getNextToken(state);
  auto arg_3 = getNextToken(state);
  i_vec.push_back(std::make_unique<three_addr_instruction>(op_cmp_gt, arg_1, arg_2, arg_3));
}

void parse_cmp_gte(instruction_vec& i_vec, scanning_state& state) {
  auto arg_1 = getNextToken(state);
  auto arg_2 = getNextToken(state);
  auto arg_3 = getNextToken(state);
  i_vec.push_back(std::make_unique<three_addr_instruction>(op_cmp_gte, arg_1, arg_2, arg_3));
}

void parse_label(instruction_vec& i_vec, scanning_state& state, label_table& table) {
  auto arg = getNextToken(state);
  i_vec.push_back(std::make_unique<unary_instruction>(op_label, arg));
  table.instance[arg] = i_vec.size();
  auto colon = getNextToken((state));
}

instruction_vec parse(const std::string& filename, label_table& table) {
  std::ifstream in(filename);
  std::string line;
  instruction_vec program;
  while (std::getline(in, line)) {
    std::string token;
    scanning_state state(line);
    token = getNextToken(state);
    if (token == "var") {
      parse_var(program, state);
    } else if (token == "mov") {
      parse_mov(program, state);
    } else if (token == "push") {
      parse_push(program, state);
    } else if (token == "pop") {
      parse_pop(program, state);
    } else if (token == "jmp") {
      parse_jmp(program, state);
    } else if (token == "if") {
      parse_if(program, state);
    } else if (token == "call") {
      parse_call(program, state);
    } else if (token == "ret") {
      parse_ret(program, state);
    } else if (token == "add") {
      parse_add(program, state);
    } else if (token == "sub") {
      parse_sub(program, state);
    } else if (token == "new") {
      parse_new(program, state);
    } else if (token == "delete") {
      parse_delete(program, state);
    } else if (token == "cmp_eq") {
      parse_cmp_eq(program, state);
    } else if (token == "cmp_neq") {
      parse_cmp_neq(program, state);
    } else if (token == "cmp_lt") {
      parse_cmp_lt(program, state);
    } else if (token == "cmp_gt") {
      parse_cmp_gt(program, state);
    } else if (token == "cmp_gte") {
      parse_cmp_gte(program, state);
    } else if (token == "cmp_lte") {
      parse_cmp_lte(program, state);
    } else if (token == "label") {
      parse_label(program, state, table);
    }
  }
  return program;
}

using control_flow_graph = std::multimap<int, int>;
using gen_set = std::map<int, std::vector<std::string>>;   // aka use set
using kill_set = std::map<int, std::vector<std::string>>;  // aka def set

control_flow_graph build_cfg(const instruction_vec& i_vec, const label_table& table) {
  std::stack<int> call_stack;
  control_flow_graph cfg;
  if (i_vec.empty()) {
    return cfg;
  }
  if (i_vec.size() == 1) {
    cfg.insert({0, -1});
    return cfg;
  }
  for (int i_index = 0; i_index < i_vec.size();++i_index) {
    if (i_vec[i_index]->type != op_jmp && i_vec[i_index]->type != op_call &&
        i_vec[i_index]->type != op_if && i_vec[i_index]->type != op_ret) {
      // last instruction does not have continuation
      // insert -1 in this case
      if (i_index == i_vec.size() - 1) {
        cfg.insert({i_index, -1});
      } else {
        cfg.insert({i_index, i_index + 1});
      }
    } else if (i_vec[i_index]->type == op_jmp) {
      auto arg = static_cast<unary_instruction*>(i_vec[i_index].get())->arg_1;
      if (!isalpha(arg[0])) {
        cfg.insert({i_index, i_index + std::stoi(arg)});
      } else {
        auto label_index_it = table.instance.find(arg);
        if (label_index_it != table.instance.end()) {
          cfg.insert({i_index, label_index_it->second});
        }
      }
    } else if (i_vec[i_index]->type == op_if) {
      auto arg = static_cast<binary_instruction*>(i_vec[i_index].get())->arg_2;
      if (!isalpha(arg[0])) {
        cfg.insert({i_index, i_index + std::stoi(arg)});
      } else {
        auto label_index_it = table.instance.find(arg);
        if (label_index_it != table.instance.end()) {
          cfg.insert({i_index, label_index_it->second});
        }
      }
      if (i_index == i_vec.size() - 1) {
        cfg.insert({i_index, -1});
      } else {
        cfg.insert({i_index, i_index + 1});
      }
    } else if (i_vec[i_index]->type == op_call) {
      auto arg = static_cast<unary_instruction*>(i_vec[i_index].get())->arg_1;
      cfg.insert({i_index, i_index + std::stoi(arg)});
      cfg.insert({i_index, i_index + 1});
      call_stack.push(i_index);
    } else if (i_vec[i_index]->type == op_ret) {
      if (!call_stack.empty()) {
        auto new_index = call_stack.top();
        call_stack.pop();
        cfg.insert({i_index, new_index + 1});
      }
    }
  }
  return cfg;
}

control_flow_graph build_backward_cfg(const control_flow_graph& cfg) {
  control_flow_graph backward_cfg;
  for (const auto& node : cfg) {
    auto from = node.first;
    auto to = node.second;
    backward_cfg.insert({to, from});
  }
  return backward_cfg;
}

void build_use_def_sets(const instruction_vec& i_vec, gen_set& out_gen_set,
                        kill_set& out_kill_set) {
  for (int i_index = 0; i_index < i_vec.size(); ++i_index) {
    switch (i_vec[i_index]->type) {
      case op_var:  // for decl, do nothing
        break;
      case op_mov:
        out_kill_set[i_index].push_back(
            static_cast<binary_instruction*>(i_vec[i_index].get())->arg_1);
        if (static_cast<binary_instruction*>(i_vec[i_index].get())->arg_2[0] != '-' &&
            !isdigit(static_cast<binary_instruction*>(i_vec[i_index].get())->arg_2[0])) {
          out_gen_set[i_index].push_back(
              static_cast<binary_instruction*>(i_vec[i_index].get())->arg_2);
        }
        break;
      case op_push:
        out_gen_set[i_index].push_back(
            static_cast<unary_instruction*>(i_vec[i_index].get())->arg_1);
        break;
      case op_pop:
        out_gen_set[i_index].push_back(
            static_cast<unary_instruction*>(i_vec[i_index].get())->arg_1);
        break;
      case op_jmp:  // for unconditional jmp, do nothing
        break;
      case op_if:
        out_gen_set[i_index].push_back(
            static_cast<binary_instruction*>(i_vec[i_index].get())->arg_1);
        break;
      case op_call:  // do nothing
        break;
      case op_add:
      case op_sub:
        out_kill_set[i_index].push_back(
            static_cast<three_addr_instruction*>(i_vec[i_index].get())->arg_1);
        out_gen_set[i_index].push_back(
            static_cast<three_addr_instruction*>(i_vec[i_index].get())->arg_2);
        out_gen_set[i_index].push_back(
            static_cast<three_addr_instruction*>(i_vec[i_index].get())->arg_3);
        break;
      case op_ret:  // do nothing
        break;
      case op_new:
      case op_delete:
        out_gen_set[i_index].push_back(
            static_cast<three_addr_instruction*>(i_vec[i_index].get())->arg_1);
        break;
      case op_cmp_eq:
      case op_cmp_neq:
      case op_cmp_gt:
      case op_cmp_lt:
      case op_cmp_lte:
      case op_cmp_gte:
        out_kill_set[i_index].push_back(
            static_cast<three_addr_instruction*>(i_vec[i_index].get())->arg_1);
        out_gen_set[i_index].push_back(
            static_cast<three_addr_instruction*>(i_vec[i_index].get())->arg_2);
        out_gen_set[i_index].push_back(
            static_cast<three_addr_instruction*>(i_vec[i_index].get())->arg_3);
        break;
      case op_label:  // do nothing
        break;
    }
  }
}

void liveness_analysis(const control_flow_graph& cfg) {
  const auto backward_cfg = build_backward_cfg(cfg);
}

void dump_raw_use_def_set_impl(const std::map<int, std::vector<std::string>>& input_set,
                               std::ostream& out) {
  for (const auto& node : input_set) {
    auto i_index = node.first;
    out << '\t' << i_index << "->";
    bool first = true;
    for (const auto& var : node.second) {
      if (!first) {
        out << ",";
      }
      first = false;
      out << var;
    }
    out << '\n';
  }
}

void dump_raw_gen_set(const gen_set& input_gen_set, std::ostream& out) {
  out << "GEN set :" << '\n';
  dump_raw_use_def_set_impl(input_gen_set, out);
}

void dump_raw_kill_set(const kill_set& input_kill_set, std::ostream& out) {
  out << "KILL set :" << '\n';
  dump_raw_use_def_set_impl(input_kill_set, out);
}

void dump_raw_cfg(const instruction_vec& i_vec, const control_flow_graph& cfg, std::ostream& out) {
  for (int i_index = 0; i_index < i_vec.size(); ++i_index) {
    out << i_index << " <- ";
    i_vec[i_index]->dump(out);
    out << '\n';
  }
  out << '\n';
  for (const auto& node : cfg) {
    auto from = node.first;
    auto to = node.second;
    out << '\t' << from << "->" << to << '\n';
  }
}

void dump_cfg_to_dot(const instruction_vec& i_vec, const control_flow_graph& cfg,
                     std::ostream& out) {
  out << "digraph {\n";
  out << "\tnode[shape=record,style=filled,fillcolor=gray95]\n";
  for (int i_index = 0; i_index < i_vec.size(); ++i_index) {
    out << '\t' << i_index << "[label=\"";
    i_vec[i_index]->dump(out);
    out << "\"]\n";
  }
  for (const auto& node : cfg) {
    auto from = node.first;
    auto to = node.second;
    if (to == -1) break;
    out << '\t' << from << "->" << to << '\n';
  }
  out << "}\n\n";
}

void test_build_instruction_vec_by_hand() {
  instruction_vec program;
  program.push_back(std::make_unique<binary_instruction>(op_var, "a", "int32"));
  program.push_back(std::make_unique<binary_instruction>(op_mov, "a", "4"));
  program.push_back(std::make_unique<binary_instruction>(op_var, "b", "int8"));
  program.push_back(std::make_unique<binary_instruction>(op_mov, "b", "2"));
  program.push_back(std::make_unique<three_addr_instruction>(op_add, "c", "a", "b"));

  std::string expected[5] = {"var a int32", "mov a 4", "var b int8", "mov b 2", "add c a b"};
  int index = 0;
  for (const auto& i : program) {
    std::ostringstream instr;
    i->dump(instr);
    assert(instr.str() == expected[index]);
    ++index;
  }
}

void test_sequential_code() {
  instruction_vec program;
  program.push_back(std::make_unique<binary_instruction>(op_var, "a", "int32"));
  program.push_back(std::make_unique<binary_instruction>(op_mov, "a", "4"));
  program.push_back(std::make_unique<binary_instruction>(op_var, "b", "int8"));
  program.push_back(std::make_unique<binary_instruction>(op_mov, "b", "2"));
  label_table table;
  auto cfg = build_cfg(program, table);
  control_flow_graph expected_cfg = {{0, 1}, {1, 2}, {2, 3}, {3, -1}};
  assert(cfg == expected_cfg);
}

void test_jmp_code()
{
  instruction_vec program;
  program.push_back(std::make_unique<binary_instruction>(op_var, "a", "int32"));
  program.push_back(std::make_unique<binary_instruction>(op_mov, "a", "4"));
  program.push_back(std::make_unique<binary_instruction>(op_var, "b", "int8"));
  program.push_back(std::make_unique<binary_instruction>(op_mov, "b", "2"));
  program.push_back(std::make_unique<unary_instruction>(op_jmp, "-2"));
  label_table table;
  auto cfg = build_cfg(program, table);
  control_flow_graph expected_cfg = {{0, 1}, {1, 2}, {2, 3}, {3, 4},{4,2}};
  assert(cfg == expected_cfg);
}

void usage() {
  std::cerr << "yadfa --command  prog" << std::endl;
  std::cerr << "where command : " << std::endl;
  std::cerr << "\traw-cfg - output of raw context free graph representation" << std::endl;
  std::cerr << "\tdot-cfg - output of dot context free graph representation" << std::endl;
  std::cerr << "\tuse-def - output of use def sets" << std::endl;
  std::cerr << "\tanalysis (liveness)" << std::endl;
}

#define YADFA_ENABLE_TESTS 1

int main(int argc, char* argv[]) {
  if (argc < 2) {
    usage();
    return -1;
  }

#ifdef YADFA_ENABLE_TESTS
  test_build_instruction_vec_by_hand();
  test_sequential_code();
  test_jmp_code();
#endif
  label_table table;

  std::string command = argv[1];

  if (command == "--raw-cfg") {
    if (argc < 3) {
      usage();
      return -1;
    }

    auto program = parse(argv[2], table);
    auto cfg = build_cfg(program, table);
    dump_raw_cfg(program, cfg, std::cout);
  }
  else if (command == "--dot-cfg") {
    if (argc < 3) {
      usage();
      return -1;
    }
    auto program = parse(argv[2], table);
    auto cfg = build_cfg(program, table);
    dump_cfg_to_dot(program, cfg, std::cout);
  }
  else if (command == "--analysis") {
    if (argc < 4) {
      usage();
      return -1;
    }
    auto type_of_analysis = argv[2];
    auto program = parse(argv[3], table);
    auto cfg = build_cfg(program, table);
    liveness_analysis(cfg);
  } else if (command == "--use-def") {
    if (argc < 3) {
      usage();
      return -1;
    }
    auto program = parse(argv[2], table);
    gen_set output_gen_set;
    kill_set output_kill_set;
    build_use_def_sets(program, output_gen_set, output_kill_set);

    dump_raw_gen_set(output_gen_set, std::cout);
    dump_raw_kill_set(output_kill_set, std::cout);
  }

  return 0;
}
