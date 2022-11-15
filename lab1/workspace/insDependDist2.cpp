/*
 * Copyright (C) 2004-2021 Intel Corporation.
 * SPDX-License-Identifier: MIT
 */

#include <fstream>
#include <iostream>
#include <vector>

#include "pin.H"
using std::cerr;
using std::endl;
using std::ios;
using std::ofstream;
using std::string;
using std::vector;

ofstream OutFile;

typedef uint32_t reg_t;
struct Registers {
  vector<reg_t> read;
  vector<reg_t> write;
};

UINT64* insDependDistance;
INT32 maxSize;
INT32 insPointer = 0;
INT32 lastInsPointer[1024] = {0};

// The running count of instructions is kept here
// make it static to help the compiler optimize docount
static UINT64 icount = 0;

// This function is called before every block
// Use the fast linkage for calls
VOID PIN_FAST_ANALYSIS_CALL docount(void* p) {
  BBL bbl = *((BBL*)p);
  for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins)) {
    icount++;
  }
}

// Pin calls this function every time a new basic block is encountered
// It inserts a call to docount
VOID Trace(TRACE trace, VOID* v) {
  printf("Trace!");
  fflush(stdout);
  // Visit every basic block in the trace
  for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
    BBL_InsertCall(bbl, IPOINT_BEFORE, AFUNPTR(docount),
                   IARG_PTR, &bbl, IARG_END);
  }
}

// This knob sets the output file name
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o",
                            "insDependDist2.csv",
                            "specify the output file name");

// This knob will set the maximum distance between two dependant instructions in
// the program
KNOB<string> KnobMaxDistance(KNOB_MODE_WRITEONCE, "pintool", "s", "100",
                             "specify the maximum distance between two "
                             "dependant instructions in the program");

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v) {
  printf("Fini!");
  // Write to a file since cout and cerr maybe closed by the application
  OutFile.setf(ios::showbase);
  for (INT32 i = 0; i < maxSize; i++) OutFile << insDependDistance[i] << ",";
  OutFile.close();
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage() {
  cerr << "This tool counts the number of dynamic instructions executed"
       << endl;
  cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
  return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char* argv[]) {
  printf("main!");
  // Initialize pin
  if (PIN_Init(argc, argv)) return Usage();

  OutFile.open(KnobOutputFile.Value().c_str());
  maxSize = atoi(KnobMaxDistance.Value().c_str());

  // Initializing depdendancy Distance
  insDependDistance = new UINT64[maxSize];
  memset((void *)insDependDistance, 0, sizeof(UINT64) * maxSize);

  // Register Instruction to be called to instrument instructions
  TRACE_AddInstrumentFunction(Trace, 0);

  // Register Fini to be called when the application exits
  PIN_AddFiniFunction(Fini, 0);

  // Start the program, never returns
  PIN_StartProgram();

  return 0;
}
