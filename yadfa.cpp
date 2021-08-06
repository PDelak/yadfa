#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
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
  op_delete
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

std::string getNextToken(scanning_state& state) {
  if (!state.eof() && isspace(*state.current)) {
    auto end = std::find_if_not(state.current, state.end, isspace);
    state.current = end;
  }
  if (!state.eof() && isalpha(*state.current)) {
    auto token_end = std::find_if_not(state.current, state.end, isalpha);
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
  i_vec.push_back(std::make_unique<binary_instruction>(op_if, arg_1, arg_2));
}

void parse_call(instruction_vec& i_vec, scanning_state& state) {
  auto arg = getNextToken(state);
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

instruction_vec parse(const std::string& filename) {
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
    }
  }

  return program;
}

void test_build_instruction_vec_by_hand() {
  instruction_vec program;
  program.push_back(std::make_unique<binary_instruction>(op_var, "a", "int32"));
  program.push_back(std::make_unique<binary_instruction>(op_mov, "a", "4"));
  program.push_back(std::make_unique<binary_instruction>(op_var, "b", "int8"));
  program.push_back(std::make_unique<binary_instruction>(op_mov, "b", "2"));
  program.push_back(std::make_unique<three_addr_instruction>(op_add, "c", "a", "b"));
  program.push_back(std::make_unique<unary_instruction>(op_jmp, "label"));
  for (const auto& i : program) {
    i->dump(std::cout);
  }
}

using control_flow_graph = std::map<int, int>;

control_flow_graph build_cfg(const instruction_vec& i_vec) {
  control_flow_graph cfg;
  if (i_vec.empty()) {
    return cfg;
  }
  if (i_vec.size() == 1) {
    cfg[0] = -1;
    return cfg;
  }
  for (int i_index = 0; i_index < i_vec.size();) {
    if (i_vec[i_index]->type != op_jmp && i_vec[i_index]->type != op_call &&
        i_vec[i_index]->type != op_if && i_vec[i_index]->type != op_ret) {
      // last instruction does not have continuation
      // insert -1 in this case
      if (i_index == i_vec.size()) {
        cfg[i_index] = -1;
      } else {
        cfg[i_index] = i_index + 1;
      }
      ++i_index;
      continue;
    } else if (i_vec[i_index]->type == op_jmp) {
      auto arg = static_cast<unary_instruction*>(i_vec[i_index].get())->arg_1;
      cfg[i_index] = i_index + std::stoi(arg);
      if (std::stoi(arg) > 0) {
        //i_index = i_index + std::stoi(arg);
        i_index = i_index + 1;
      } else {
        i_index = i_index + 1;
      }
      continue;
    }
  }
  return cfg;
}

void dump_raw_cfg(const instruction_vec& i_vec, std::ostream& out) {
  auto cfg = build_cfg(i_vec);
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

void dump_cfg_to_dot(const instruction_vec& i_vec, std::ostream& out) {
  auto cfg = build_cfg(i_vec);
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

int main() {
  auto program = parse("prog");
  // for (const auto& i : program) {
  //   i->dump(std::cout);
  // }
  auto cfg = build_cfg(program);
  dump_cfg_to_dot(program, std::cout);
  //dump_raw_cfg(program, std::cout);
  return 0;
}
