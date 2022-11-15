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

// This function is called before every block
// Use the fast linkage for calls
VOID PIN_FAST_ANALYSIS_CALL docount(void* insts_ptr) {
  // printf("insts_ptr : %p\n", insts_ptr);
  if (!insts_ptr) return;
  vector<Registers*>* insts = (vector<Registers*>*)(insts_ptr);
  for (auto regs : *insts) {
    if (regs) ++insPointer;
    for (vector<reg_t>::iterator it = regs->read.begin();
         it != regs->read.end(); it++) {
      // 当前读寄存器
      reg_t reg = *it;

      if (lastInsPointer[reg] > 0) {
        // 有访问过这个寄存器
        // 当前指令号 - 上次访问指令号 = 寄存器依赖距离
        INT32 distance = insPointer - lastInsPointer[reg];
        // 统计当前依赖距离出现次数
        if (distance <= maxSize) insDependDistance[distance]++;
      }
    }

    // 更新写入寄存器的 lastInstructionCount
    for (vector<reg_t>::iterator it = regs->write.begin();
         it != regs->write.end(); it++)
      lastInsPointer[*it] = insPointer;
  }
}

// Pin calls this function every time a new basic block is encountered
// It inserts a call to docount
VOID Trace(TRACE trace, VOID* v) {
  fflush(stdout);
  // Visit every basic block in the trace
  for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
    vector<Registers*>* insts = new vector<Registers*>();
    // printf("insts @ %p\n", insts);
    for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins)) {
      // regs stores the registers read, written by this instruction
      // regs 存储由该指令读取、写入的寄存器
      Registers* regs = new Registers();

      // Find all the register written
      // 查找所有写入的寄存器
      for (uint32_t iw = 0; iw < INS_MaxNumWRegs(ins); iw++) {
        // 获取当前指令中被写的寄存器(即目的寄存器)
        REG wr = INS_RegW(ins, iw);
        // 获取寄存器名
        wr = REG_FullRegName(wr);
        if (!REG_valid(wr)) continue;

        // 将被写寄存器保存到regs->write当中
        if (std::find(regs->write.begin(), regs->write.end(), wr) ==
            regs->write.end())
          regs->write.push_back(wr);
      }

      // Find all the registers read
      // 查找所有读取的寄存器
      for (uint32_t ir = 0; ir < INS_MaxNumRRegs(ins); ir++) {
        // 获取当前指令中被写的寄存器(即目的寄存器)
        REG rr = INS_RegR(ins, ir);
        // 获取寄存器名
        rr = REG_FullRegName(rr);
        if (!REG_valid(rr)) continue;
        // 将被读寄存器保存到regs->read当中
        if (std::find(regs->read.begin(), regs->read.end(), rr) ==
            regs->read.end())
          regs->read.push_back(rr);
      }
      insts->push_back(regs);
    }
    BBL_InsertCall(bbl, IPOINT_BEFORE, AFUNPTR(docount), IARG_PTR, insts,
                   IARG_END);
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
VOID Fini(INT32 code, VOID* v) {
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
  // Initialize pin
  if (PIN_Init(argc, argv)) return Usage();

  OutFile.open(KnobOutputFile.Value().c_str());
  maxSize = atoi(KnobMaxDistance.Value().c_str());

  // Initializing depdendancy Distance
  insDependDistance = new UINT64[maxSize];
  memset((void*)insDependDistance, 0, sizeof(UINT64) * maxSize);

  // Register Instruction to be called to instrument instructions
  TRACE_AddInstrumentFunction(Trace, 0);

  // Register Fini to be called when the application exits
  PIN_AddFiniFunction(Fini, 0);

  // Start the program, never returns
  PIN_StartProgram();

  return 0;
}
