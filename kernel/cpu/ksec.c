#include "../libs/types.h"
#include "../drivers/timer.h"

uint32_t seed;

//seed kernel security randomizer;
void seed_ksecr(uint32_t gseed){
    seed = gseed;
}

int prng(){
   int num = seed;
   num = (((seed % (ticks + 1)) > 5) * (ticks + (seconds / (seed + 1)) | seed << (seconds / ((ticks % 4 + 1)))) + seed ^ ((ticks + 1) << seed % ((seconds % 31) + 1)));
   seed = (num - (ticks | (seed + seconds))) + 1;
   return num;
}
uint32_t gen_driver_key(uint8_t pid, uint32_t modifier){
    seed_ksecr(seed);
    return prng() | pid + ((pid > seed ? 1 : -1) * modifier);
}