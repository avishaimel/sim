#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h> 

#include "core.h"
#include "cache.h"
#include "bridge.h" 
#include "pipeline.h"
#pragma warning(disable:4996)

//Private Functions:

Tsram** initialize_tsram() {
	struct Tsram** tsram = (struct Tsram**)malloc(sizeof(struct Tsram*)* NUM_OF_BLOCKS);
	if (tsram == NULL) {
		printf("An error occurred while allocating memory for tsram");
		exit(1); /*exiting the program after an error occured */
	}
	for (int i = 0; i < NUM_OF_BLOCKS; i++) {
		struct Tsram* tsram_line = (struct Tsram*)malloc(sizeof(struct Tsram));
		if (tsram_line == NULL) {
			printf("An error occurred while allocating memory for tsram_line");
			exit(1); /*exiting the program after an error occured */
		}
		tsram[i] = tsram_line;
		tsram[i]->mesi_state = INVALID;
		tsram[i]->tag = 0;
	}
	return tsram;
}

Cache* cache_initiation(int coreID) {
	struct cache* cache = (struct cache *) malloc(sizeof(struct cache));
	if (cache == NULL) {
		printf("An error occurred while allocating memory for cache");
		exit(1); /*exiting the program after an error occured */
	}

	for (int i = 0; i < NUM_OF_BLOCKS; i++) {
		for (int j = 0; j < BLOCK_SIZE; ++j)
		cache->dsram[i][j] = 0;
	}
	cache->coreID = coreID;
	cache->tsram = initialize_tsram();
	cache->mem_stall = 0;
	cache->read_hit = 0;
	cache->read_miss = 0;
	cache->write_hit = 0;
	cache->write_miss = 0;
	return cache;
}

int translate_address(int address) {
	return address & LEFT_MOST_20;
}

int translate_tag(int address) {
	return (address & TAG_MASK) >> 8;
}

int translate_index(int address) {
	return (address & BLOCK_MASK) >> 2;
}

int translate_offset(int address) {
	return address & OFFSET_MASK;
}

char* translate_mesi_transaction(int opcode) {
	if (opcode == LW) {
		return READ;
	}
	else if (opcode == SW) {
		return WRITE;
	}
	else {
		printf("Error_Chace_3: Opcode given to cache is not legal");
		exit(1);
	}
}

int get_mesi_state_old(Cache* cache, int index, int tag, bool* tagConflict) {
	if (cache->tsram[index]->mesi_state == INVALID) {
		return INVALID;
	}

	if (cache->tsram[index]->tag == tag) {
		return cache->tsram[index]->mesi_state;
	}

	*tagConflict = true;

	return INVALID;
}

int get_mesi_state(Cache* cache, int address, bool* tag_conflict) {
	int block = translate_index(address);
	if (cache->tsram[block]->tag != translate_tag(address)) {
		*tag_conflict = true;
	}
	return cache->tsram[block]->mesi_state;
}

//void update_stats(Cache* cache, char* transaction, int increment) {
//	if (!strcmp(transaction, READ)) {
//		cache->read_hit
//	}
//}

int mesi_state_machine(char* type_transaction, int mesi_current_state) {
	int next_state = INVALID;
	switch (mesi_current_state) {
	case INVALID:
		if (!strcmp(type_transaction, READ)) {
			if (main_bridge->main_bus->bus_shared == 1)
				next_state = SHARED;
			else {
				next_state = EXCLUSIVE;
				main_bridge->main_bus->bus_shared = 0;
			}
		}
		else if (!strcmp(type_transaction, WRITE)) {
			next_state = MODIFIED;
		}
		else {
			printf("Error_Cache_2: Mesi transaction given not legal");
			exit(1);
		}
		break;

	case SHARED:
		if (!strcmp(type_transaction, READ)) {
			next_state = SHARED;
		}
		else if (!strcmp(type_transaction, WRITE)) {
			next_state = MODIFIED;
		}
		else {
			printf("Error_Cache_2: Mesi transaction given not legal");
			exit(1);
		}
		break;

	case EXCLUSIVE:
		if (!strcmp(type_transaction, READ)) {
			next_state = EXCLUSIVE;
		}
		else if (!strcmp(type_transaction, WRITE)) {
			next_state = MODIFIED;
		}
		else {
			printf("Error_Cache_2: Mesi transaction given not legal");
			exit(1);
		}
		break;

	case MODIFIED:
		if (!strcmp(type_transaction, READ)) {
			next_state = MODIFIED;
		}
		else if (!strcmp(type_transaction, WRITE)) {
			next_state = MODIFIED;
		}
		else {
			printf("Error_Cache_2: Mesi transaction given not legal");
			exit(1);
		}
		break;
	}

	return next_state;
}

void cache_read(Core* core, Cache* cache, int address, StallData* stall_data) {
	bool tag_conflict = false;
	int mesi_state = get_mesi_state(core->Cache, core->current_state_Reg->ex_mem->ALUOutput, &tag_conflict);
	if (mesi_state == INVALID || tag_conflict) {
		//return if already stalled
		if (stall_data[MEMORY].active) {
			core->new_state_Reg->mem_wb->isStall = true;
			return;
		}
		stall_data[MEMORY].active = true;
		core->new_state_Reg->mem_wb->isStall = true;
		cache->read_miss++;

	}
}

void free_cache(Cache* cache) {
	for (int i = 0; i < NUM_OF_BLOCKS; i++) {
		free(cache->tsram[i]);
	}
	free(cache->tsram);
	free(cache);
}