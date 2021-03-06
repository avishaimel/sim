#pragma once
#include <stdbool.h>

#define READ "PrRd"
#define WRITE "PrWr"
#define NUM_OF_BLOCKS 64
#define BLOCK_SIZE 4
#define LEFT_MOST_20 0xfffff
#define TAG_MASK 0xfff00
#define BLOCK_MASK 0xFC
#define OFFSET_MASK 0x3

typedef enum mesi_state {
	INVALID,
	SHARED,
	EXCLUSIVE,
	MODIFIED
}mesi_state;

/* Tsram structure:*/
typedef struct Tsram {
	int tag;
	int mesi_state;
}Tsram;

/* Cache structure:*/
typedef struct cache {
	int dsram[NUM_OF_BLOCKS][BLOCK_SIZE];
	Tsram** tsram;
	int coreID;
	int read_hit;
	int read_miss;
	int write_hit;
	int write_miss;
	int mem_stall;
}Cache;

//Documentation in cache.c
int translate_address(int address);
int translate_tag(int address);
int translate_index(int address);
int translate_offset(int address);
int get_mesi_state(Cache* cache, int address, bool* tagConflict);
Cache* cache_initiation(int coreID);
void cache_access(void* core_ptr, Cache* cache, void* stall_data_ptr, int trans);
void snoop(Cache* cache, int core_id);
void free_cache(Cache* cache);
