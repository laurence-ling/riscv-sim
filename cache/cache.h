#ifndef CACHE_CACHE_H_
#define CACHE_CACHE_H_

#include <stdint.h>
#include "storage.h"

const int MAX_ROW = 20000;
const int MAX_COL = 10;

typedef struct CacheConfig_ {
  int size;
  int associativity;
  int set_num; // Number of cache sets
  int write_through; // 0|1 for back|through
  int write_allocate; // 0|1 for no-alc|alc
  int block_size;
  int block_offset;
  int set_offset;
} CacheConfig;

typedef struct {
  bool valid;
  bool dirty;
  uint64_t tag;
  int size;
  int timeStamp; // time stamp for LRU
} Block;

class Cache: public Storage {
 public:
  Cache() {lower_ = NULL;}
  ~Cache() {}

  // Sets & Gets
  void SetConfig(CacheConfig cc);
  void GetConfig(CacheConfig cc);
  void SetLower(Storage *ll) { lower_ = ll; }
  // Main access process
  void HandleRequest(uint64_t addr, int bytes, int read,
                     char *content, int &hit, int &time);
  void PrintCache();
  void PrintMissRate();

  Storage *lower_;
  
 private:
  int HaveHit(int idx, uint64_t tag);
  // Bypassing
  int BypassDecision();
  // Partitioning
  void PartitionAlgorithm();
  // Replacement
  int ReplaceDecision();
  void ReplaceAlgorithm(int idx, uint64_t tag);
  // Prefetching
  int PrefetchDecision();
  void PrefetchAlgorithm();

  CacheConfig config_;

  int nRow, nCol;
  Block array[MAX_ROW][MAX_COL];
  DISALLOW_COPY_AND_ASSIGN(Cache);
};

#endif
