#include "cache.h"
#include "def.h"

void Cache::HandleRequest(uint64_t addr, int bytes, int read,
                          char *content, int &hit, int &time) {

    time = 0;
    stats_.access_counter += 1;
    uint64_t temp = addr >> config_.block_offset;
    int64_t mask = ~(-1 << config_.set_offset);
    int idx = (int)(temp & mask);
    uint64_t tag = temp >> config_.set_offset;

    if (hit = HaveHit(idx, tag)){
        #ifdef DEBUG
            printf("hit! addr = %x, idx = %x, tag = %x\n", (int)addr, idx, (int)tag);
        #endif
        time += latency_.hit_latency;
        stats_.access_time += time;

        if (read == 1) { // if read, return immediately     
            return;
        } else {
            if (config_.write_through == 1) {
                int lower_time, lower_hit;
                lower_->HandleRequest(addr, bytes, read, content,
                    lower_hit, lower_time);
                time += lower_time;
            } else {
                // write back, set dirty flag
                
            }
            return;
        }
    }

    // Miss
    stats_.miss_num += 1;
    int lower_hit, lower_time;
    #ifdef DEBUG
        printf("miss! request lower addr %x\n", (int)addr);
    #endif
    lower_->HandleRequest(addr, bytes, read, content,
                          lower_hit, lower_time);
    hit = lower_hit;
    time += lower_time;

    if (read == 1 || config_.write_allocate == 1) { // update cache
        if (BypassDecision())
            return;
        ReplaceAlgorithm(idx, tag);
    }
}

int Cache::HaveHit(int idx, uint64_t tag) {
    for (int i = 0; i < nCol; ++i) {
        if (array[idx][i].valid && array[idx][i].tag == tag){
            array[idx][i].timeStamp = stats_.access_counter; // update the time stamp
            return 1;
        }
    }
    return 0;
}

int Cache::BypassDecision() {
  return FALSE;
}

void Cache::PartitionAlgorithm() {
}

int Cache::ReplaceDecision() {
  return TRUE;
  //return FALSE;
}

void Cache::ReplaceAlgorithm(int idx, uint64_t tag){
    for (int i = 0; i < nCol; ++i) {
        if (!array[idx][i].valid){ // not full
            array[idx][i].valid = 1;       
            array[idx][i].tag = tag;            
            array[idx][i].timeStamp = stats_.access_counter; //time stamp
            return;
        }
    }
    // LRU replace
    int evict = 0;
    int oldest = 0x7fffffff;
    for (int i = 0; i < nCol; ++i) {
        if (array[idx][i].timeStamp < oldest) {
            oldest = array[idx][i].timeStamp;
            evict = i;
        }
    }
    //evict
    array[idx][evict].valid = 1;
    array[idx][evict].tag = tag;
    array[idx][evict].timeStamp = stats_.access_counter;
}

int Cache::PrefetchDecision() {
  return FALSE;
}

void Cache::PrefetchAlgorithm() {
}

void Cache::SetConfig(CacheConfig cc) {
    config_ = cc;
    nRow = cc.set_num;
    nCol = cc.associativity;
    if (config_.block_size == 1) {
            config_.block_offset = 0;
    } else {
        for (int i = 0; ; ++i) {
            if ((1 << i) < config_.block_size && (1 << (i+1)) >= config_.block_size){
                config_.block_offset = i + 1;
                break;
            }
        }
    }
    if (config_.set_num == 1) {
            config_.set_offset = 0;
    } else {
        for (int i = 0; ; ++i) {
            if ((1 << i) < config_.set_num && (1 << (i+1)) >= config_.set_num){
                config_.set_offset = i + 1;
                break;
            }
        }
    }
}

void Cache::PrintCache() {
    for (int i = 0; i < nRow; ++i) {
        for (int j = 0; j < nCol; ++j)
            printf("(t %x, v %x, s %x) ", (int)array[i][j].tag,
                array[i][j].valid, array[i][j].timeStamp);
        printf("\n");
    }
}

void Cache::PrintMissRate() {
    printf("Cache access count: %d, miss_num: %d, miss_rate: %f\n", 
        stats_.access_counter, stats_.miss_num,
        1.0 * stats_.miss_num / stats_.access_counter);
}