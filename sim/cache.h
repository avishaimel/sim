#pragma once
#include <stdbool.h>

#define READ "PrRd"
#define WRITE "PrWr"
#define NUM_OF_BLOCK 256
#define LEFT_MOST_20 0xfffff
#define LEFT_MOST_8 0xff
#define RIGHT_MOST_12 0xfff00

typedef enum mesi_state {
	INVALID,
	SHARED,
	EXCLUSIVE,
	MODIFIED
}mesi_state;

typedef struct Tsram {
	int tag;
	int mesi_state;
}Tsram;

/* Cache structure:*/
typedef struct cache {
	int dsram[NUM_OF_BLOCK];
	Tsram** tsram;
	int coreID;
	int read_hit;
	int read_miss;
	int write_hit;
	int write_miss;
	int mem_stall;
}Cache;


int translate_address(int address);
int translate_tag(int address);
int translate_index(int address);
char* translate_mesi_transaction(int opcode);
int get_mesi_state(Cache* cache, int index, int tag, bool* tagConflict);
int mesi_state_machine(char* type_transaction, int mesi_current_state);
Cache* cache_initiation(int coreID);
void free_cache(Cache* cache);
