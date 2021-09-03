cmake_minimum_required(VERSION 3.17)
project(yadfa)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(yadfa
        ./asmjit/x86/x86instapi.cpp
        ./asmjit/x86/x86assembler.cpp
        ./asmjit/x86/x86instdb.cpp
        ./asmjit/x86/x86func.cpp
        ./asmjit/x86/x86emithelper.cpp
        ./asmjit/x86/x86builder.cpp
        ./asmjit/x86/x86compiler.cpp
        ./asmjit/x86/x86operand.cpp
        ./asmjit/x86/x86features.cpp
        ./asmjit/x86/x86rapass.cpp
        ./asmjit/x86/x86formatter.cpp
        ./asmjit/core/formatter.cpp
        ./asmjit/core/func.cpp
        ./asmjit/core/globals.cpp
        ./asmjit/core/zonestack.cpp
        ./asmjit/core/emitterutils.cpp
        ./asmjit/core/cpuinfo.cpp
        ./asmjit/core/codewriter.cpp
        ./asmjit/core/assembler.cpp
        ./asmjit/core/zonehash.cpp
        ./asmjit/core/operand.cpp
        ./asmjit/core/zone.cpp
        ./asmjit/core/rapass.cpp
        ./asmjit/core/emitter.cpp
        ./asmjit/core/funcargscontext.cpp
        ./asmjit/core/logger.cpp
        ./asmjit/core/zonevector.cpp
        ./asmjit/core/jitallocator.cpp
        ./asmjit/core/constpool.cpp
        ./asmjit/core/virtmem.cpp
        ./asmjit/core/zonetree.cpp
        ./asmjit/core/codeholder.cpp
        ./asmjit/core/string.cpp
        ./asmjit/core/inst.cpp
        ./asmjit/core/builder.cpp
        ./asmjit/core/type.cpp
        ./asmjit/core/compiler.cpp
        ./asmjit/core/support.cpp
        ./asmjit/core/rastack.cpp
        ./asmjit/core/ralocal.cpp
        ./asmjit/core/errorhandler.cpp
        ./asmjit/core/osutils.cpp
        ./asmjit/core/zonelist.cpp
        ./asmjit/core/archtraits.cpp
        ./asmjit/core/emithelper.cpp
        ./asmjit/core/target.cpp
        ./asmjit/core/jitruntime.cpp
        ./asmjit/core/environment.cpp

        yadfa.cpp
        tests.cpp
        driver.cpp)

target_link_libraries(yadfa rt)