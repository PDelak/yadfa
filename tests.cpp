#include "tests.h"

#include "yadfa.h"

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

void test_jmp_code() {
  instruction_vec program;
  program.push_back(std::make_unique<binary_instruction>(op_var, "a", "int32"));
  program.push_back(std::make_unique<binary_instruction>(op_mov, "a", "4"));
  program.push_back(std::make_unique<binary_instruction>(op_var, "b", "int8"));
  program.push_back(std::make_unique<binary_instruction>(op_mov, "b", "2"));
  program.push_back(std::make_unique<unary_instruction>(op_jmp, "-2"));
  label_table table;
  auto cfg = build_cfg(program, table);
  control_flow_graph expected_cfg = {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 2}};
  assert(cfg == expected_cfg);
}
