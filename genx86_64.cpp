#include "yadfa.h"

struct code_generation_error : public std::runtime_error {
  code_generation_error(const char *what) : std::runtime_error(what) {}
};

struct function_definition
{
  std::string name;
  function_instruction function;
};

struct variable_info {
  size_t index = std::numeric_limits<size_t>::max();
  std::string type;
};

using function_instruction_vec = std::map<std::string, function_definition>;

void dump_x86_64(const asmjit::CodeHolder &code) {
  using namespace asmjit;
  CodeBuffer &buffer = code.textSection()->buffer();

  for (size_t i = 0; i < buffer.size(); i++)
    printf("%02X", buffer[i]);
  std::cout << '\n';
}

std::map<std::string, variable_info>
populate_variable_indexes(const instruction_vec &i_vec) {
  std::map<std::string, variable_info> variables_indexes;
  size_t num_variables = 0;
  for (size_t i_index = 0; i_index != i_vec.size(); ++i_index) {
    const auto &instr = i_vec[i_index];
    if (instr->type == op_var) {
      ++num_variables;
      auto var_name =
          static_cast<binary_instruction *>(i_vec[i_index].get())->arg_1;
      auto var_type =
          static_cast<binary_instruction *>(i_vec[i_index].get())->arg_2;

      variables_indexes[var_name] = variable_info{num_variables, var_type};
    }
  }
  return variables_indexes;
}

void gen_prolog(asmjit::x86::Assembler &a) {
  using namespace asmjit;
  a.push(x86::rbp);
  a.mov(x86::rbp, x86::rsp);
}

void gen_epilog(asmjit::x86::Assembler &a) {
  using namespace asmjit;
  a.pop(x86::rbp);
}

size_t
gen_allocation(const std::map<std::string, variable_info> &variables_indexes,
               asmjit::x86::Assembler &a) {
  using namespace asmjit;
  // TODO hardcoded for now, only 32 bit values
  constexpr size_t variable_size = 8;
  const size_t allocated_mem = variables_indexes.size() * variable_size;
  a.sub(x86::rsp, allocated_mem);
  return allocated_mem;
}

void deallocate_and_return(size_t allocated_mem, asmjit::x86::Assembler &a) {
  using namespace asmjit;
  a.add(x86::rsp, allocated_mem);
  gen_epilog(a);
  a.ret();
}

void populate_label(const instruction_vec &i_vec, asmjit::x86::Assembler &a,
                    std::map<size_t, asmjit::Label> &label_per_instruction,
                    int index) {
  const auto &instr = i_vec[index];
  auto instruction_label = a.newLabel();
  label_per_instruction[index] = instruction_label;
}

asmjit::x86::Gp get_register_by_index(int index) {
  using namespace asmjit;
  x86::Gp reg;
  switch (index) {
  case 1:
    reg = x86::rdi;
    break;
  case 2:
    reg = x86::rsi;
    break;
  case 3:
    reg = x86::rdx;
    break;
  case 4:
    reg = x86::rcx;
    break;
  case 5:
    reg = x86::r8;
    break;
  case 6:
    reg = x86::r9;
    break;

  default:
    return reg;
  }
  return reg;
}

void push_arguments_for_builtin_fun(
    asmjit::x86::Assembler &a,
    const std::map<std::string, variable_info> &variables_info,
    const builtin_function &builtin_fun, const std::vector<std::string> &args) {
  using namespace asmjit;
  constexpr auto max_number_args_via_register = 6;
  constexpr size_t variable_size = 8;
  auto args_number = args.size() - 1; // first arg is always function name
  int args_on_stack = args_number - max_number_args_via_register;
  if (args_number <= max_number_args_via_register) {
    for (int arg_index = 1; arg_index != args.size(); ++arg_index) {
      if (!isdigit(args[arg_index].front())) {
        auto var_info_it = variables_info.find(args[arg_index]);
        if (var_info_it != variables_info.end()) {
          auto var_offset = var_info_it->second.index * (-variable_size);
          a.mov(get_register_by_index(arg_index),
                x86::dword_ptr(x86::rbp, var_offset));
        }
      } else {
        a.mov(get_register_by_index(arg_index), std::stoi(args[arg_index]));
      }
    }
  } else {
    if (args_on_stack > 0) {
      for (int arg_index = 1; arg_index != max_number_args_via_register + 1;
           ++arg_index) {
        if (!isdigit(args[arg_index].front())) {

        } else {
          a.mov(get_register_by_index(arg_index), std::stoi(args[arg_index]));
        }
      }
      // arguments are passed in reverse order from last to first one
      // and first will have index 7
      // as first 6 arguments are passed via registers
      for (int arg_on_stack_index = args_on_stack; arg_on_stack_index != 0;
           --arg_on_stack_index) {
        if (!isdigit(args[max_number_args_via_register + arg_on_stack_index]
                         .front())) {

        } else {
          a.push(std::stoi(
              args[max_number_args_via_register + arg_on_stack_index]));
        }
      }
    }
  }
}

void push_arguments_for_def_fun(
    asmjit::x86::Assembler &a,
    const std::map<std::string, variable_info> &variables_info,
    const std::vector<std::string> &args) {
  using namespace asmjit;
  constexpr auto max_number_args_via_register = 6;
  constexpr size_t variable_size = 8;
  auto args_number = args.size() - 1; // first arg is always function name
  int args_on_stack = args_number - max_number_args_via_register;
  if (args_number <= max_number_args_via_register) {
    for (int arg_index = 1; arg_index != args.size(); ++arg_index) {
      if (!isdigit(args[arg_index].front())) {
        auto var_info_it = variables_info.find(args[arg_index]);
        if (var_info_it != variables_info.end()) {
          auto var_offset = var_info_it->second.index * (-variable_size);
          a.mov(get_register_by_index(arg_index),
                x86::dword_ptr(x86::rbp, var_offset));
        }
      } else {
        a.mov(get_register_by_index(arg_index), std::stoi(args[arg_index]));
      }
    }
  } else {
    if (args_on_stack > 0) {
      for (int arg_index = 1; arg_index != max_number_args_via_register + 1;
           ++arg_index) {
        if (!isdigit(args[arg_index].front())) {

        } else {
          a.mov(get_register_by_index(arg_index), std::stoi(args[arg_index]));
        }
      }
      // arguments are passed in reverse order from last to first one
      // and first will have index 7
      // as first 6 arguments are passed via registers
      for (int arg_on_stack_index = args_on_stack; arg_on_stack_index != 0;
           --arg_on_stack_index) {
        if (!isdigit(args[max_number_args_via_register + arg_on_stack_index]
                         .front())) {

        } else {
          a.push(std::stoi(
              args[max_number_args_via_register + arg_on_stack_index]));
        }
      }
    }
  }
}

void gen_x64_instruction(const instruction_vec &i_vec,
                         std::map<std::string, variable_info> &variables_info,
                         std::map<size_t, asmjit::Label> &label_per_instruction,
                         std::map<std::string, asmjit::Label> &function_labels,
                         function_instruction_vec &function_vec,
                         asmjit::x86::Assembler &a, const label_table &ltable,
                         int index,
                         const builtin_functions_map &builtin_functions) {
  using namespace asmjit;
  const auto &instr = i_vec[index];
  a.bind(label_per_instruction[index]);
  // for now mov handles lvalues as well as rvalues
  // other instructions always use lvalues
  if (instr->type == op_mov) {
    auto var_name = static_cast<binary_instruction *>(instr.get())->arg_1;
    auto var_value = static_cast<binary_instruction *>(instr.get())->arg_2;
    auto var_info = variables_info[var_name];
    std::uint8_t variable_size = 8;
    auto var_offset = var_info.index * (-variable_size);
    if (!isdigit(var_value[0])) {
      auto rhs_info = variables_info[var_value];
      auto rhs_offset = rhs_info.index * (-variable_size);
      a.mov(x86::rax, x86::qword_ptr(x86::rbp, rhs_offset));
      a.mov(x86::qword_ptr(x86::rbp, var_offset), x86::rax);
    } else {
      a.mov(x86::dword_ptr(x86::rbp, var_offset), std::stoi(var_value));
    }
  }
  if (instr->type == op_add) {
    auto arg_1 = static_cast<three_addr_instruction *>(instr.get())->arg_1;
    auto arg_2 = static_cast<three_addr_instruction *>(instr.get())->arg_2;
    auto arg_3 = static_cast<three_addr_instruction *>(instr.get())->arg_3;
    // TODO assuming args are lvalues
    std::uint8_t variable_size = 8;
    auto arg_1_info = variables_info[arg_1];
    auto arg_2_info = variables_info[arg_2];
    auto arg_3_info = variables_info[arg_3];
    auto arg_1_offset = arg_1_info.index * (-variable_size);
    auto arg_2_offset = arg_2_info.index * (-variable_size);
    auto arg_3_offset = arg_3_info.index * (-variable_size);
    a.mov(x86::rax, x86::dword_ptr(x86::rbp, arg_2_offset));
    a.add(x86::rax, x86::dword_ptr(x86::rbp, arg_3_offset));
    a.mov(x86::dword_ptr(x86::rbp, arg_1_offset), x86::eax);
  }
  if (instr->type == op_sub) {
    auto arg_1 = static_cast<three_addr_instruction *>(instr.get())->arg_1;
    auto arg_2 = static_cast<three_addr_instruction *>(instr.get())->arg_2;
    auto arg_3 = static_cast<three_addr_instruction *>(instr.get())->arg_3;
    // TODO assuming args are lvalues
    std::uint8_t variable_size = 8;
    auto arg_1_info = variables_info[arg_1];
    auto arg_2_info = variables_info[arg_2];
    auto arg_3_info = variables_info[arg_3];
    auto arg_1_offset = arg_1_info.index * (-variable_size);
    auto arg_2_offset = arg_2_info.index * (-variable_size);
    auto arg_3_offset = arg_3_info.index * (-variable_size);
    a.mov(x86::rax, x86::dword_ptr(x86::rbp, arg_2_offset));
    a.sub(x86::rax, x86::dword_ptr(x86::rbp, arg_3_offset));
    a.mov(x86::dword_ptr(x86::rbp, arg_1_offset), x86::eax);
  }
  if (instr->type == op_mul) {
    auto arg_1 = static_cast<three_addr_instruction *>(instr.get())->arg_1;
    auto arg_2 = static_cast<three_addr_instruction *>(instr.get())->arg_2;
    auto arg_3 = static_cast<three_addr_instruction *>(instr.get())->arg_3;
    // TODO assuming args are lvalues
    std::uint8_t variable_size = 8;
    auto arg_1_info = variables_info[arg_1];
    auto arg_2_info = variables_info[arg_2];
    auto arg_3_info = variables_info[arg_3];
    auto arg_1_offset = arg_1_info.index * (-variable_size);
    auto arg_2_offset = arg_2_info.index * (-variable_size);
    auto arg_3_offset = arg_3_info.index * (-variable_size);
    a.mov(x86::rax, x86::dword_ptr(x86::rbp, arg_2_offset));
    a.mov(x86::rcx, x86::dword_ptr(x86::rbp, arg_3_offset));
    a.mul(x86::rcx);
    a.mov(x86::dword_ptr(x86::rbp, arg_1_offset), x86::eax);
  }

  if (instr->type == op_div) {
    auto arg_1 = static_cast<three_addr_instruction *>(instr.get())->arg_1;
    auto arg_2 = static_cast<three_addr_instruction *>(instr.get())->arg_2;
    auto arg_3 = static_cast<three_addr_instruction *>(instr.get())->arg_3;
    // TODO assuming args are lvalues
    std::uint8_t variable_size = 8;
    auto arg_1_info = variables_info[arg_1];
    auto arg_2_info = variables_info[arg_2];
    auto arg_3_info = variables_info[arg_3];
    auto arg_1_offset = arg_1_info.index * (-variable_size);
    auto arg_2_offset = arg_2_info.index * (-variable_size);
    auto arg_3_offset = arg_3_info.index * (-variable_size);
    a.mov(x86::rax, x86::dword_ptr(x86::rbp, arg_2_offset));
    a.cdq();
    a.idiv(x86::dword_ptr(x86::rbp, arg_3_offset));
    a.mov(x86::dword_ptr(x86::rbp, arg_1_offset), x86::eax);
  }
  if (instr->type == op_push) {
    // TODO assuming args are lvalues
    std::uint8_t variable_size = 8;
    auto arg = static_cast<unary_instruction *>(instr.get())->arg_1;
    auto arg_info = variables_info[arg];
    auto arg_offset = arg_info.index * (-variable_size);
    a.push(x86::dword_ptr(x86::rbp, arg_offset));
  }
  if (instr->type == op_pop) {
    // TODO assuming args are lvalues
    std::uint8_t variable_size = 8;
    auto arg = static_cast<unary_instruction *>(instr.get())->arg_1;
    auto arg_info = variables_info[arg];
    auto arg_offset = arg_info.index * (-variable_size);
    a.pop(x86::dword_ptr(x86::rbp, arg_offset));
  }
  if (instr->type == op_jmp) {
    // TODO handle lvalues
    // only jmp backward for now
    int next_instruction_index = 0;
    auto arg = static_cast<unary_instruction *>(instr.get())->arg_1;
    // only if digit for now
    if (isdigit(arg[0])) {
      auto jmp_offset = std::stoi(arg);
      if (jmp_offset > 0) {
        jmp_offset += 1;
      }
      next_instruction_index = index + jmp_offset;
    } else {
      auto label_it = ltable.instance.find(arg);
      if (label_it != ltable.instance.end()) {
        next_instruction_index = label_it->second;
      } else {
        std::string message = "label : " + arg + " does not exists";
        throw code_generation_error(message.c_str());
      }
    }
    auto label_it = label_per_instruction.find(next_instruction_index);
    if (label_it == label_per_instruction.end()) {
      code_generation_error("instruction is out of range");
    }
    a.jmp(label_it->second);
  }
  switch (instr->type) {
  case op_cmp_eq:
  case op_cmp_neq:
  case op_cmp_gt:
  case op_cmp_lt:
  case op_cmp_lte:
  case op_cmp_gte:
    auto arg_1 = static_cast<three_addr_instruction *>(instr.get())->arg_1;
    auto arg_2 = static_cast<three_addr_instruction *>(instr.get())->arg_2;
    auto arg_3 = static_cast<three_addr_instruction *>(instr.get())->arg_3;
    // TODO assuming args are lvalues
    std::uint8_t variable_size = 8;
    auto arg_1_info = variables_info[arg_1];
    auto arg_2_info = variables_info[arg_2];
    auto arg_3_info = variables_info[arg_3];
    auto arg_1_offset = arg_1_info.index * (-variable_size);
    auto arg_2_offset = arg_2_info.index * (-variable_size);
    auto arg_3_offset = arg_3_info.index * (-variable_size);
    a.mov(x86::eax, x86::dword_ptr(x86::rbp, arg_2_offset));
    a.cmp(x86::eax, x86::dword_ptr(x86::rbp, arg_3_offset));
    auto false_label = a.newLabel();
    auto end_label = a.newLabel();
    if (instr->type == op_cmp_eq) {
      a.jne(false_label);
    } else if (instr->type == op_cmp_neq) {
      a.je(false_label);
    } else if (instr->type == op_cmp_gt) {
      a.jng(false_label);
    } else if (instr->type == op_cmp_lt) {
      a.jnl(false_label);
    } else if (instr->type == op_cmp_lte) {
      a.jnle(false_label);
    } else if (instr->type == op_cmp_gte) {
      a.jnge(false_label);
    }
    a.mov(x86::eax, 1);
    a.mov(x86::dword_ptr(x86::rbp, arg_1_offset), x86::eax);
    a.jmp(end_label);
    a.bind(false_label);
    a.mov(x86::eax, 0);
    a.mov(x86::dword_ptr(x86::rbp, arg_1_offset), x86::eax);
    a.bind(end_label);

    break;
  }
  if (instr->type == op_if) {
    auto false_label = a.newLabel();
    auto arg = static_cast<binary_instruction *>(instr.get())->arg_1;
    auto arg_info = variables_info[arg];
    std::uint8_t variable_size = 8;
    auto arg_offset = arg_info.index * (-variable_size);
    auto offset = static_cast<binary_instruction *>(instr.get())->arg_2;
    a.mov(x86::ebx, x86::dword_ptr(x86::rbp, arg_offset));
    a.cmp(x86::ebx, 0);
    a.jng(false_label);
    // only if digit for now
    int next_instruction_index = 0;
    if (isdigit(offset[0])) {
      auto jmp_offset = std::stoi(offset);
      next_instruction_index = index + jmp_offset;
    } else {
      auto label_it = ltable.instance.find(offset);
      if (label_it != ltable.instance.end()) {
        next_instruction_index = label_it->second;
      } else {
        std::string message = "label : " + arg + " does not exists";
        throw code_generation_error(message.c_str());
      }
    }
    auto label_it = label_per_instruction.find(next_instruction_index);
    if (label_it == label_per_instruction.end()) {
      code_generation_error("instruction is out of range");
    }
    a.jmp(label_it->second);
    a.bind(false_label);
  }
  if (instr->type == op_nop) {
    a.nop();
  }
  if (instr->type == op_function) {
    // function codegen is postponed
    auto args = static_cast<function_instruction *>(instr.get())->args;
    auto body =
        std::move(static_cast<function_instruction *>(instr.get())->body);
    function_definition def{args.front(), function_instruction(op_function, args,
                                                               std::move(body))};
    def.name = args.front();
    function_vec.insert({args.front(), def});
  }
  if (instr->type == op_call) {
    auto args = static_cast<call_instruction *>(instr.get())->args;
    constexpr size_t number_of_args_passed_via_regs = 6;
    constexpr size_t bytes_64 = 8;
    constexpr size_t fun_name_arg = 1;
    // first argument is always function name
    // number of arguments has to be calculaled by
    // subtract it and number passed via regs (6) from all arguments
    // and multiply by size of each argument on stack which 8 bytes
    int deallocateArgMem =
        (args.size() - fun_name_arg - number_of_args_passed_via_regs) *
        bytes_64;
    auto fun_name = args.front();
    auto label_it = function_labels.find(fun_name);
    if (label_it == function_labels.end()) {
      std::string message = "function " + fun_name + " does not exits";
      auto builtin_functions_it = builtin_functions.find(fun_name);
      if (builtin_functions_it == builtin_functions.end()) {
        throw code_generation_error(message.c_str());
      } else {
        // TODO only rvalue arguments for now
        push_arguments_for_builtin_fun(a, variables_info,
                                       builtin_functions_it->second, args);
        a.call(asmjit::imm(builtin_functions_it->second.function_pointer));
        if (args.size() > number_of_args_passed_via_regs + fun_name_arg) {
          a.add(x86::rsp, deallocateArgMem);
        }
      }
    } else {
      // TODO only rvalue arguments for now
      push_arguments_for_def_fun(a, variables_info, args);
      a.call(label_it->second);
      if (args.size() > number_of_args_passed_via_regs + fun_name_arg) {
        a.add(x86::rsp, deallocateArgMem);
      }
    }
  }
  // op_ret is no-op for now
  if (instr->type == op_ret) {
  }
  if (instr->type == op_pop_args) {
    auto args = static_cast<pop_args_instruction *>(instr.get())->args;
    // TODO for now it's taking only first six arguments via registers
    for (size_t arg_index = 0; arg_index != args.size(); ++arg_index) {
      assert(arg_index < 6);
      auto var_info = variables_info[args[arg_index].first];
      std::uint8_t variable_size = 8;
      auto var_offset = var_info.index * (-variable_size);
      a.mov(x86::qword_ptr(x86::rbp, var_offset),
            get_register_by_index(arg_index + 1));
    }
  }
}

void gen_x64(const instruction_vec &i_vec, const asmjit::JitRuntime &rt,
             asmjit::CodeHolder &code, const label_table &ltable,
             const builtin_functions_map &builtin_functions) {
  using namespace asmjit;
  code.init(rt.environment());
  x86::Assembler a(&code);

  std::map<size_t, Label> label_per_instruction;
  std::map<std::string, Label> function_labels;

  auto variables_indexes = populate_variable_indexes(i_vec);
  function_instruction_vec function_vec;

  auto mainLabel = a.newLabel();
  a.jmp(mainLabel);

  // there are two passes
  // first we traverse all functions
  // and cache them
  // then we traverse cache to generate code for each
  // funtion
  for (int index = 0; index != i_vec.size(); ++index) {
    if (i_vec[index]->type != op_function)
      continue;
    gen_x64_instruction(i_vec, variables_indexes, label_per_instruction,
                        function_labels, function_vec, a, ltable, index,
                        builtin_functions);
  }

  // allocate function arguments
  for (auto &fun : function_vec) {
    auto &function_def = fun.second;
    auto &function_body = function_def.function.body;
    const auto &function_args = function_def.function.args;
    if (!function_args.empty()) {
      std::vector<std::pair<std::string, std::string>> args_vec;
      for (size_t arg_index = 1; arg_index != function_args.size();
           arg_index = arg_index + 2) {
        args_vec.push_back(
            {function_args[arg_index], function_args[arg_index + 1]});
      }
      for (size_t arg_index = 1; arg_index != function_args.size();
           arg_index = arg_index + 2) {
        function_body.insert(
            function_body.begin(),
            std::make_unique<pop_args_instruction>(op_pop_args, args_vec));
        function_body.insert(function_body.begin(),
                             std::make_unique<binary_instruction>(
                                 op_var, function_args[arg_index],
                                 function_args[arg_index + 1]));
      }
    }
  }
  // traverse internal function cache and generate code
  // generate code for functions
  for (const auto &fun : function_vec) {
    auto function_name = fun.first;
    const auto& function_def = fun.second;
    const auto &function_body = function_def.function.body;
    const auto &function_args = function_def.function.args;
    auto function_label = a.newLabel();
    function_labels[function_name] = function_label;

    auto variables_indexes_function_body =
        populate_variable_indexes(function_body);
    a.bind(function_label);
    gen_prolog(a);
    // allocate memory
    auto allocated_mem_fun = gen_allocation(variables_indexes_function_body, a);

    for (int body_index = 0; body_index != function_body.size(); ++body_index) {
      populate_label(function_body, a, label_per_instruction, body_index);
    }
    for (int body_index = 0; body_index != function_body.size(); ++body_index) {
      gen_x64_instruction(function_body, variables_indexes_function_body,
                          label_per_instruction, function_labels, function_vec,
                          a, ltable, body_index, builtin_functions);
    }
    // deallocate
    deallocate_and_return(allocated_mem_fun, a);
  }
  // generate code for "main" function
  a.bind(mainLabel);
  gen_prolog(a);
  // allocate memory
  auto allocated_mem = gen_allocation(variables_indexes, a);

  for (int index = 0; index != i_vec.size(); ++index) {
    populate_label(i_vec, a, label_per_instruction, index);
  }
  for (int index = 0; index != i_vec.size(); ++index) {
    gen_x64_instruction(i_vec, variables_indexes, label_per_instruction,
                        function_labels, function_vec, a, ltable, index,
                        builtin_functions);
  }
  // deallocate
  deallocate_and_return(allocated_mem, a);
}

int exec(const instruction_vec &i_vec, const label_table &ltable,
         const builtin_functions_map &builtin_functions) {
  typedef int (*Func)(void);

  asmjit::CodeHolder code;
  asmjit::JitRuntime rt;

  gen_x64(i_vec, rt, code, ltable, builtin_functions);
  Func fn;
  asmjit::Error err = rt.add(&fn, &code);
  if (err)
    return -1;

  int result = fn();
  // no longer needed
  (void)result;
  return 0;
}

void dump_x86_64(const instruction_vec &i_vec, const label_table &ltable,
                 const builtin_functions_map &builtin_functions) {
  typedef int (*Func)(void);

  asmjit::CodeHolder code;
  asmjit::JitRuntime rt;

  gen_x64(i_vec, rt, code, ltable, builtin_functions);
  dump_x86_64(code);
}
