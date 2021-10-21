#include "tests.h"
#include "yadfa.h"

void usage() {
  std::cerr << "yadfa --command  prog" << std::endl;
  std::cerr << "where command : " << std::endl;
  std::cerr << "\traw-cfg - output of raw context free graph representation" << std::endl;
  std::cerr << "\tdot-cfg - output of dot context free graph representation" << std::endl;
  std::cerr << "\tuse-def - output of use def sets" << std::endl;
  std::cerr << "\tanalysis (liveness)" << std::endl;
  std::cerr << "\toptimize" << std::endl;
  std::cerr << "\texec" << std::endl;
  std::cerr << "\tdump-x86" << std::endl;
}

#define YADFA_ENABLE_TESTS 1

extern "C" {
void builtin_writeln(int32_t a) { printf("\n%d", a); }
void builtin_write(int32_t a) { printf("%d", a); }

void builtin_print(int32_t a, int32_t b, int32_t c, int32_t d, int32_t e,
                   int32_t f, int32_t g, int32_t h) {
  printf("\nprint:%d, %d, %d, %d, %d, %d, %d, %d\n", a, b, c, d, e, f, g, h);
}
}

int main(int argc, char* argv[]) {
  builtin_functions_map builtin_functions;
  builtin_function builtin_print_def{(void *)builtin_print,
                                     {type_int32, type_int32, type_int32,
                                      type_int32, type_int32, type_int32,
                                      type_int32, type_int32}};
  builtin_function builtin_writeln_def{(void *)builtin_writeln, {type_int32}};
  builtin_function builtin_write_def{(void *)builtin_write, {type_int32}};
  builtin_functions.insert({"print", builtin_print_def});
  builtin_functions.insert({"write", builtin_write_def});
  builtin_functions.insert({"writeln", builtin_writeln_def});
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
  } else if (command == "--dot-cfg") {
    if (argc < 3) {
      usage();
      return -1;
    }
    auto program = parse(argv[2], table);
    auto cfg = build_cfg(program, table);
    gen_set output_gen_set;
    kill_set output_kill_set;
    build_use_def_sets(program, output_gen_set, output_kill_set);
    auto liveness_sets = liveness_analysis(program, cfg);
    dump_cfg_to_dot(program, cfg, output_gen_set, output_kill_set, liveness_sets, std::cout);
  } else if (command == "--analysis") {
    if (argc < 4) {
      usage();
      return -1;
    }
    auto type_of_analysis = argv[2];
    auto program = parse(argv[3], table);
    auto cfg = build_cfg(program, table);
    auto liveness_sets = liveness_analysis(program, cfg);
    dump_raw_liveness(liveness_sets, std::cout);
    auto variable_intervals = compute_variables_live_ranges(liveness_sets);
    dump_variable_intervals(variable_intervals, std::cout);
    generate_gnuplot_interval(variable_intervals);
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
  } else if (command == "--optimize") {
    if (argc < 3) {
      usage();
      return -1;
    }
    auto program = parse(argv[2], table);
    auto cfg = build_cfg(program, table);
    auto liveness_sets = liveness_analysis(program, cfg);
    auto variable_intervals = compute_variables_live_ranges(liveness_sets);
    auto optimized_program = optimize(program, variable_intervals);
    dump_program(optimized_program, std::cout);
  } else if (command == "--exec") {
    auto program = parse(argv[2], table);
    exec(program, table, builtin_functions);
  } else if (command == "--dump-x86") {
    auto program = parse(argv[2], table);
    dump_x86_64(program, table, builtin_functions);
  } else {
    usage();
    return -1;
  }
  return 0;
}
