#include <cstdio>
#include <cmath>
#include <fstream>
#include <ctime>
#include <utility>
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

class LinkedLRU {
  vector<UINT32> replace_queue;
public:
  explicit LinkedLRU(size_t count) {
    for (int i = 0; i < count; i++)
      replace_queue.emplace_back(i);
  }

  UINT32 front() {
    return *replace_queue.begin();
  }

  void update(UINT32 index) {
    // insert to back, and shift other indexes
    auto p = find(replace_queue.begin(), replace_queue.end(), index);
    if (p == replace_queue.end()) {
      Err("cannot find block %d!!", index);
    }
    replace_queue.erase(p);
    replace_queue.emplace_back(index);
  }

  size_t capacity() {
    return replace_queue.size() * 32;
  }
};

class ReplaceAlgo {
protected:
  size_t total;
public:
  explicit ReplaceAlgo(size_t total) : total(total) {}

  virtual ~ReplaceAlgo() {}

  virtual size_t select(bool update) = 0;

  virtual size_t capacity() = 0;
};

class LRURepl : public ReplaceAlgo {
  LinkedLRU lru;
public:
  explicit LRURepl(size_t total) :
      lru(total),
      ReplaceAlgo(total) {}

  size_t select(bool update) override {
    if (update) {
      auto f = lru.front();
      lru.update(f);
      return f;
    } else
      return lru.front();
  }

  size_t capacity() override {
    auto s = total;
    size_t l = 0;
    while (s) {
      s >>= 1;
      l++;
    }
    if (l > 0) l--;
    return l;
  }
};

class RandomRepl : public ReplaceAlgo {
public:
  explicit RandomRepl(size_t total) : ReplaceAlgo(total) {}

  size_t select(bool update) override {
    return rand() % total;
  }

  size_t capacity() override {
    return 0;
  }
};

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

public:
  string name;

  // Constructor
  CacheModel(UINT32 block_num, UINT32 log_block_size, string name = "Basic model")
      : m_block_num(block_num), m_blksz_log(log_block_size),
        m_rd_reqs(0), m_wr_reqs(0), m_rd_hits(0), m_wr_hits(0), name(std::move(name)) {
  }

  virtual ~CacheModel() = default;

  virtual size_t capacity() = 0;

  void setName(const string &name_) {
    this->name = name_;
  }

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

  float statistics() {
    float hitRate = 100 * (float) (m_rd_hits + m_wr_hits) / (float) (m_wr_reqs + m_rd_reqs);
    float rdHitRate = 100 * (float) m_rd_hits / (float) m_rd_reqs;
    float wrHitRate = 100 * (float) m_wr_hits / (float) m_wr_reqs;
    log_write("model: %s, %.4f%%, %.4f KiB\n", name.c_str(), hitRate, (float) capacity() / 8 / 0x400);
    log_write("\t read req: %lu,\thit: %lu,\thit rate: %.4f%%\n", m_rd_reqs, m_rd_hits, rdHitRate);
    log_write("\twrite req: %lu,\thit: %lu,\thit rate: %.4f%%\n", m_wr_reqs, m_wr_hits, wrHitRate);
    return hitRate;
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

  LinearCache(UINT32 block_num, UINT32 log_block_size, string name = "DirectMappingCache") :
      CacheModel(block_num, log_block_size, std::move(name)) {
    Dbg("%s(0x%x, %d)", this->name.c_str(), block_num, log_block_size);
    m_valids = new bool[m_block_num];
    m_tags = new UINT32[m_block_num];

    for (UINT i = 0; i < m_block_num; i++) {
      m_valids[i] = false;
    }
  }

  ~LinearCache() override {
    delete[] m_valids;
    delete[] m_tags;
  }

  UINT32 getTag(UINT32 addr) {
    return addr >> m_blksz_log;
  }

  UINT32 getIndex(UINT32 addr) {
    return getTag(addr) & (m_block_num - 1);
  }

  size_t capacity() override {
    return ((32 - m_blksz_log) + 1 + (1 << (m_blksz_log + 3))) * m_block_num;
  }

protected:

  bool lookup(UINT32 mem_addr, UINT32 &blk_id) override {
    auto tag = getTag(mem_addr);
    auto index = getIndex(mem_addr);
    return m_valids[index] && m_tags[index] == tag;
  }

  bool access(UINT32 mem_addr) override {
    UINT32 blk_id = 0;
    if (lookup(mem_addr, blk_id)) {
      return true;
    }
    auto tag = getTag(mem_addr);
    auto index = getIndex(mem_addr);
    m_valids[index] = true;
    m_tags[index] = tag;
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
  LinkedLRU lru;

  // Constructor
  FullAssoCache(UINT32 block_num, UINT32 log_block_size)
      : inner(LinearCache(block_num, log_block_size)), lru(LinkedLRU(block_num)),
        CacheModel(block_num, log_block_size, "FullAssoCache") {
  }

  size_t capacity() override {
    return lru.capacity() + inner.capacity();
  }

private:
  UINT32 getTag(UINT32 addr) {
    return addr >> m_blksz_log;
  }

  // Look up the cache to decide whether the access is hit or missed
  bool lookup(UINT32 mem_addr, UINT32 &blk_id) override {
    UINT32 tag = getTag(mem_addr);
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
    UINT32 bid_2be_replaced = lru.front();

    // Replace the cache block...?
    inner.m_valids[bid_2be_replaced] = true;
    inner.m_tags[bid_2be_replaced] = getTag(mem_addr);
    updateReplaceQ(bid_2be_replaced);

    return false;
  }

  // Update m_replace_q
  void updateReplaceQ(UINT32 blk_id) override {
    lru.update(blk_id);
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
  vector<LinearCache *> sets;
  ReplaceAlgo **replace;
  bool phy_index;
  bool phy_tag;

  // Constructor
  SetAssoCache(UINT32 sets_log, UINT32 log_block_size, UINT32 asso, string name = "SetAssoCache",
               bool phy_index = false, bool phy_tag = false) :
      m_sets_log(sets_log), m_asso(asso),
      replace(nullptr),
      phy_index(phy_index),
      phy_tag(phy_tag),
      CacheModel(asso, log_block_size, std::move(name)) {
    Dbg("SetAssoCache(%u, %u, %u)", sets_log, log_block_size, asso);
    for (auto i = 0; i < m_asso; i++) {
      // Dbg("creating set %d", i);
      sets.emplace_back(
          new LinearCache(1 << m_sets_log, log_block_size, string("SetAssoCache-Set-") + std::to_string(i)));
    }
    // Dbg("init done");
  }

  ~SetAssoCache() override {
    for (auto set: sets) {
      delete set;
    }
    for (int i = 0; i < 1 << m_sets_log; i++)
      delete replace[i];
    delete replace;
  }

  size_t capacity() override {
    size_t s = 0;
    for (auto &set: sets) s += set->capacity();
    for (int i = 0; i < 1 << m_sets_log; i++) replace[i]->capacity();
    return s;
  }

  template<typename F>
  void setReplace(F const &f) {
    if (!this->replace) this->replace = new ReplaceAlgo *[1 << m_sets_log];
    for (int i = 0; i < 1 << m_sets_log; i++)
      this->replace[i] = f(m_asso);
  }

protected:

  // addr: [ tag | set index | block offset ]

  UINT32 getTag(UINT32 addr) {
    return addr >> (m_blksz_log + m_asso + m_sets_log);
  }

  UINT32 getSetIndex(UINT32 addr) {
    return (addr >> m_blksz_log) & ((1 << m_asso) - 1);
  }

private:
  // Look up the cache to decide whether the access is hit or missed
  bool lookup(UINT32 mem_addr, UINT32 &blk_id) override {
    auto tag = getTag(phy_tag ? get_phy_addr(mem_addr) : mem_addr);
    auto index_set = getSetIndex(phy_index ? get_phy_addr(mem_addr) : mem_addr);
    for (int i = 0; i < sets.size(); i++) {
      auto &set = sets[i];
      auto r = set->m_valids[index_set] && set->m_tags[index_set] == tag;
      blk_id = i;
      if (r) return true;
    }
    return false;
  }

  // Access the cache: update m_replace_q if hit, otherwise replace a block and update m_replace_q
  bool access(UINT32 mem_addr) override {
    UINT32 blk_id = 0;
    if (lookup(mem_addr, blk_id)) {
      return true;
    }
    auto tag = getTag(phy_tag ? get_phy_addr(mem_addr) : mem_addr);
    auto index_set = getSetIndex(phy_index ? get_phy_addr(mem_addr) : mem_addr);
    LinearCache *empty_set = nullptr;
    for (auto &set: sets) {
      if (!set->m_valids[index_set]) {
        empty_set = set;
        break;
      }
    }
    if (empty_set != nullptr) {
      empty_set->m_tags[index_set] = tag;
      empty_set->m_valids[index_set] = true;
    } else {
      // kick out one block
      auto select = replace ? replace[index_set]->select(true) : (rand() % m_asso);
      auto set = sets[select];
      set->m_tags[index_set] = tag;
      set->m_valids[index_set] = true;
    }
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
class SetAsso_VIVT : public SetAssoCache {
public:
  SetAsso_VIVT(UINT32 setsLog, UINT32 logBlockSize, UINT32 asso) :
      SetAssoCache(setsLog, logBlockSize, asso,
                   "SetAsso_VIVT",
                   false, false) {}

private:
};

/**
 * Set-Associative Cache Class (PIPT)
 */
class SetAsso_PIPT : public SetAssoCache {
public:
  SetAsso_PIPT(UINT32 setsLog, UINT32 logBlockSize, UINT32 asso) :
      SetAssoCache(setsLog, logBlockSize, asso,
                   "SetAsso_PIPT",
                   true, true) {}

private:
};

/**
 * Set-Associative Cache Class (VIPT)
 */
class SetAsso_VIPT : public SetAssoCache {
public:
  SetAsso_VIPT(UINT32 setsLog, UINT32 logBlockSize, UINT32 asso) :
      SetAssoCache(setsLog, logBlockSize, asso,
                   "SetAsso_VIPT",
                   false, true) {}

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
  vector<pair<string, pair<float, size_t>>> results;
  for (auto &model: models) {
    results.emplace_back(model->name, pair(model->statistics(), model->capacity()));
    delete model;
  }
  log_write("%26s == RANKING ==\n", " ");
  sort(results.begin(), results.end(),
       [](auto &a, auto &b) { return a.second > b.second; });
  for (auto &r: results) {
    log_write("%32s : %.8f%% : %5.2f KiB\n", r.first.c_str(), 1 - r.second.first, (float) r.second.second / 8 / 0x400);
  }
}

INT32 Usage() {
  cerr << "This tool tests multiple models of caching" << endl;
  cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
  return -1;
}

#define APPEND_TEST_MODEL(inst) do {   \
  auto _p = (new inst);                \
  models.emplace_back(_p);             \
  _p->setName(#inst);                  \
} while (0)

#define APPEND_TEST_MODEL_REPLACE(inst, replace) do { \
  auto _p = (new inst);                               \
  models.emplace_back(_p);                            \
  _p->setName(#inst "-" #replace);                    \
  _p->setReplace([](auto i) {                         \
      return new replace(i);                          \
    });                                               \
  } while (0)

FILE *log_fp = nullptr;

// argc, argv are the entire command line, including pin -t <toolname> -- ...
int main(int argc, char *argv[]) {
  // Initialize pin
  if (PIN_Init(argc, argv)) return Usage();

  srand(time(nullptr));

  auto last_arg = string(argv[argc - 1]);
  if (last_arg.find('/') != string::npos) {
    last_arg = last_arg.substr(last_arg.rfind('/') + 1);
  }
  auto filename = string("cacheModels-") + last_arg + ".txt";

  log_fp = fopen(filename.c_str(), "w");

  Log("Cache Model Test Program, log to file %s", filename.c_str());

  APPEND_TEST_MODEL(DirectMappingCache(256, 6));
  APPEND_TEST_MODEL(FullAssoCache(256, 6));

  APPEND_TEST_MODEL_REPLACE(SetAssoCache(6, 6, 4), RandomRepl);
  // APPEND_TEST_MODEL_REPLACE(SetAssoCache(7, 6, 2), RandomRepl);
  // APPEND_TEST_MODEL_REPLACE(SetAssoCache(8, 6, 1), RandomRepl);

  APPEND_TEST_MODEL_REPLACE(SetAsso_VIVT(6, 6, 4), RandomRepl);
  // APPEND_TEST_MODEL_REPLACE(SetAsso_VIVT(7, 6, 2), RandomRepl);
  // APPEND_TEST_MODEL_REPLACE(SetAsso_VIVT(8, 6, 1), RandomRepl);

  APPEND_TEST_MODEL_REPLACE(SetAsso_PIPT(6, 6, 4), RandomRepl);
  // APPEND_TEST_MODEL_REPLACE(SetAsso_PIPT(7, 6, 2), RandomRepl);
  // APPEND_TEST_MODEL_REPLACE(SetAsso_PIPT(8, 6, 1), RandomRepl);

  APPEND_TEST_MODEL_REPLACE(SetAsso_VIPT(6, 6, 4), RandomRepl);
  // APPEND_TEST_MODEL_REPLACE(SetAsso_VIPT(7, 6, 2), RandomRepl);
  // APPEND_TEST_MODEL_REPLACE(SetAsso_VIPT(8, 6, 1), RandomRepl);

  APPEND_TEST_MODEL_REPLACE(SetAssoCache(6, 6, 4), LRURepl);
  // APPEND_TEST_MODEL_REPLACE(SetAssoCache(7, 6, 2), LRURepl);
  // APPEND_TEST_MODEL_REPLACE(SetAssoCache(8, 6, 1), LRURepl);

  // models.emplace_back(new SetAsso_VIVT(KnobSetsLog.Value(), KnobBlockSizeLog.Value(), KnobAssociativity.Value()));
  // Dbg("init done: SetAsso_VIVT");
  // models.emplace_back(new SetAsso_PIPT(KnobSetsLog.Value(), KnobBlockSizeLog.Value(), KnobAssociativity.Value()));
  // Dbg("init done: SetAsso_PIPT");
  // models.emplace_back(new SetAsso_VIPT(KnobSetsLog.Value(), KnobBlockSizeLog.Value(), KnobAssociativity.Value()));
  // Dbg("init done: SetAsso_VIPT");

  auto limit_bits = 32 * 8 * 0x400;
  for (auto const &m: models) {
    Assert(m->capacity() < limit_bits, "%s size is larger than limit! size is %.2f KiB", m->name.c_str(),
           (float) m->capacity() / 8 / 0x400);
  }

  Dbg("%lu models init done", models.size());

  // Register Instruction to be called to instrument instructions
  INS_AddInstrumentFunction(Instruction, nullptr);

  // Register Fini to be called when the application exits
  PIN_AddFiniFunction(Fini, nullptr);

  // Start the program, never returns
  PIN_StartProgram();

  return 0;
}
