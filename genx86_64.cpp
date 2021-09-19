#include "yadfa.h"

void gen_x64(const instruction_vec &i_vec, const asmjit::JitRuntime &rt,
             asmjit::CodeHolder &code) {
  using namespace asmjit;
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
  // TODO hardcoded for now, only 32 bit values
  constexpr size_t variable_size = 4;
  const size_t allocated_mem = num_variables * variable_size;

  code.init(rt.environment());

  x86::Assembler a(&code);
  // allocate memory
  a.push(x86::rbp);
  a.mov(x86::rbp, x86::rsp);
  a.sub(x86::rsp, allocated_mem);

  for (const auto &instr : i_vec) {
    if (instr->type == op_mov) {
      auto var_name = static_cast<binary_instruction *>(instr.get())->arg_1;
      auto var_value = static_cast<binary_instruction *>(instr.get())->arg_2;
      auto var_index = variables_indexes[var_name];
      auto var_offset = var_index * (-variable_size);
      // TODO check if that's number literal
      a.mov(x86::dword_ptr(x86::rbp, var_offset), std::stoi(var_value));
    }
    if (instr->type == op_add) {
      auto arg_2 = static_cast<three_addr_instruction *>(instr.get())->arg_2;
      auto arg_3 = static_cast<three_addr_instruction *>(instr.get())->arg_3;
      // TODO assuming args are lvalues
      auto arg_2_index = variables_indexes[arg_2];
      auto arg_3_index = variables_indexes[arg_3];
      auto arg_2_offset = arg_2_index * (-variable_size);
      auto arg_3_offset = arg_3_index * (-variable_size);
      a.mov(x86::rax, x86::dword_ptr(x86::rbp, arg_2_offset));
      a.add(x86::rax, x86::dword_ptr(x86::rbp, arg_3_offset));
    }
    if (instr->type == op_sub) {
      auto arg_2 = static_cast<three_addr_instruction *>(instr.get())->arg_2;
      auto arg_3 = static_cast<three_addr_instruction *>(instr.get())->arg_3;
      // TODO assuming args are lvalues
      auto arg_2_index = variables_indexes[arg_2];
      auto arg_3_index = variables_indexes[arg_3];
      auto arg_2_offset = arg_2_index * (-variable_size);
      auto arg_3_offset = arg_3_index * (-variable_size);
      a.mov(x86::rax, x86::dword_ptr(x86::rbp, arg_2_offset));
      a.sub(x86::rax, x86::dword_ptr(x86::rbp, arg_3_offset));
    }
    if (instr->type == op_mul) {
      auto arg_2 = static_cast<three_addr_instruction *>(instr.get())->arg_2;
      auto arg_3 = static_cast<three_addr_instruction *>(instr.get())->arg_3;
      // TODO assuming args are lvalues
      auto arg_2_index = variables_indexes[arg_2];
      auto arg_3_index = variables_indexes[arg_3];
      auto arg_2_offset = arg_2_index * (-variable_size);
      auto arg_3_offset = arg_3_index * (-variable_size);
      a.mov(x86::rax, x86::dword_ptr(x86::rbp, arg_2_offset));
      a.mov(x86::rcx, x86::dword_ptr(x86::rbp, arg_3_offset));
      a.mul(x86::rcx);
    }

    if (instr->type == op_div) {
      auto arg_2 = static_cast<three_addr_instruction *>(instr.get())->arg_2;
      auto arg_3 = static_cast<three_addr_instruction *>(instr.get())->arg_3;
      // TODO assuming args are lvalues
      auto arg_2_index = variables_indexes[arg_2];
      auto arg_3_index = variables_indexes[arg_3];
      auto arg_2_offset = arg_2_index * (-variable_size);
      auto arg_3_offset = arg_3_index * (-variable_size);
      a.mov(x86::rax, x86::qword_ptr(x86::rbp, arg_2_offset));
      a.cdq();
      a.idiv(x86::dword_ptr(x86::rbp, arg_3_offset));
    }
    if (instr->type == op_push) {
      auto arg = static_cast<unary_instruction *>(instr.get())->arg_1;
      auto arg_index = variables_indexes[arg];
      auto arg_offset = arg_index * (-variable_size);
      a.mov(x86::rax, x86::dword_ptr(x86::rbp, arg_offset));
      a.push(x86::rax);
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

void dump_x86_64(const asmjit::CodeHolder &code) {
  using namespace asmjit;
  CodeBuffer &buffer = code.textSection()->buffer();

  for (size_t i = 0; i < buffer.size(); i++)
    printf("%02X", buffer[i]);
  std::cout << '\n';
}
