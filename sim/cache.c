#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h> 

#include "cache.h"
#include "core.h"
#include "bus.h"
#include "pipeline.h"
#pragma warning(disable:4996)

//Private Functions:

Tsram** initialize_tsram() {
	struct Tsram** tsram = (struct Tsram**)malloc(sizeof(struct Tsram*) * NUM_OF_BLOCKS);
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
	struct cache* cache = (struct cache*)malloc(sizeof(struct cache));
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

void cache_access(void* core_ptr, Cache* cache, void* stall_data_ptr, int trans) {
	bool tag_conflict = false;
	Core* core = (Core*)core_ptr;
	StallData* stall_data = (StallData*)stall_data_ptr;
	int address = translate_address(core->current_state_Reg->ex_mem->ALUOutput);
	int block = translate_index(address);
	int mesi_state = get_mesi_state(core->Cache, address, &tag_conflict);
	int miss_states = trans == BUSRD ? 1 : 2; // for reading, S is considered a hit
	if (mesi_state < miss_states || tag_conflict) { // cache miss
		cache->mem_stall++;
		// return if already stalled
		if (stall_data[MEMORY].active) {
			core->new_state_Reg->mem_wb->isStall = true;
			return;
		}
		// first attempt to access the data - stall pipeline and wait for memory
		stall_data[MEMORY].active = true;
		core->new_state_Reg->mem_wb->isStall = true;
		if (trans == BUSRD) cache->read_miss++;
		else cache->write_miss++;
		if (tag_conflict && mesi_state == MODIFIED) { // write modified value to memory before overwriting
			transaction* flush = (transaction*)malloc(sizeof(transaction));
			if (flush == NULL) {
				printf("An error occurred while allocating memory for bus transaction");
				exit(1); /*exiting the program after an error occured */
			}
			flush->addr = (cache->tsram[block]->tag << 8) | (address & ~TAG_MASK);
			flush->cmd = FLUSH;
			flush->flush_source_addr = cache->dsram[block];
			flush->next = NULL;
			enqueue(core->coreID, flush);
		}
		transaction* bus_trans = (transaction*)malloc(sizeof(transaction));
		if (bus_trans == NULL) {
			printf("An error occurred while allocating memory for bus transaction");
			exit(1); /*exiting the program after an error occured */
		}
		bus_trans->addr = address;
		bus_trans->cmd = trans;
		bus_trans->flush_source_addr = 0;
		bus_trans->next = NULL;
		enqueue(core->coreID, bus_trans);
	}
	else { // cache hit
		if (trans == BUSRD) {
			cache->read_hit++;
			core->new_state_Reg->mem_wb->LMD = cache->dsram[block][translate_offset(address)];
		}
		else {
			cache->write_hit++;
			cache->dsram[block][translate_offset(address)] = core->current_state_Reg->privateRegisters[core->current_state_Reg->ex_mem->IR->rd];
			cache->tsram[block]->mesi_state = MODIFIED;
		}
		if (stall_data[MEMORY].active) {
			// release pipeline in case data just returned from main memory
			stall_data[MEMORY].active = false;
			core->new_state_Reg->mem_wb->isStall = false;
		}
	}
}

void snoop(Cache* cache, int core_id) {
	bool tag_conflict = false;
	int cmd = main_bus->bus_cmd;
	if (cmd == BUSRD || cmd == BUSRDX) {
		if (core_id == main_bus->bus_origid) return;
		int mesi_state = get_mesi_state(cache, main_bus->bus_addr, &tag_conflict);
		if (!tag_conflict && mesi_state != INVALID) {
			if (mesi_state == MODIFIED) {
				transaction* flush = (transaction*)malloc(sizeof(transaction));
				if (flush == NULL) {
					printf("An error occurred while allocating memory for bus transaction");
					exit(1); /*exiting the program after an error occured */
				}
				flush->addr = main_bus->bus_addr;
				flush->cmd = FLUSH;
				flush->flush_source_addr = cache->dsram[translate_index(main_bus->bus_addr)];
				flush->next = NULL;
				main_bus->fast_pass = flush;
				main_bus->fast_pass_core_id = core_id;
			}
			if (cmd == BUSRD) main_bus->bus_shared = 1;
			cache->tsram[translate_index(main_bus->bus_addr)]->mesi_state = cmd == BUSRD ? SHARED : INVALID;
		}
	}
	else if (cmd == FLUSH) {
		if (core_id == main_bus->bus_origid && core_id == main_bus->last_served && main_bus->flush_offset == 0) {
			// this core sent a modified value to memory before overwriting - change to exclusive
			cache->tsram[translate_index(main_bus->bus_addr)]->mesi_state = EXCLUSIVE;
		}
		else if (core_id != main_bus->bus_origid && core_id == main_bus->last_served) {
			cache->dsram[translate_index(main_bus->bus_addr)][translate_offset(main_bus->bus_addr)] = main_bus->bus_data;
			if (main_bus->flush_offset == 0) {
				cache->tsram[translate_index(main_bus->bus_addr)]->tag = translate_tag(main_bus->bus_addr);
				cache->tsram[translate_index(main_bus->bus_addr)]->mesi_state = main_bus->bus_shared ? SHARED : EXCLUSIVE;
			}
		}
	}
}

void free_cache(Cache* cache) {
	for (int i = 0; i < NUM_OF_BLOCKS; i++) {
		free(cache->tsram[i]);
	}
	free(cache->tsram);
	free(cache);
}