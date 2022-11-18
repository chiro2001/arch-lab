// #include "bridge.h"
#include "rusty_bridge/lib.h"
#include <iostream>
#include <fstream>
#include <string>
#include "pin.H"
#include <dlfcn.h>

using namespace std;

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long int UINT64;
typedef unsigned __int128 UINT128;

#define METHOD_COUNT_MAX 4

// 将val截断, 使其宽度变成bits
#define truncate(val, bits) ((val) & ((1 << (bits)) - 1))

// This knob sets the output file name
// KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "bp.txt", "specify the output file name");
// KnobOutputFile.Value().c_str()

// This function is called when the application exits
VOID Fini(int, VOID *v) {
  // rust_finish();
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage() {
  cerr << "This tool counts the number of dynamic instructions executed" << endl;
  cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
  return -1;
}

void Instruction(INS ins, void *v);

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */
/*   argc, argv are the entire command line: pin -t <toolname> -- ...    */
/* ===================================================================== */

int main(int argc, char *argv[]) {
  // Initialize pin
  if (PIN_Init(argc, argv)) return Usage();

  // rust_start();
  std::cout << "rusty_cxxbridge_integer = " << rusty_cxxbridge_integer() << std::endl;

  // Register Instruction to be called to instrument instructions
  INS_AddInstrumentFunction(Instruction, nullptr);

  // Register Fini to be called when the application exits
  PIN_AddFiniFunction(Fini, nullptr);

  // Start the program, never returns
  PIN_StartProgram();

  return 0;
}

// This function is called every time a control-flow instruction is encountered
void predictBranch(ADDRINT pc, BOOL direction) {
  // predict_branch(pc, direction);
}

// Pin calls this function every time a new instruction is encountered
void Instruction(INS ins, void *v) {
  if (INS_IsControlFlow(ins) && INS_HasFallThrough(ins)) {
    // Insert a call to the branch target
    INS_InsertCall(ins, IPOINT_TAKEN_BRANCH, (AFUNPTR) predictBranch,
                   IARG_INST_PTR, IARG_BOOL, TRUE, IARG_END);

    // Insert a call to the next instruction of a branch
    INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR) predictBranch,
                   IARG_INST_PTR, IARG_BOOL, FALSE, IARG_END);
  }
}