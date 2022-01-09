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
/**
 * Function that initializes tsram and allocates the space needed to store it
 * @return Tsram** struct initialized tsram
 */
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

/**
 * Function that initializes the cache and allocates the space needed to store it
 * @param coreID: the ID of core that wants to initialize cache
 * @return Cache struct - initialized cache
 */
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
/**
 * Function that translates the address according to the space of main memory (20 bits)
 * @param address: address to translate
 * @return int - the right address
 */
int translate_address(int address) {
	return address & LEFT_MOST_20;
}
/**
 * Function that extracts the current tag
 * @param address: address from which to extract tag
 * @return int - current tag
 */
int translate_tag(int address) {
	return (address & TAG_MASK) >> 8;
}
/**
 * Function to extract current index
 * @param address: address from which to extract index
 * @return int - current index
 */
int translate_index(int address) {
	return (address & BLOCK_MASK) >> 2;
}
/**
 * Function to extract current offset
 * @param address: address from which to extract offset
 * @return int - current offset
 */
int translate_offset(int address) {
	return address & OFFSET_MASK;
}

/**
 * Function to get the mesi state
 * @param cache: current cache
 * @param address - the address to check the mesi state of
 * @param tagConflict: boolean to check if tag conflict is present
 * @return int current mesi state
 */
int get_mesi_state(Cache* cache, int address, bool* tag_conflict) {
	int block = translate_index(address);
	if (cache->tsram[block]->tag != translate_tag(address)) {
		*tag_conflict = true;
	}
	return cache->tsram[block]->mesi_state;
}

/**
 * Function that manages the access to the cache
 * if there is cache hit the functions updates the LMD (READ) or the dsram (WRITE) with the correct data
 * if there is cache miss the function uses the enqueue function to put the request in the bus access queue
 * The function also updates the mesi state accordingly
 * @param core_ptr - pointer to the current core
 * @param cache: current cache
 * @param stall_data_pointer - pointer to the stall data to use
 * @param int trans: the needed bus transaction
 */
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
		if (trans == BUSRD) cache->read_miss++; // lw inst -> read miss
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

/**
 * Function that implements the snoop protocol to ensure cache coherency 
 * After runBus(), the function updates each core's cache according to current bus transaction
 * @param cache: current cache
 * @param int core_id: the current core number
 */
void snoop(Cache* cache, int core_id) {
	bool tag_conflict = false;
	int cmd = main_bus->bus_cmd;
	if (cmd == BUSRD || cmd == BUSRDX) {
		if (core_id == main_bus->bus_origid) return;
		int mesi_state = get_mesi_state(cache, main_bus->bus_addr, &tag_conflict);
		if (!tag_conflict && mesi_state != INVALID) {
			if (mesi_state == MODIFIED) { // another core is in Modified- Flush
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
			if (cmd == BUSRD) main_bus->bus_shared = 1; // BusRd -> change to Shared
			cache->tsram[translate_index(main_bus->bus_addr)]->mesi_state = cmd == BUSRD ? SHARED : INVALID; // update Mesi state (snoop)
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

/**
 * Function that frees the cache and the space allocated for it
 * @param cache: cache to be freed
 */
void free_cache(Cache* cache) {
	for (int i = 0; i < NUM_OF_BLOCKS; i++) {
		free(cache->tsram[i]);
	}
	free(cache->tsram);
	free(cache);
}