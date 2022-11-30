//
// Created by chiro on 22-11-17.
//
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include "pin.H"

using namespace std;

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long int UINT64;
typedef unsigned __int128 UINT128;


ofstream OutFile;

// 将val截断, 使其宽度变成bits
#define truncate(val, bits) ((val) & ((1 << (bits)) - 1))

const static int TEST_SIZE_MAX = 32;

class TestResult {
public:
  UINT64 takenPcCorrect = 0;
  UINT64 takenPcIncorrect = 0;
  UINT64 takenCorrect = 0;
  UINT64 takenIncorrect = 0;
  UINT64 notTakenCorrect = 0;
  UINT64 notTakenIncorrect = 0;
  const char *name = "Unknown";
};

static vector<TestResult> results(TEST_SIZE_MAX);

// 饱和计数器 (N < 64)
class SaturatingCnt {
  size_t m_wid;
  UINT8 m_val;
  const UINT8 m_init_val;

public:
  // Initial value is weak-taken
  SaturatingCnt(size_t width = 2) : m_init_val((1 << width) / 2) {
    m_wid = width;
    m_val = m_init_val;
  }

  void increase() { if (m_val < (1 << m_wid) - 1) m_val++; }

  void decrease() { if (m_val > 0) m_val--; }

  void reset() { m_val = m_init_val; }

  UINT8 getVal() { return m_val; }

  bool isTaken() { return (m_val > (1 << m_wid) / 2 - 1); }

  void setVal(UINT8 value) { m_val = value; }
};

// 移位寄存器 (N < 128)
class ShiftReg {
  size_t m_wid;
  UINT128 m_val;

public:
  ShiftReg(size_t width) : m_wid(width), m_val(0) {}

  bool shiftIn(bool b) {
    bool ret = !!(m_val & (1 << (m_wid - 1)));
    m_val <<= 1;
    m_val |= b;
    m_val &= (1 << m_wid) - 1;
    return ret;
  }

  UINT128 getVal() { return m_val; }

  size_t getMWid() const {
    return m_wid;
  }
};

// Base class of all predictors
class BranchPredictor {
public:
  BranchPredictor() {}

  virtual ~BranchPredictor() {}

  /**
   * Predict take and pc
   * @param addr PC to judge
   * @return 0==not-taken 1==taken other==predicted-pc
   */
  virtual ADDRINT predict(ADDRINT addr) { return 0; };

  virtual void update(bool takenActually, bool takenPredicted, ADDRINT addr, ADDRINT target) {};

  virtual uint64_t getTagFromAddr(ADDRINT addr) { return 0; };

  /**
   * capacity usage **IN BYTES**
   * @return
   */
  virtual UINT32 capacity() { return 0; };

  double capacity_kib() { return ((double) capacity()) / 1024.0; };
};

BranchPredictor *BP[TEST_SIZE_MAX] = {0};


/**
 * StaticPredictor: random choice
 */
class StaticPredictor : public BranchPredictor {
public:
  StaticPredictor() {
    srand(time(nullptr));
  }

private:
  ADDRINT predict(ADDRINT addr) { return rand() % 2; };
};

class BHTEntry {
public:
  bool valid = false;
  SaturatingCnt cnt;
  ADDRINT target = 0;
  ADDRINT tag = 0;

  explicit BHTEntry(size_t width = 2) : cnt(SaturatingCnt(width)) {}

  void setVal(bool valid_, SaturatingCnt cnt_, ADDRINT target_) {
    valid = valid_;
    cnt.setVal(cnt_.getVal());
    target = target_;
  }
};

using PHTEntry = BHTEntry;

/* ===================================================================== */
/* BHT-based branch predictor                                            */
/* ===================================================================== */
class BHTPredictor : public BranchPredictor {
protected:
  size_t m_entries_log;
  vector<BHTEntry> entries;              // BHT
  bool predict_address;
  size_t m_scnt_width;

public:
  // Constructor
  // param:   entry_num_log:  BHT行数的对数
  //          scnt_width:     饱和计数器的位数, 默认值为2
  // max size 33 KiB, every entry (2+64) bit, tot = 66 bit
  // 33 * 0x400 * 8 = 270336 > 135168 = 66 * 2048 = 66 * 2^n, n = 11
  explicit BHTPredictor(size_t entry_num_log = 11, size_t scnt_width = 2, bool predict_address = true) :
          m_scnt_width(scnt_width) {
    m_entries_log = entry_num_log;
    this->predict_address = predict_address;
    // OutFile << this << " Init entries, entry_num_log = " << entry_num_log << endl;
    for (int i = 0; i < (1 << entry_num_log); i++) {
      entries.emplace_back(BHTEntry(scnt_width));
    }
  }

  UINT32 capacity() override {
    if (predict_address) {
      return ((sizeof(BHTEntry::target) + m_scnt_width) * (1 << m_entries_log)) / 8;
    } else return (m_scnt_width * (1 << m_entries_log)) / 8;
  }

  // Destructor
  ~BHTPredictor() {
  }

  uint64_t getTagFromAddr(ADDRINT addr) override {
    return truncate(addr >> 2, m_entries_log);
  }

  virtual BHTEntry &getEntryFromAddr(ADDRINT addr) {
    return entries[getTagFromAddr(addr)];
  }

  ADDRINT predict(ADDRINT addr) override {
    // Produce prediction according to BHT
    auto entry = getEntryFromAddr(addr);
    if (predict_address) {
      return entry.cnt.isTaken() ? (entry.target ? entry.target : 1) : 0;
    } else {
      return entry.cnt.isTaken() ? 1 : 0;
    }
  }

#pragma clang diagnostic push
#pragma ide diagnostic ignored "Simplify"

  void update(BOOL takenActually, BOOL takenPredicted, ADDRINT addr, ADDRINT target) override {
    // Update BHT according to branch results and prediction
    auto &entry = getEntryFromAddr(addr);
    if (predict_address && false) {
      // is this logic ok...?
      if (!entry.valid) {
        if (takenActually) {
          entry.valid = true;
          entry.cnt.reset();
          entry.target = target;
        }
      } else {
        if (entry.target == target) {
          if (takenActually) {
            entry.cnt.increase();
            entry.target = target;
          } else {
            entry.cnt.decrease();
          }
        } else {
          entry.valid = false;
        }
      }
    } else {
      if (takenActually) {
        entry.cnt.increase();
        entry.target = target;
      } else {
        entry.cnt.decrease();
      }
    }
  }

#pragma clang diagnostic pop
};

class HashMethods {
private:
  /**
   * Fold data h into m bytes
   * @tparam F
   * @param h
   * @param m
   * @return
   */
  template<typename F>
  static UINT128 fold(UINT128 h, UINT128 m, F const &f) {
    UINT128 r = 0;
    for (int i = 0; i < (128 / 8) / m; i++) {
      auto s = ((1 << (8 * m)) - 1) & h;
      h >>= (8 * m);
      r = f(r, s);
    }
    return r;
  }

public:
  /**
   * Simply slice address to make hash, assert 4 bytes align
   * @param addr
   * @param history
   * @return hashed data
   */
  inline static UINT128 slice(UINT128 addr, UINT128 history) {
    return addr;
  }

  inline static UINT128 hash_xor(UINT128 addr, UINT128 history) {
    return addr ^ history;
  }

  inline static UINT128 fold_xor(UINT128 addr, UINT128 history) {
    return fold(addr, 2, [](auto a, auto b) { return a ^ b; }) ^ history;
  }

  // Hash functions
  inline static UINT128 f_xor(UINT128 a, UINT128 b) { return a ^ b; }

  inline static UINT128 f_xor1(UINT128 a, UINT128 b) { return ~a ^ ~b; }

  inline static UINT128 f_xnor(UINT128 a, UINT128 b) { return ~(a ^ ~b); }
};

/* ===================================================================== */
/* Global-history-based branch predictor                                 */
/* ===================================================================== */
template<UINT128 (*hash)(UINT128 addr, UINT128 history)>
class GlobalHistoryPredictor : public BHTPredictor {
  ShiftReg *m_ghr;                   // GHR

public:
  // Constructor
  // param:   ghr_width:      Width of GHR
  //          entry_num_log:  PHT表行数的对数
  //          scnt_width:     饱和计数器的位数, 默认值为2
  // PHT.w = 2+64+1, tot = (2^11)*(67.0/8)+8 = 17152B < 33KiB = 270336
  GlobalHistoryPredictor(size_t ghr_width = 8, size_t entry_num_log = 11, size_t scnt_width = 2,
                         bool predict_address = true)
          : BHTPredictor(entry_num_log, scnt_width, predict_address) {
    m_ghr = new ShiftReg(ghr_width);
  }

  BHTEntry &getEntryFromAddr(ADDRINT addr) override {
    return entries[getTagFromAddr(addr)];
  }

  uint64_t getTagFromAddr(ADDRINT addr) override {
    return truncate(hash(addr, m_ghr->getVal()), m_entries_log);
  }

  // Destructor
  ~GlobalHistoryPredictor() {}

  // Only for TAGE: return a tag according to the specificed address
  UINT128 get_tag(ADDRINT addr) {
    return getEntryFromAddr(addr).tag;
  }

  // Only for TAGE: return GHR's value
  UINT128 get_ghr() {
    return m_ghr->getVal();
  }

  ShiftReg *get_ghr_instance() const {
    return m_ghr;
  }

  // Only for TAGE: reset a saturating counter to default value (which is weak taken)
  void reset_ctr(ADDRINT addr) {
    getEntryFromAddr(addr).cnt.reset();
  }

  ADDRINT predict(ADDRINT addr) override {
    // Produce prediction according to GHR and PHT
    auto entry = getEntryFromAddr(addr);
    if (predict_address)
      return entry.cnt.isTaken() ? (entry.target ? entry.target : 1) : 0;
    else return entry.cnt.isTaken();
  }

  void update(bool takenActually, bool takenPredicted, ADDRINT addr, ADDRINT target) override {
    // Update GHR and PHT according to branch results and prediction
    auto &entry = getEntryFromAddr(addr);
    if (takenActually) {
      entry.cnt.increase();
      entry.target = target;
    } else {
      entry.cnt.decrease();
    }
    m_ghr->shiftIn(takenActually);
  }
};

/* ===================================================================== */
/* Tournament predictor: Select output by global/local selection history */
/* ===================================================================== */
class TournamentPredictor : public BranchPredictor {
  BranchPredictor *m_BPs[2];      // Sub-predictors
  SaturatingCnt *m_gshr;          // Global select-history register
  bool predict_addr;

public:
  /**
   * PBs will be managed by this class.
   * @param BP0
   * @param BP1
   * <br/>
   * width: 66, 67; size: 66*2^10+67*2^11=204800 < 270336
   */
  TournamentPredictor(BranchPredictor *BP0, BranchPredictor *BP1, bool predict_addr = true) {
    size_t gshr_width = 2;
    m_BPs[0] = BP0;
    m_BPs[1] = BP1;
    m_gshr = new SaturatingCnt(gshr_width);
    this->predict_addr = predict_addr;
  }

  UINT32 capacity() override {
    return m_BPs[0]->capacity() + m_BPs[1]->capacity();
  }

  ~TournamentPredictor() {
    delete m_gshr;
    delete m_BPs[0];
    delete m_BPs[1];
  }

  ADDRINT predict(ADDRINT addr) override {
    if (m_gshr->isTaken()) {
      return m_BPs[1]->predict(addr);
    } else {
      return m_BPs[0]->predict(addr);
    }
  }

  void update(bool takenActually, bool takenPredicted, ADDRINT addr, ADDRINT target) override {
    auto predict1 = m_BPs[0]->predict(addr);
    auto predict2 = m_BPs[1]->predict(addr);
    bool correct1, correct2;
    if (!predict_addr) {
      auto actually = takenActually ? 1 : 0;
      correct1 = (predict1 ? 1 : 0) == actually;
      correct2 = (predict2 ? 1 : 0) == actually;
    } else {
      correct1 = predict1 == target;
      correct2 = predict2 == target;
    }
    if (correct1 && !correct2) {
      m_gshr->decrease();
    }
    if (!correct1 && correct2) {
      m_gshr->increase();
    }
    m_BPs[0]->update(takenActually, takenPredicted, addr, target);
    m_BPs[1]->update(takenActually, takenPredicted, addr, target);
  }
};

/* ===================================================================== */
/* TArget GEometric history length Predictor                             */
/* ===================================================================== */
template<UINT128 (*hash1)(UINT128 pc, UINT128 ghr) = HashMethods::fold_xor,
        UINT128 (*hash2)(UINT128 pc, UINT128 ghr) = HashMethods::hash_xor>
class TAGEPredictor : public BranchPredictor {
  const size_t m_tnum;            // 子预测器个数 (T[0 : m_tnum - 1])
  const size_t m_entries_log;     // 子预测器T[1 : m_tnum - 1]的PHT行数的对数
  BranchPredictor **m_T;          // 子预测器指针数组
  bool *m_T_pred;                 // 用于存储各子预测的预测值
  UINT8 **m_useful;               // usefulness matrix
  int provider_index = -1;        // Provider's index of m_T
  int altpred_index = -1;         // Alternate provider's index of m_T
  int useful_bits = 2;
  int scnt_width;
  int Tn_entry_num_log;
  int T0_entry_num_log;

  const size_t m_rst_period;      // Reset period of usefulness
  size_t m_rst_cnt;               // Reset counter
  static const size_t tnum_max = 12;

public:
  // Constructor
  // param:   tnum:               The number of sub-predictors
  //          T0_entry_num_log:   子预测器T0的BHT行数的对数
  //          T1ghr_len:          子预测器T1的GHR位宽
  //          alpha:              各子预测器T[1 : m_tnum - 1]的GHR几何倍数关系
  //          Tn_entry_num_log:   各子预测器T[1 : m_tnum - 1]的PHT行数的对数
  //          scnt_width:         Width of saturating counter (3 by default)
  //          rst_period:         Reset period of usefulness
  // width: 2, 70; size = 2 * (1<<T0_entry_num_log) + (64+useful_bits+scnt_width) * (1<<Tn_entry_num_log)
  TAGEPredictor(size_t tnum, size_t T0_entry_num_log, size_t T1ghr_len, float alpha, size_t Tn_entry_num_log,
                size_t scnt_width = 3, size_t rst_period = 256 * 1024, int useful_bits = 2)
          : m_tnum(tnum), m_entries_log(Tn_entry_num_log), m_rst_period(rst_period), m_rst_cnt(0),
            useful_bits(useful_bits), scnt_width(scnt_width), T0_entry_num_log(T0_entry_num_log),
            Tn_entry_num_log(Tn_entry_num_log) {
    m_T = new BranchPredictor *[m_tnum];
    m_T_pred = new bool[m_tnum];
    m_useful = new UINT8 *[m_tnum];

    m_T[0] = new BHTPredictor(T0_entry_num_log, 2, false);

    size_t ghr_size = T1ghr_len;
    for (size_t i = 1; i < m_tnum; i++) {
      m_T[i] = new GlobalHistoryPredictor<hash1>(ghr_size, m_entries_log, scnt_width, false);
      ghr_size = (size_t) (round((double) ghr_size * alpha));

      m_useful[i] = new UINT8[1 << m_entries_log];
      memset(m_useful[i], 0, sizeof(UINT8) * (1 << m_entries_log));
    }
  }

  UINT32 capacity() override {
    return ((2 * (1 << T0_entry_num_log)) +
            ((m_entries_log + useful_bits + scnt_width) * (m_tnum ? (m_tnum - 1) : 0)) * (1 << Tn_entry_num_log)) / 8;
  }

  ~TAGEPredictor() {
    for (size_t i = 0; i < m_tnum; i++) delete m_T[i];
    for (size_t i = 1; i < m_tnum; i++) delete[] m_useful[i];

    delete[] m_T;
    delete[] m_T_pred;
    delete[] m_useful;
  }

  void update_useful_entry(int predicator, int index, bool increase) {
    auto &usefulness = m_useful[predicator][index];
    if (increase) {
      if (usefulness != ((1 << useful_bits) - 1)) usefulness++;
    } else {
      if (usefulness != 0) usefulness--;
    }
  }

  ADDRINT predict(ADDRINT addr) override {
    UINT128 predictors_max_ghr = 0;
    int predictors_max_ghr_index = -1;
    int predictors_max_ghr_index2 = -1;
    int tag_matched_count = 0;
    for (size_t i = 0; i < m_tnum; i++) {
      if (i != 0) {
        auto ghp = ((GlobalHistoryPredictor<hash1> *) (m_T[i]));
        auto entry = ghp->getEntryFromAddr(addr);
        auto h2 = hash2(addr, ghp->get_ghr_instance()->getVal());
        // Has tag matched
        if (entry.tag == h2) {
          tag_matched_count++;
          if (predictors_max_ghr < ghp->get_ghr_instance()->getMWid()) {
            predictors_max_ghr_index2 = predictors_max_ghr_index;
            predictors_max_ghr_index = (int) i - 1;
          }
        }
      }
    }
    altpred_index = 0;
    if (tag_matched_count == 0) {
      // use T0 as provider and altpred
      provider_index = 0;
    } else if (tag_matched_count == 1) {
      provider_index = predictors_max_ghr_index + 1;
    } else {
      // count >= 2
      provider_index = predictors_max_ghr_index + 1;
      altpred_index = predictors_max_ghr_index2 + 1;
    }
    // cerr << "provider: " << provider_index << ", altpred: " << altpred_index << endl;
    return m_T[provider_index]->predict(addr);
  }

  void update(bool takenActually, bool takenPredicted, ADDRINT addr, ADDRINT target) override {
    auto predict_provider = m_T[provider_index]->predict(addr) != 0;
    auto predict_altpred = m_T[altpred_index]->predict(addr) != 0;
    auto provider_entry_index = m_T[provider_index]->getTagFromAddr(addr);
    auto branch_actually = (takenActually ? 1 : 0);
    // Update provider itself
    m_T[provider_index]->update(takenActually, takenPredicted, addr, target);

    // Update usefulness
    if (provider_index != 0 && predict_altpred != predict_provider) {
      update_useful_entry(provider_index, provider_entry_index, predict_provider == branch_actually);
    }

    // Reset usefulness periodically
    m_rst_cnt++;
    if (m_rst_cnt == m_rst_period) {
      m_rst_cnt = 0;
      // T0 has no useful field
      for (int i = 1; i < m_tnum - 1; i++) {
        memset(m_useful[i], 0, sizeof(UINT8) * (1 << m_entries_log));
      }
    }

    // Entry replacement
    vector<int> predicator_a;
    vector<int> predicator_b;
    if (predict_provider != branch_actually) {
      // find a predictor that has longer history and usefulness==0
      for (int i = 1; i < m_tnum; i++) {
        if (i == provider_index) continue;
        auto ghp_provider = ((GlobalHistoryPredictor<hash1> *) (m_T[provider_index]));
        auto ghp = ((GlobalHistoryPredictor<hash1> *) (m_T[i]));
        bool longer_history;
        if (provider_index != 0) {
          longer_history = ghp->get_ghr_instance()->getMWid() > ghp_provider->get_ghr_instance()->getMWid();
        } else {
          longer_history = true;
        }
        if (longer_history) {
          predicator_a.emplace_back(i);
        }
        bool usefulness = m_useful[i][ghp->getTagFromAddr(addr)];
        if (usefulness == 0 && longer_history) {
          predicator_b.emplace_back(i);
        }
      }
    }
    sort(predicator_b.begin(), predicator_b.end(),
         [&](int &a, int &b) {
           auto c = (GlobalHistoryPredictor<hash1> *) m_T[a];
           auto d = (GlobalHistoryPredictor<hash1> *) m_T[b];
           return c->get_ghr_instance()->getMWid() > d->get_ghr_instance()->getMWid();
         });
    if (!predicator_b.empty()) {
      auto i = *predicator_b.begin();
      auto p = (GlobalHistoryPredictor<hash1> *) m_T[i];
      p->reset_ctr(addr);
      // memset(m_useful[i], 0, sizeof(UINT8) * (1 << m_entries_log));
      p->getEntryFromAddr(addr).tag = hash2(addr, p->get_ghr_instance()->getVal());
    } else {
      for (auto &i: predicator_a) {
        auto p = (GlobalHistoryPredictor<hash1> *) m_T[i];
        update_useful_entry(i, p->getTagFromAddr(addr), false);
        // p->getEntryFromAddr(addr).tag = hash2(addr, p->get_ghr_instance()->getVal());
      }
    }
  }
};


// This function is called every time a control-flow instruction is encountered
void predictBranch(ADDRINT pc, BOOL direction, ADDRINT target) {
  for (int i = 0; i < TEST_SIZE_MAX; i++) {
    auto P = BP[i];
    if (!P) continue;
    auto &r = results[i];
    ADDRINT prediction = P->predict(pc);
    P->update(direction, prediction, pc, target);
    if (prediction) {
      if (direction) {
        r.takenCorrect++;
        // == 1 means no prediction
        if (prediction != 1) {
          if (prediction == target) {
            r.takenPcCorrect++;
          } else {
            r.takenPcIncorrect++;
            // OutFile << "Incorrect predict:real = " << hex << (int) prediction << ":" << (int) target << endl;
          }
        }
      } else
        r.takenIncorrect++;
    } else {
      if (direction)
        r.notTakenIncorrect++;
      else
        r.notTakenCorrect++;
    }
  }
}

// Pin calls this function every time a new instruction is encountered
void Instruction(INS ins, void *v) {
  if (INS_IsControlFlow(ins) && INS_HasFallThrough(ins)) {
    // Insert a call to the branch target
    INS_InsertCall(ins, IPOINT_TAKEN_BRANCH, (AFUNPTR) predictBranch,
                   IARG_INST_PTR, IARG_BOOL, TRUE, IARG_BRANCH_TARGET_ADDR, IARG_END);

    // IARG_INST_PTR: This value does not change at IPOINT_AFTER.
    // Insert a call to the next instruction of a branch
    INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR) predictBranch,
                   IARG_INST_PTR, IARG_BOOL, FALSE, IARG_BRANCH_TARGET_ADDR, IARG_END);
  }
}

// This knob sets the output file name
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "brchPredict.txt", "specify the output file name");

// This function is called when the application exits
VOID Fini(int, VOID *v) {
  int rank_best = -1;
  double precision_best = 0;
  for (int i = 0; i < TEST_SIZE_MAX; i++) {
    if (!BP[i]) continue;
    auto r = results[i];
    double precision = 100 * double(r.takenCorrect + r.notTakenCorrect) /
                       ((double) r.takenCorrect + (double) r.notTakenCorrect + (double) r.takenIncorrect +
                        (double) r.notTakenIncorrect);

    // cout << "result[" << i << "] name: " << r.name << endl
    //      << "capacity: " << BP[i]->capacity_kib() << " KiB" << endl
    //      << "takenCorrect: " << r.takenCorrect << endl
    //      << "takenIncorrect: " << r.takenIncorrect << endl
    //      << "notTakenCorrect: " << r.notTakenCorrect << endl
    //      << "notTakenIncorrect: " << r.notTakenIncorrect << endl
    //      << "Precision: " << precision << endl;
    // if (r.takenPcCorrect != 0 || r.takenPcIncorrect != 0)
    //   cout << "takenPcCorrect: " << r.takenPcCorrect << endl
    //        << "takenPcIncorrect: " << r.takenPcIncorrect << endl
    //        << "PcPrecision: "
    //        << (100.0 * (double) r.takenPcCorrect / ((double) r.takenPcCorrect + (double) r.takenPcIncorrect))
    //        << endl;

    OutFile.setf(ios::showbase);
    OutFile << "result[" << i << "] name: " << r.name << endl
            << "capacity: " << BP[i]->capacity_kib() << " KiB" << endl
            << "takenCorrect: " << r.takenCorrect << endl
            << "takenIncorrect: " << r.takenIncorrect << endl
            << "notTakenCorrect: " << r.notTakenCorrect << endl
            << "notTakenIncorrect: " << r.notTakenIncorrect << endl
            << "Precision: " << precision << endl;
    if (r.takenPcCorrect != 0 || r.takenPcIncorrect != 0)
      OutFile << "takenPcCorrect: " << r.takenPcCorrect << endl
              << "takenPcIncorrect: " << r.takenPcIncorrect << endl
              << "PcPrecision: "
              << (100.0 * (double) r.takenPcCorrect / ((double) r.takenPcCorrect + (double) r.takenPcIncorrect))
              << endl;

    delete BP[i];

    if (precision > precision_best) {
      precision_best = precision;
      rank_best = i;
    }
  }
  if (rank_best >= 0) {
    auto best = results[rank_best];
    cout << "Best: result[" << rank_best << "] " << best.name << " with precision " << precision_best << endl;
    OutFile << "Best: result[" << rank_best << "] " << best.name << " with precision " << precision_best << endl;
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

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */
/*   argc, argv are the entire command entry: pin -t <toolname> -- ...    */
/* ===================================================================== */

static int test_tail = 0;

#define SET_TEST_PREDICTOR(index, inst) do {   \
  BP[(index)] = (new inst);                    \
  results[(index)].name = #inst;               \
} while (0)

#define APPEND_TEST_PREDICTOR(inst) do {   \
  BP[(test_tail)] = (new inst);            \
  results[(test_tail)].name = #inst;       \
  test_tail++;                             \
} while (0)

int main(int argc, char *argv[]) {
  // Initialize pin
  if (PIN_Init(argc, argv)) return Usage();

  // APPEND_TEST_PREDICTOR(StaticPredictor());
#ifdef PREPARING
  // APPEND_TEST_PREDICTOR(BHTPredictor());
  cerr << "Prepare with no predicator loaded!" << endl;
#else
  // auto filename = KnobOutputFile.Value();
  auto last_arg = string(argv[argc - 1]);
  if (last_arg.find('/') != string::npos) {
    last_arg = last_arg.substr(last_arg.rfind('/') + 1);
  }
  auto filename = string("brchPredict-") + last_arg + ".txt";
  OutFile.open(filename.c_str());
  cerr << "Output filename: " << filename << endl;

  bool allow_oversize = true;
  // bool allow_oversize = false;

  APPEND_TEST_PREDICTOR(BHTPredictor(14));
  APPEND_TEST_PREDICTOR(BHTPredictor(17, 2, false));
  APPEND_TEST_PREDICTOR(GlobalHistoryPredictor<HashMethods::fold_xor>(20, 17, 2, false));
  APPEND_TEST_PREDICTOR(TournamentPredictor(new BHTPredictor(13), new GlobalHistoryPredictor<HashMethods::fold_xor>(20, 13)));
  APPEND_TEST_PREDICTOR(TournamentPredictor(new BHTPredictor(16, 2, false), new GlobalHistoryPredictor<HashMethods::fold_xor>(20, 16, 2, false)));

  // APPEND_TEST_PREDICTOR(TAGEPredictor(7, 16, 4, 1.2, 10));
  // APPEND_TEST_PREDICTOR(TAGEPredictor(5, 16, 4, 1.2, 11));
  // APPEND_TEST_PREDICTOR(TAGEPredictor(3, 16, 18, 1.2, 12));
  APPEND_TEST_PREDICTOR(TAGEPredictor(3, 16, 10, 2, 12));

  // APPEND_TEST_PREDICTOR(TAGEPredictor(5, 16, 8, 1.5, 11));
  // APPEND_TEST_PREDICTOR(TAGEPredictor(5, 16, 16, 1.2, 11));
  APPEND_TEST_PREDICTOR(TAGEPredictor(5, 16, 20, 1.2, 11));
  // APPEND_TEST_PREDICTOR(TAGEPredictor(5, 16, 24, 1.2, 11));

  // APPEND_TEST_PREDICTOR(TAGEPredictor(3, 16, 16, 1.5, 12));
  // APPEND_TEST_PREDICTOR(TAGEPredictor(5, 16, 8, 1.8, 11));
#endif

  // check capacity
  bool oversize = false;
  for (int i = 0; i < TEST_SIZE_MAX; i++) {
    auto t = BP[i];
    if (!t) continue;
    if (t->capacity() > 33 * 0x400) {
      OutFile << "predictor[" << i << "] " << results[i].name << " oversize! capacity = " << t->capacity() << ", "
              << t->capacity_kib() << " KiB" << endl;
      cerr << "predictor[" << i << "] " << results[i].name << " oversize! capacity = " << t->capacity() << ", "
           << t->capacity_kib() << " KiB" << endl;
      oversize = true;
    }
  }
  if (oversize && !allow_oversize) {
    exit(1);
  }

  // Register Instruction to be called to instrument instructions
  INS_AddInstrumentFunction(Instruction, nullptr);

  // Register Fini to be called when the application exits
  PIN_AddFiniFunction(Fini, nullptr);

  // Start the program, never returns
  PIN_StartProgram();

  return 0;
}
