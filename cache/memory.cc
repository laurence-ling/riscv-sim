#include "memory.h"

void Memory::HandleRequest(uint64_t addr, int bytes, int read,
                          char *content, int &hit, int &time) {
  //printf("reach memory level\n");
  hit = 1;
  time = latency_.hit_latency;
  stats_.access_counter += 1;
  stats_.access_time += time;
}

