#include "stdio.h"
#include "cache.h"
#include "memory.h"

int main(void) {
  Memory m;
  Cache l1;
  StorageStats s;
  l1.SetLower(&m);

  StorageLatency ml;
  ml.hit_latency = 100;
  m.SetLatency(ml);

  StorageLatency ll;
  ll.hit_latency = 10;
  l1.SetLatency(ll);

  CacheConfig cc;
  cc.associativity = 4;
  cc.set_num = 4;
  cc.block_size = 4;
  cc.write_through = 1;
  cc.write_allocate = 1;
  l1.SetConfig(cc);

  int hit, time;
  char content[64];
  l1.HandleRequest(16, 0, 0, content, hit, time);
  l1.PrintCache();
  printf("Request access time: %dns\n", time);

  l1.HandleRequest(24, 0, 1, content, hit, time);
  l1.PrintCache();
  printf("Request access time: %dns\n", time);

  l1.HandleRequest(24, 0, 0, content, hit, time);
  l1.PrintCache();
  printf("Request access time: %dns\n", time);

  l1.GetStats(s);
  printf("Total L1 access time: %dns\n", s.access_time);
  m.GetStats(s);
  printf("Total Memory access time: %dns\n", s.access_time);
  return 0;
}
