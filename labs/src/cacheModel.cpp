#include <cstdio>
#include <cmath>
#include <fstream>
#include <ctime>
#include <vector>
#include <string>
#include <iostream>
#include "debug_macros.h"
#include "pin.H"

using namespace std;

typedef unsigned int UINT32;
typedef unsigned long int UINT64;


#define PAGE_SIZE_LOG       12
#define PHY_MEM_SIZE_LOG    30

#define get_vir_page_no(virtual_addr)   (virtual_addr >> PAGE_SIZE_LOG)
#define get_page_offset(addr)           (addr & ((1u << PAGE_SIZE_LOG) - 1))

// Obtain physical page number according to a given virtual page number
UINT32 get_phy_page_no(UINT32 virtual_page_no) {
  UINT32 vpn = virtual_page_no;
  vpn = (~vpn ^ (vpn << 16)) + (vpn & (vpn << 16)) + (~vpn | (vpn << 2));

  UINT32 mask = (UINT32) (~0) << (32 - PHY_MEM_SIZE_LOG);
  mask = mask >> (32 - PHY_MEM_SIZE_LOG + PAGE_SIZE_LOG);
  mask = mask << PAGE_SIZE_LOG;

  return vpn & mask;
}

// Transform a virtual address into a physical address
UINT32 get_phy_addr(UINT32 virtual_addr) {
  return (get_phy_page_no(get_vir_page_no(virtual_addr)) << PAGE_SIZE_LOG) + get_page_offset(virtual_addr);
}

/**
 * Cache Model Base Class
 */
class CacheModel {
protected:
  UINT32 m_block_num;     // The number of cache blocks
  UINT32 m_blksz_log;     // 块大小的对数

  UINT64 m_rd_reqs;       // The number of read-requests
  UINT64 m_wr_reqs;       // The number of write-requests
  UINT64 m_rd_hits;       // The number of hit read-requests
  UINT64 m_wr_hits;       // The number of hit write-requests

  string name = "Basic model";
public:
  // Constructor
  CacheModel(UINT32 block_num, UINT32 log_block_size)
      : m_block_num(block_num), m_blksz_log(log_block_size),
        m_rd_reqs(0), m_wr_reqs(0), m_rd_hits(0), m_wr_hits(0) {
  }

  virtual ~CacheModel() = default;

  // Update the cache state whenever data is read
  void readReq(UINT32 mem_addr) {
    m_rd_reqs++;
    // Dbg("R [%6lu] %08x", m_rd_reqs, mem_addr);
    if (access(mem_addr)) m_rd_hits++;
  }

  // Update the cache state whenever data is written
  void writeReq(UINT32 mem_addr) {
    m_wr_reqs++;
    // Dbg("W [%6lu] %08x", m_wr_reqs, mem_addr);
    if (access(mem_addr)) m_wr_hits++;
  }

  [[nodiscard]] UINT32 getRdReq() const { return m_rd_reqs; }

  [[nodiscard]] UINT32 getWrReq() const { return m_wr_reqs; }

  void dumpResults() const {
    float rdHitRate = 100 * (float) m_rd_hits / (float) m_rd_reqs;
    float wrHitRate = 100 * (float) m_wr_hits / (float) m_wr_reqs;
    printf("\t read req: %lu,\thit: %lu,\thit rate: %.2f%%\n", m_rd_reqs, m_rd_hits, rdHitRate);
    printf("\twrite req: %lu,\thit: %lu,\thit rate: %.2f%%\n", m_wr_reqs, m_wr_hits, wrHitRate);
  }

  void statistics(ofstream &file) {
    file << "Ok";
  }

protected:
  // Look up the cache to decide whether the access is hit or missed
  virtual bool lookup(UINT32 mem_addr, UINT32 &blk_id) = 0;

  // Access the cache: update m_replace_q if hit, otherwise replace a block and update m_replace_q
  virtual bool access(UINT32 mem_addr) = 0;

  // Update m_replace_q
  virtual void updateReplaceQ(UINT32 blk_id) = 0;
};

/**
 * Basic cache data storage
 */
class LinearCache : public CacheModel {
public:
  bool *m_valids;
  UINT32 *m_tags;
  vector<UINT32> m_replace_q;    // Cache块替换的候选队列
  LinearCache(UINT32 block_num, UINT32 log_block_size) : CacheModel(block_num, log_block_size) {
    m_valids = new bool[m_block_num];
    m_tags = new UINT32[m_block_num];

    for (UINT i = 0; i < m_block_num; i++) {
      m_valids[i] = false;
      m_replace_q.emplace_back(i);
    }
  }

  ~LinearCache() override {
    delete[] m_valids;
    delete[] m_tags;
  }

  UINT32 getTag(UINT32 addr) {
    return (addr >> m_blksz_log) & ((1 << m_block_num) - 1);
  }

protected:

  bool lookup(UINT32 mem_addr, UINT32 &blk_id) override {
    return m_valids[getTag(mem_addr)];
  }

  bool access(UINT32 mem_addr) override {
    return false;
  }

  void updateReplaceQ(UINT32 blk_id) override {
    // do nothing
  }
};

using DirectMappingCache = LinearCache;

/**
 * Fully Associative Cache Class
 */
class FullAssoCache : public CacheModel {
public:
  LinearCache inner;

  // Constructor
  FullAssoCache(UINT32 block_num, UINT32 log_block_size)
      : inner(LinearCache(block_num, log_block_size)),
        CacheModel(block_num, log_block_size) {
  }

private:
  // Look up the cache to decide whether the access is hit or missed
  bool lookup(UINT32 mem_addr, UINT32 &blk_id) override {
    UINT32 tag = inner.getTag(mem_addr);
    for (int i = 0; i < m_block_num; i++) {
      if (inner.m_valids[i] && inner.m_tags[i] == tag) {
        blk_id = i;
        return true;
      }
    }
    return false;
  }

  // Access the cache: update m_replace_q if hit, otherwise replace a block and update m_replace_q
  bool access(UINT32 mem_addr) override {
    UINT32 blk_id;
    if (lookup(mem_addr, blk_id)) {
      updateReplaceQ(blk_id);     // Update m_replace_q
      return true;
    }

    // Get the to-be-replaced block id using m_replace_q
    UINT32 bid_2be_replaced = inner.m_replace_q[0];

    // Replace the cache block..?
    // TODO
    updateReplaceQ(bid_2be_replaced);

    return false;
  }

  // Update m_replace_q
  void updateReplaceQ(UINT32 blk_id) override {
    // insert to head, and shift other indexes
    auto index = find(inner.m_replace_q.begin(), inner.m_replace_q.end(), blk_id);
    if (index == inner.m_replace_q.end()) {
      printf("cannot find block %d!!", blk_id);
    }
    inner.m_replace_q.erase(index);
    inner.m_replace_q.insert(inner.m_replace_q.begin(), blk_id);
  }
};

/**
 * Set-Associative Cache Class
 */
class SetAssoCache : public CacheModel {
public:
  UINT32 m_sets_log;
  // total asso sets
  UINT32 m_asso;
  vector<LinearCache> sets;

  // Constructor
  SetAssoCache(UINT32 sets_log, UINT32 log_block_size, UINT32 asso) :
      m_sets_log(sets_log), m_asso(asso),
      CacheModel(asso, log_block_size) {
    Dbg("SetAssoCache(%u, %u, %u)", sets_log, log_block_size, asso);
    for (auto i = 0; i < m_asso; i++) {
      Dbg("creating set %d", i);
      sets.emplace_back(LinearCache(1 << m_sets_log, log_block_size));
    }
  }

protected:

  // addr: [ tag | group index | set index | block offset ]

  UINT32 getTag(UINT32 addr) {
    return addr >> (m_blksz_log + m_asso + m_sets_log);
  }

  UINT32 getGroupIndex(UINT32 addr) {
    return (addr >> (m_blksz_log + m_asso)) & ((1 << m_sets_log) - 1);
  }

  UINT32 getSetIndex(UINT32 addr) {
    return (addr >> m_blksz_log) & ((1 << m_asso) - 1);
  }

private:
  // Look up the cache to decide whether the access is hit or missed
  bool lookup(UINT32 mem_addr, UINT32 &blk_id) override {
    auto tag = getTag(mem_addr);
    auto index_group = getGroupIndex(mem_addr);
    auto index_set = getSetIndex(mem_addr);
    auto &set = sets[index_group];
    return set.m_valids[index_set] && set.m_tags[index_set];
  }

  // Access the cache: update m_replace_q if hit, otherwise replace a block and update m_replace_q
  bool access(UINT32 mem_addr) override {
    // TODO
    return false;
  }

  // Update m_replace_q
  void updateReplaceQ(UINT32 blk_id) override {
    // TODO
  }
};

/**
 * Set-Associative Cache Class (VIVT)
 */
class SetAssoCache_VIVT : public SetAssoCache {
public:
  SetAssoCache_VIVT(UINT32 setsLog, UINT32 logBlockSize, UINT32 asso) : SetAssoCache(setsLog, logBlockSize, asso) {}

private:
};

/**
 * Set-Associative Cache Class (PIPT)
 */
class SetAssoCache_PIPT : public SetAssoCache {
public:
  SetAssoCache_PIPT(UINT32 setsLog, UINT32 logBlockSize, UINT32 asso) : SetAssoCache(setsLog, logBlockSize, asso) {}

private:
};

/**
 * Set-Associative Cache Class (VIPT)
 */
class SetAssoCache_VIPT : public SetAssoCache {
public:
  SetAssoCache_VIPT(UINT32 setsLog, UINT32 logBlockSize, UINT32 asso) : SetAssoCache(setsLog, logBlockSize, asso) {}

private:
};

vector<CacheModel *> models;

// Cache reading analysis routine
void readCache(UINT32 mem_addr) {
  mem_addr = (mem_addr >> 2) << 2;
  for (auto &model: models) {
    model->readReq(mem_addr);
  }
}

// Cache writing analysis routine
void writeCache(UINT32 mem_addr) {
  mem_addr = (mem_addr >> 2) << 2;
  for (auto &model: models) {
    model->writeReq(mem_addr);
  }
}

// This knob will set the cache param m_block_num
KNOB<UINT32> KnobBlockNum(KNOB_MODE_WRITEONCE, "pintool",
                          "n", "512", "specify the number of blocks in bytes");

// This knob will set the cache param m_blksz_log
KNOB<UINT32> KnobBlockSizeLog(KNOB_MODE_WRITEONCE, "pintool",
                              "b", "6", "specify the log of the block size in bytes");

// This knob will set the cache param m_sets_log
KNOB<UINT32> KnobSetsLog(KNOB_MODE_WRITEONCE, "pintool",
                         "r", "7", "specify the log of the number of rows");

// This knob will set the cache param m_asso
KNOB<UINT32> KnobAssociativity(KNOB_MODE_WRITEONCE, "pintool",
                               "a", "4", "specify the m_asso");
// This knob sets the output file name
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "cacheModels.txt", "specify the output file name");

// Pin calls this function every time a new instruction is encountered
VOID Instruction(INS ins, VOID *v) {
  if (INS_IsMemoryRead(ins))
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) readCache, IARG_MEMORYREAD_EA, IARG_END);
  if (INS_IsMemoryWrite(ins))
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) writeCache, IARG_MEMORYWRITE_EA, IARG_END);
}

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v) {
  Dbg("All finished.");
  ofstream output;
  output.setf(ios::showbase);
  for (auto &model: models) {
    model->statistics(output);
    delete model;
  }
}

INT32 Usage() {
  cerr << "This tool tests multiple models of caching" << endl;
  cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
  return -1;
}

FILE *log_fp = nullptr;

// argc, argv are the entire command line, including pin -t <toolname> -- ...
int main(int argc, char *argv[]) {
  // Initialize pin
  if (PIN_Init(argc, argv)) return Usage();

  log_fp = fopen(KnobOutputFile.Value().c_str(), "w");

  Log("Cache Model Test Program");

  models.emplace_back(new FullAssoCache(KnobBlockNum.Value(), KnobBlockSizeLog.Value()));
  Dbg("init done: FullAssoCache");
//
//  models.emplace_back(new SetAssoCache(KnobSetsLog.Value(), KnobBlockSizeLog.Value(), KnobAssociativity.Value()));
//  Dbg("init done: SetAssoCache");

  // models.emplace_back(new SetAssoCache_VIVT(KnobSetsLog.Value(), KnobBlockSizeLog.Value(), KnobAssociativity.Value()));
  // Dbg("init done: SetAssoCache_VIVT");
  // models.emplace_back(new SetAssoCache_PIPT(KnobSetsLog.Value(), KnobBlockSizeLog.Value(), KnobAssociativity.Value()));
  // Dbg("init done: SetAssoCache_PIPT");
  // models.emplace_back(new SetAssoCache_VIPT(KnobSetsLog.Value(), KnobBlockSizeLog.Value(), KnobAssociativity.Value()));
  // Dbg("init done: SetAssoCache_VIPT");

  Dbg("%lu models init done", models.size());

  // Register Instruction to be called to instrument instructions
  INS_AddInstrumentFunction(Instruction, nullptr);

  // Register Fini to be called when the application exits
  PIN_AddFiniFunction(Fini, nullptr);

  // Start the program, never returns
  PIN_StartProgram();

  return 0;
}
