#include "yadfa.h"

struct code_generation_error : public std::runtime_error {
  code_generation_error(const char *what) : std::runtime_error(what) {}
};

using function_instruction_vec = std::map<std::string, function_instruction>;

void dump_x86_64(const asmjit::CodeHolder &code) {
  using namespace asmjit;
  CodeBuffer &buffer = code.textSection()->buffer();

  for (size_t i = 0; i < buffer.size(); i++)
    printf("%02X", buffer[i]);
  std::cout << '\n';
}

std::map<std::string, size_t>
populate_variable_indexes(const instruction_vec &i_vec) {
  std::map<std::string, size_t> variables_indexes;
  size_t num_variables = 0;
  for (size_t i_index = 0; i_index != i_vec.size(); ++i_index) {
    const auto &instr = i_vec[i_index];
    if (instr->type == op_var) {
      ++num_variables;
      auto var_name =
          static_cast<binary_instruction *>(i_vec[i_index].get())->arg_1;
      auto var_type =
          static_cast<binary_instruction *>(i_vec[i_index].get())->arg_2;

      variables_indexes[var_name] = num_variables;
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

size_t gen_allocation(const std::map<std::string, size_t> &variables_indexes,
                      asmjit::x86::Assembler &a) {
  using namespace asmjit;
  // TODO hardcoded for now, only 32 bit values
  constexpr size_t variable_size = 4;
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

void gen_x64_instruction(const instruction_vec &i_vec,
                         std::map<std::string, size_t> &variables_indexes,
                         std::map<size_t, asmjit::Label> &label_per_instruction,
                         std::map<std::string, asmjit::Label> &function_labels,
                         function_instruction_vec &function_vec,
                         asmjit::x86::Assembler &a, const label_table &ltable,
                         int index, const builtin_functions_map& builtin_functions) {
  constexpr size_t variable_size = 4;
  using namespace asmjit;
  const auto &instr = i_vec[index];
  a.bind(label_per_instruction[index]);
  if (instr->type == op_mov) {
    auto var_name = static_cast<binary_instruction *>(instr.get())->arg_1;
    auto var_value = static_cast<binary_instruction *>(instr.get())->arg_2;
    auto var_index = variables_indexes[var_name];
    auto var_offset = var_index * (-variable_size);
    // TODO check if that's number literal
    // below two lines can be replaced by last one
    // it's implemented for now this way for debugging purposes
    // just to leave most recent value in rax register
    a.mov(x86::eax, std::stoi(var_value));
    a.mov(x86::dword_ptr(x86::rbp, var_offset), x86::eax);
    // a.mov(x86::dword_ptr(x86::rbp, var_offset), std::stoi(var_value));
  }
  if (instr->type == op_add) {
    auto arg_1 = static_cast<three_addr_instruction *>(instr.get())->arg_1;
    auto arg_2 = static_cast<three_addr_instruction *>(instr.get())->arg_2;
    auto arg_3 = static_cast<three_addr_instruction *>(instr.get())->arg_3;
    // TODO assuming args are lvalues
    auto arg_1_index = variables_indexes[arg_1];
    auto arg_2_index = variables_indexes[arg_2];
    auto arg_3_index = variables_indexes[arg_3];
    auto arg_1_offset = arg_1_index * (-variable_size);
    auto arg_2_offset = arg_2_index * (-variable_size);
    auto arg_3_offset = arg_3_index * (-variable_size);
    a.mov(x86::rax, x86::dword_ptr(x86::rbp, arg_2_offset));
    a.add(x86::rax, x86::dword_ptr(x86::rbp, arg_3_offset));
    a.mov(x86::dword_ptr(x86::rbp, arg_1_offset), x86::eax);
  }
  if (instr->type == op_sub) {
    auto arg_1 = static_cast<three_addr_instruction *>(instr.get())->arg_1;
    auto arg_2 = static_cast<three_addr_instruction *>(instr.get())->arg_2;
    auto arg_3 = static_cast<three_addr_instruction *>(instr.get())->arg_3;
    // TODO assuming args are lvalues
    auto arg_1_index = variables_indexes[arg_1];
    auto arg_2_index = variables_indexes[arg_2];
    auto arg_3_index = variables_indexes[arg_3];
    auto arg_1_offset = arg_1_index * (-variable_size);
    auto arg_2_offset = arg_2_index * (-variable_size);
    auto arg_3_offset = arg_3_index * (-variable_size);
    a.mov(x86::rax, x86::dword_ptr(x86::rbp, arg_2_offset));
    a.sub(x86::rax, x86::dword_ptr(x86::rbp, arg_3_offset));
    a.mov(x86::dword_ptr(x86::rbp, arg_1_offset), x86::eax);
  }
  if (instr->type == op_mul) {
    auto arg_1 = static_cast<three_addr_instruction *>(instr.get())->arg_1;
    auto arg_2 = static_cast<three_addr_instruction *>(instr.get())->arg_2;
    auto arg_3 = static_cast<three_addr_instruction *>(instr.get())->arg_3;
    // TODO assuming args are lvalues
    auto arg_1_index = variables_indexes[arg_1];
    auto arg_2_index = variables_indexes[arg_2];
    auto arg_3_index = variables_indexes[arg_3];
    auto arg_1_offset = arg_1_index * (-variable_size);
    auto arg_2_offset = arg_2_index * (-variable_size);
    auto arg_3_offset = arg_3_index * (-variable_size);
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
    auto arg_1_index = variables_indexes[arg_1];
    auto arg_2_index = variables_indexes[arg_2];
    auto arg_3_index = variables_indexes[arg_3];
    auto arg_1_offset = arg_1_index * (-variable_size);
    auto arg_2_offset = arg_2_index * (-variable_size);
    auto arg_3_offset = arg_3_index * (-variable_size);
    a.mov(x86::rax, x86::dword_ptr(x86::rbp, arg_2_offset));
    a.cdq();
    a.idiv(x86::dword_ptr(x86::rbp, arg_3_offset));
    a.mov(x86::dword_ptr(x86::rbp, arg_1_offset), x86::eax);
  }
  if (instr->type == op_push) {
    // TODO assuming args are lvalues
    auto arg = static_cast<unary_instruction *>(instr.get())->arg_1;
    auto arg_index = variables_indexes[arg];
    auto arg_offset = arg_index * (-variable_size);
    a.push(x86::dword_ptr(x86::rbp, arg_offset));
  }
  if (instr->type == op_pop) {
    // TODO assuming args are lvalues
    auto arg = static_cast<unary_instruction *>(instr.get())->arg_1;
    auto arg_index = variables_indexes[arg];
    auto arg_offset = arg_index * (-variable_size);
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
    auto arg_1_index = variables_indexes[arg_1];
    auto arg_2_index = variables_indexes[arg_2];
    auto arg_3_index = variables_indexes[arg_3];
    auto arg_1_offset = arg_1_index * (-variable_size);
    auto arg_2_offset = arg_2_index * (-variable_size);
    auto arg_3_offset = arg_3_index * (-variable_size);
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
    auto arg_index = variables_indexes[arg];
    auto arg_offset = arg_index * (-variable_size);
    auto offset = static_cast<binary_instruction *>(instr.get())->arg_2;
    a.mov(x86::ebx, x86::dword_ptr(x86::rbp, arg_offset));
    a.cmp(x86::ebx, 0);
    a.jng(false_label);
    // only if digit for now
    int next_instruction_index = 0;
    if (isdigit(offset[0])) {
      auto jmp_offset = std::stoi(offset);
      if (jmp_offset > 0) {
        jmp_offset += 1;
      }
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
    function_vec.insert({args.front(), function_instruction(op_function, args,
                                                            std::move(body))});
  }
  if (instr->type == op_call) {
    auto arg = static_cast<unary_instruction *>(instr.get())->arg_1;
    auto label_it = function_labels.find(arg);
    if (label_it == function_labels.end()) {
      std::string message = "function " + arg + " does not exits";
      auto builtin_functions_it = builtin_functions.find(arg);
      if (builtin_functions_it == builtin_functions.end()) {
        throw code_generation_error(message.c_str());
      } else {
        a.mov(x86::rax,  builtin_functions_it->second);
        a.call(x86::rax);
      }
    } else {
      a.call(label_it->second);
    }
  }
  // op_ret is no-op for now
  if (instr->type == op_ret) {
  }
}

void gen_x64(const instruction_vec &i_vec, const asmjit::JitRuntime &rt,
             asmjit::CodeHolder &code, const label_table &ltable, const builtin_functions_map& builtin_functions) {
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
                        function_labels, function_vec, a, ltable, index, builtin_functions);
  }

  // traverse internal function cache and generate code
  // generate code for functions
  for (const auto &fun : function_vec) {
    auto function_name = fun.first;
    const auto &function_body = fun.second.body;
    const auto &instr = function_body.front();
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
                        function_labels, function_vec, a, ltable, index, builtin_functions);
  }
  // deallocate
  deallocate_and_return(allocated_mem, a);
}

int exec(const instruction_vec &i_vec, const label_table &ltable, const builtin_functions_map& builtin_functions) {
  typedef int (*Func)(void);

  asmjit::CodeHolder code;
  asmjit::JitRuntime rt;

  gen_x64(i_vec, rt, code, ltable, builtin_functions);
  Func fn;
  asmjit::Error err = rt.add(&fn, &code);
  if (err)
    return -1;

  int result = fn();
  printf("%d\n", result);
  return 0;
}

void dump_x86_64(const instruction_vec &i_vec, const label_table &ltable, const builtin_functions_map& builtin_functions) {
  typedef int (*Func)(void);

  asmjit::CodeHolder code;
  asmjit::JitRuntime rt;

  gen_x64(i_vec, rt, code, ltable, builtin_functions);
  dump_x86_64(code);
}
