#include "yadfa.h"

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

struct parse_exception : public std::runtime_error {
  parse_exception(const std::string &what) : std::runtime_error(what) {}
};

std::string getNextToken(scanning_state& state) {
  if (!state.eof() && *state.current == '\n') {
    ++state.line_number;
    auto token_end = state.current + 1;
    state.current = token_end;
  }
  if (!state.eof() && *state.current == '\r') {
    ++state.line_number;
    auto token_end = state.current + 2;
    state.current = token_end;
  }
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

void parse_mul(instruction_vec &i_vec, scanning_state &state) {
  auto arg_1 = getNextToken(state);
  auto arg_2 = getNextToken(state);
  auto arg_3 = getNextToken(state);
  i_vec.push_back(
      std::make_unique<three_addr_instruction>(op_mul, arg_1, arg_2, arg_3));
}

void parse_div(instruction_vec &i_vec, scanning_state &state) {
  auto arg_1 = getNextToken(state);
  auto arg_2 = getNextToken(state);
  auto arg_3 = getNextToken(state);
  i_vec.push_back(
      std::make_unique<three_addr_instruction>(op_div, arg_1, arg_2, arg_3));
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

void parse_function(instruction_vec& i_vec, scanning_state& state, label_table& table) {
  auto function_name = getNextToken(state);
  auto open_bracket = getNextToken(state);
  std::vector<std::string> function_args;
  function_args.push_back(function_name);
  std::string token;
  // handle function signature
  do {
    token = getNextToken(state);
    if (isdigit(token[0])) {
      function_args.back() = function_args.back() + token;
      continue;
    }
    if (token != ")") {
      function_args.push_back(token);
    }
  } while (token != ")");
  // handle function body
  instruction_vec body;
  do {
    token = parse_instruction(body, state, table);
  } while (token != "ret");
  parse_instruction(body, state, table);

  i_vec.push_back(std::make_unique<function_instruction>(
      op_function, function_args, std::move(body)));
}

void parse_nop(instruction_vec& i_vec, scanning_state& state, label_table& table) {
  i_vec.push_back(std::make_unique<noarg_instruction>(op_nop));
}

std::string read_file(const std::string file) {
  std::ifstream in(file);
  if (!in.is_open()) throw file_not_found_exception("FileNotFound");
  in.unsetf(std::ios::skipws);
  std::istream_iterator<char> begin(in);
  std::istream_iterator<char> end;
  std::string buffer;
  std::copy(begin, end, std::back_inserter(buffer));
  return buffer;
}

std::string parse_instruction(instruction_vec &program, scanning_state &state,
                              label_table &table) {
  std::string token;
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
  } else if (token == "mul") {
    parse_mul(program, state);
  } else if (token == "div") {
    parse_div(program, state);
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
  } else if (token == "function") {
    parse_function(program, state, table);
  } else if (token == "nop") {
    parse_nop(program, state, table);
  } else if (!state.eof()) {
    throw parse_exception("undefined opcode : " + token +
                          " in line : " + std::to_string(state.line_number));
  }
  return token;
}

instruction_vec parse(const std::string& filename, label_table& table) {
  std::ifstream in(filename);
  instruction_vec program;
  const auto parse_buf = read_file(filename);
  scanning_state state(parse_buf);
  do {
    parse_instruction(program, state, table);
  } while (!state.eof());
  return program;
}

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
      case op_function:
        break;
      case op_add:
      case op_sub:
      case op_mul:
      case op_div:
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

void dump_use_def_set_to_dot(const std::string& set_label,
                             const std::map<int, std::vector<std::string>>& input_set,
                             std::ostream& out) {
  out << set_label;
  out << " [label=<\n";
  out << "<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n";
  out << "<tr><td><i>";
  out << set_label;
  out << "</i></td></tr>\n";
  for (const auto node : input_set) {
    out << "<tr><td port=\"" << node.first << "\">";
    out << node.first << ":: [";
    bool first = true;
    for (const auto var : node.second) {
      if (!first) {
        out << ",";
      }
      out << var;
      first = false;
    }
    out << "]";
    out << "</td></tr>\n";
  }
  out << "</table>>]\n";
}

void dump_kill_set_to_dot(const kill_set& input_kill_set, std::ostream& out) {
  dump_use_def_set_to_dot("KILL_Set", input_kill_set, out);
}

void dump_gen_set_to_dot(const gen_set& input_gen_set, std::ostream& out) {
  dump_use_def_set_to_dot("GEN_Set", input_gen_set, out);
}

void dump_liveness_sets_to_dot(const std::string& set_label,
                               const liveness_sets& liveness_input_set, std::ostream& out) {
  out << set_label;
  out << " [label=<\n";
  out << "<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n";
  out << "<tr><td><i>";
  out << set_label;
  out << "</i></td></tr>\n";
  for (const auto node : liveness_input_set) {
    if (node.first == -1) continue;
    out << "<tr><td port=\"" << node.first << "\">";
    out << node.first << " inp "
        << ":: [";
    bool first = true;
    for (const auto var : node.second.in_set) {
      if (!first) {
        out << ",";
      }
      out << var;
      first = false;
    }
    out << "]";
    out << "</td></tr>\n";

    out << "<tr><td port=\"" << node.first << "\">";
    out << node.first << " out "
        << ":: [";

    first = true;
    for (const auto var : node.second.out_set) {
      if (!first) {
        out << ",";
      }
      out << var;
      first = false;
    }
    out << "]";
    out << "</td></tr>\n";
  }
  out << "</table>>]\n";
}

void dump_cfg_to_dot(const instruction_vec& i_vec, const control_flow_graph& cfg,
                     const gen_set& input_gen_set, const gen_set& input_kill_set,
                     const liveness_sets& liveness_sets_input, std::ostream& out) {
  out << "digraph {\n";
  out << "\tnode[shape=record,style=filled,fillcolor=gray95]\n";
  for (int i_index = 0; i_index < i_vec.size(); ++i_index) {
    out << '\t' << i_index << "[label=\"";
    out << i_index << " :: ";
    i_vec[i_index]->dump(out);
    out << "\"]\n";
  }

  dump_gen_set_to_dot(input_gen_set, out);
  dump_kill_set_to_dot(input_kill_set, out);
  dump_liveness_sets_to_dot("LIVE", liveness_sets_input, std::cout);

  for (const auto& node : cfg) {
    auto from = node.first;
    auto to = node.second;
    if (to == -1) break;
    out << '\t' << from << "->" << to << '\n';
  }
  out << "}\n\n";
}

void dump_raw_liveness(const liveness_sets& liveness_sets_input, std::ostream& out) {
  for (const auto& node : liveness_sets_input) {
    auto i_index = node.first;
    if (i_index == -1) continue;  // skip this
    bool first = true;
    std::cout << "in  (" << i_index << ") {";
    for (const auto& var : node.second.in_set) {
      if (!first) {
        out << ",";
      }
      first = false;
      out << var;
    }
    out << "}\n";
    first = true;
    std::cout << "out (" << i_index << ") {";
    for (const auto& var : node.second.out_set) {
      if (!first) {
        out << ",";
      }
      first = false;
      out << var;
    }
    out << "}\n";
  }
}

liveness_sets liveness_analysis(const instruction_vec& i_vec, const control_flow_graph& cfg) {
  const auto backward_cfg = build_backward_cfg(cfg);
  gen_set output_gen_set;
  kill_set output_kill_set;
  build_use_def_sets(i_vec, output_gen_set, output_kill_set);

  std::vector<std::string> in_set;
  std::vector<std::string> out_set;
  liveness_sets liveness_map;

  auto end_node_it = backward_cfg.find(-1);
  auto range = backward_cfg.equal_range(-1);
  // end node must always exists
  assert(end_node_it != backward_cfg.end());
  // end node has only one predessor
  assert(std::distance(range.first, range.second) == 1);

  auto end_node = end_node_it->second;
  std::stack<int> workList;
  workList.push(end_node);
  while (!workList.empty()) {
    auto current_node = workList.top();
    workList.pop();

    // OUT(node) = U IN(p) where p E succ(node)
    std::vector<std::string> union_of_in_set;
    auto successors_range = cfg.equal_range(current_node);
    for (auto succ_begin = successors_range.first; succ_begin != successors_range.second;
         ++succ_begin) {
      std::set_union(liveness_map[succ_begin->second].in_set.begin(),
                     liveness_map[succ_begin->second].in_set.end(), union_of_in_set.begin(),
                     union_of_in_set.end(),
                     std::inserter(union_of_in_set, union_of_in_set.begin()));
    }
    liveness_map[current_node].out_set = union_of_in_set;

    // IN(node) = (OUT(node -- KILL_SET(node)) U GEN_SET(node)
    std::vector<std::string> out_kill_diff;
    std::set_difference(liveness_map[current_node].out_set.begin(),
                        liveness_map[current_node].out_set.end(),
                        output_kill_set[current_node].begin(), output_kill_set[current_node].end(),
                        std::inserter(out_kill_diff, out_kill_diff.begin()));

    std::set_union(out_kill_diff.begin(), out_kill_diff.end(), output_gen_set[current_node].begin(),
                   output_gen_set[current_node].end(),
                   std::inserter(liveness_map[current_node].in_set,
                                 liveness_map[current_node].in_set.begin()));

    auto next_node_it = backward_cfg.find(current_node);
    if (next_node_it != backward_cfg.end()) {
      workList.push(next_node_it->second);
    }
  }

  return liveness_map;
}

variable_interval_map compute_variables_live_ranges(const liveness_sets& live_sets) {
  variable_interval_map variables_intervals;
  std::multimap<std::string, int> variable_live_points;
  for (const auto& in_out_live_set : live_sets) {
    for (const auto& variable : in_out_live_set.second.in_set) {
      variable_live_points.insert({variable, in_out_live_set.first});
    }
    for (const auto& variable : in_out_live_set.second.out_set) {
      variable_live_points.insert({variable, in_out_live_set.first});
    }
  }
  int previous = variable_live_points.begin()->second;
  int begin = previous;
  std::string previousVar = variable_live_points.begin()->first;
  int index = 0;
  for (const auto& var : variable_live_points) {
    if (var.second - previous > 1 || var.first != previousVar) {
      live_range range;
      range.first = begin;
      range.second = previous;
      variables_intervals.insert({previousVar, range});
      begin = var.second;
      previousVar = var.first;
    }
    if (index == variable_live_points.size() - 1) {
      live_range range;
      range.first = begin;
      range.second = var.second;
      variables_intervals.insert({previousVar, range});
    }
    previous = var.second;
    ++index;
  }
  return variables_intervals;
}

void dump_variable_intervals(const variable_interval_map& variables_intervals, std::ostream& out) {
  for (const auto& interval : variables_intervals) {
    out << interval.first << "[" << interval.second.first << "," << interval.second.second << "]"
        << std::endl;
  }
}

void generate_gnuplot_interval(const variable_interval_map& variables_intervals) {
  std::map<std::string, int> variable_to_index;
  size_t min_range = std::numeric_limits<size_t>::max();
  size_t max_range = std::numeric_limits<size_t>::min();
  for (const auto& interval : variables_intervals) {
    if (interval.second.first < min_range) {
      min_range = interval.second.first;
    }
    if (interval.second.second > max_range) {
      max_range = interval.second.second;
    }
  }

  {
    std::ofstream out("variables.dat");
    out << "set ytics(";
    bool first = true;
    int index = 1;
    for (const auto& interval : variables_intervals) {
      variable_to_index[interval.first] = index;
      if (!first) {
        out << ",";
      }
      first = false;

      out << "\"" << interval.first << "\""
          << " " << variable_to_index[interval.first];
      ++index;
    }
    out << ")";
  }
  {
    std::ofstream out("intervals.dat");
    for (const auto& interval : variables_intervals) {
      out << interval.second.first << " " << variable_to_index[interval.first] << "\n";
      out << interval.second.second << " " << variable_to_index[interval.first] << "\n";
      out << "\n";
    }
  }

  {
    std::ofstream out("intervals.gpi");
    out << "set terminal png\n";
    out << "set xrange[" << min_range << ":" << max_range << "]\n";
    out << "set yrange["
        << "0"
        << ":" << variable_to_index.size() + 3 << "]\n";
    out << "set style line 2 \\\n";
    out << "\tlinecolor rgb '#dd181f' \\\n";
    out << "\tlinetype 1 linewidth 2 \\\n";
    out << "\tpointtype 5 pointsize 1.5\n";
    out << "load \""
           "variables.dat\"\n";
    out << "plot 'intervals.dat' with linespoints linestyle 2 title ''\n";
  }
}

instruction_vec remove_dead_code(const instruction_vec& i_vec,
                                 const variable_interval_map& variables_intervals) {
  instruction_vec optimized_i_vec;
  std::set<size_t> added_instructions;

  for (int line_index = 0; line_index != i_vec.size(); ++line_index) {
    const auto& instr = i_vec[line_index];
    if (instr->type == op_var) {
      // add all variables decl for now
      auto var_name = static_cast<binary_instruction*>(instr.get())->arg_1;
      std::unique_ptr<instruction> var_instr(instr->clone());
      optimized_i_vec.push_back(std::move(var_instr));
    }
    // for now just copy function, nop, jmp, label and call statements
    // their behavior should be correctly implemented during
    // building use-def sets
    else if (instr->type == op_function || instr->type == op_call || instr->type == op_jmp ||
             instr->type == op_nop || instr->type == op_label) {
      std::unique_ptr<instruction> var_instr(instr->clone());
      optimized_i_vec.push_back(std::move(var_instr));
    } else {
      for (const auto& interval : variables_intervals) {
        if (instr->is_arg_equal(interval.first)) {
          // check live range
          if (interval.second.first > line_index || interval.second.second < line_index) {
            continue;
          }
          // intervals for different variable may overlap
          // prevent that
          if (added_instructions.find(line_index) != added_instructions.end()) {
            continue;
          }
          std::unique_ptr<instruction> var_instr(instr->clone());
          optimized_i_vec.push_back(std::move(var_instr));
          added_instructions.insert(line_index);
        }
      }
    }
  }

  return optimized_i_vec;
}

instruction_vec optimize(const instruction_vec& i_vec,
                         const variable_interval_map& variables_intervals) {
  return remove_dead_code(i_vec, variables_intervals);
}

void dump_program(const instruction_vec& i_vec, std::ostream& out) {
  for (const auto& i : i_vec) {
    i->dump(out);
    out << '\n';
  }
}
