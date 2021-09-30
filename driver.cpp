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
    exec(program, table);
  } else if (command == "--dump-x86") {
    auto program = parse(argv[2], table);
    dump_x86_64(program, table);
  } else {
    usage();
    return -1;
  }
  return 0;
}
