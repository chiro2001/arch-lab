#include "bridge.h"
#include <iostream>
#include <fstream>
#include <string>
#include "pin.H"

using namespace std;

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long int UINT64;
typedef unsigned __int128 UINT128;

#define METHOD_COUNT_MAX 4

ofstream OutFile;

// 将val截断, 使其宽度变成bits
#define truncate(val, bits) ((val) & ((1 << (bits)) - 1))

class PredictionResult {
public:
  UINT64 takenCorrect = 0;
  UINT64 takenIncorrect = 0;
  UINT64 notTakenCorrect = 0;
  UINT64 notTakenIncorrect = 0;
  BOOL used = false;
  string name = "unknown";
};

static PredictionResult results[METHOD_COUNT_MAX] = {};

// static UINT64 takenCorrect = 0;
// static UINT64 takenIncorrect = 0;
// static UINT64 notTakenCorrect = 0;
// static UINT64 notTakenIncorrect = 0;

// This knob sets the output file name
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "brchPredict.txt", "specify the output file name");

// This function is called when the application exits
VOID Fini(int, VOID *v) {
  for (int i = 0; i < METHOD_COUNT_MAX; i++) {
    if (results[i].used) {
      double precision = 100 * double(results[i].takenCorrect + results[i].notTakenCorrect) /
                         (results[i].takenCorrect + results[i].notTakenCorrect + results[i].takenIncorrect +
                          results[i].notTakenIncorrect);

      cout << "name: " << results[i].name << endl
           << "takenCorrect: " << results[i].takenCorrect << endl
           << "takenIncorrect: " << results[i].takenIncorrect << endl
           << "notTakenCorrect: " << results[i].notTakenCorrect << endl
           << "nnotTakenIncorrect: " << results[i].notTakenIncorrect << endl
           << "Precision: " << precision << endl;

      OutFile.setf(ios::showbase);
      OutFile << "name: " << results[i].name << endl
              << "takenCorrect: " << results[i].takenCorrect << endl
              << "takenIncorrect: " << results[i].takenIncorrect << endl
              << "notTakenCorrect: " << results[i].notTakenCorrect << endl
              << "nnotTakenIncorrect: " << results[i].notTakenIncorrect << endl
              << "Precision: " << precision << endl;
    }
  }

  OutFile.close();
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
  std::cout << "A value given via generated cxxbridge "
            << rusty_cxxbridge_integer() << "\n";
  // TODO: New your Predictor below.
  // BP = new BranchPredictor();

  // Initialize pin
  if (PIN_Init(argc, argv)) return Usage();

  OutFile.open(KnobOutputFile.Value().c_str());

  // Register Instruction to be called to instrument instructions
  INS_AddInstrumentFunction(Instruction, nullptr);

  // Register Fini to be called when the application exits
  PIN_AddFiniFunction(Fini, nullptr);

  // Start the program, never returns
  PIN_StartProgram();

  return 0;
}

/**
 * Predict for one method
 * @param method method index
 * @param pc PC now
 * @param direction before / after
 * @return 0: not take, other value: jump PC
 */
UINT64 predictBranchMethod(int method, ADDRINT pc, BOOL direction) {
  // TODO
  return 0;
}

// This function is called every time a control-flow instruction is encountered
void predictBranch(ADDRINT pc, BOOL direction) {
  for (int method = 0; method < METHOD_COUNT_MAX; method++) {
    UINT64 prediction = predictBranchMethod(method, pc, direction);
    BOOL take = prediction != 0;
    if (take) {
      if (direction)
        results[method].takenCorrect++;
      else
        results[method].takenIncorrect++;
    } else {
      if (direction)
        results[method].notTakenIncorrect++;
      else
        results[method].notTakenCorrect++;
    }
  }
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