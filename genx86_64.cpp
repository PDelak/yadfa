#include "yadfa.h"

struct code_generation_error : public std::runtime_error {
  code_generation_error(const char *what) : std::runtime_error(what) {}
};

void dump_x86_64(const asmjit::CodeHolder &code) {
  using namespace asmjit;
  CodeBuffer &buffer = code.textSection()->buffer();

  for (size_t i = 0; i < buffer.size(); i++)
    printf("%02X", buffer[i]);
  std::cout << '\n';
}

void gen_x64(const instruction_vec &i_vec, const asmjit::JitRuntime &rt,
             asmjit::CodeHolder &code) {
  using namespace asmjit;
  std::map<std::string, size_t> variables_indexes;

  std::map<size_t, Label> label_per_instruction;

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
  // TODO hardcoded for now, only 32 bit values
  constexpr size_t variable_size = 4;
  const size_t allocated_mem = num_variables * variable_size;

  code.init(rt.environment());

  x86::Assembler a(&code);
  // allocate memory
  a.push(x86::rbp);
  a.mov(x86::rbp, x86::rsp);
  a.sub(x86::rsp, allocated_mem);
  for (int index = 0; index != i_vec.size(); ++index) {
    const auto &instr = i_vec[index];
    auto instruction_label = a.newLabel();
    label_per_instruction[index] = instruction_label;
  }
  for (int index = 0; index != i_vec.size(); ++index) {
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
      a.mov(x86::dword_ptr(x86::rbp, arg_1_offset), x86::rax);
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
      a.mov(x86::dword_ptr(x86::rbp, arg_1_offset), x86::rax);
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
      a.mov(x86::dword_ptr(x86::rbp, arg_1_offset), x86::rax);
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
      a.mov(x86::dword_ptr(x86::rbp, arg_1_offset), x86::rax);
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
      auto arg = static_cast<unary_instruction *>(instr.get())->arg_1;
      // only if digit for now
      auto jmp_offset = std::stoi(arg);
      if (jmp_offset > 0) {
        jmp_offset += 1;
      }
      auto next_instruction_index = index + jmp_offset;
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
      auto jmp_offset = std::stoi(offset);
      if (jmp_offset > 0) {
        jmp_offset += 1;
      }
      auto next_instruction_index = index + jmp_offset;
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
  }

  // deallocate
  a.add(x86::rsp, allocated_mem);
  a.pop(x86::rbp);
  a.ret();
}

int exec(const instruction_vec &i_vec) {
  typedef int (*Func)(void);

  asmjit::CodeHolder code;
  asmjit::JitRuntime rt;

  gen_x64(i_vec, rt, code);
  Func fn;
  asmjit::Error err = rt.add(&fn, &code);
  if (err)
    return -1;

  int result = fn();
  printf("%d\n", result);
  return 0;
}

void dump_x86_64(const instruction_vec &i_vec) {
  typedef int (*Func)(void);

  asmjit::CodeHolder code;
  asmjit::JitRuntime rt;

  gen_x64(i_vec, rt, code);
  dump_x86_64(code);
}
