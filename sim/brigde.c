#include "bridge.h"
#include "bus.h"
#pragma warning(disable:4996)



//private functions:
void first_bus_trace(Cache_inst current_request, int coreID, int cycle) {
	int address = current_request.address;
	int state_cache = current_request.mesi_state;
	char* transaction = current_request.mesi_transaction;
	cycle += 2;
	if (!strcmp(transaction, READ) && state_cache == INVALID) {
		update_status_bus(main_bridge->main_bus, BUSRD, address, coreID, 0, cycle, main_bridge->bus_trace, true);
	}
	else if (!strcmp(transaction, WRITE) && state_cache == INVALID) {
		update_status_bus(main_bridge->main_bus, BUSRDX, address, coreID, 0, cycle, main_bridge->bus_trace, true);
	}
	else if (!strcmp(transaction, WRITE) && state_cache == SHARED) {
		update_status_bus(main_bridge->main_bus, BUSRDX, address, coreID, 0, cycle, main_bridge->bus_trace, true);
	}

	return;
}

void update_stall_list(int coreID, bool zeroAll) {
	if (zeroAll) {
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 2; j++) {
				main_bridge->stalls[i][j] = 0;
			}
		}
	}
	else {
		for (int i = 0; i < 4; i++) {
			if (i == coreID)
				main_bridge->stalls[i][0] = 1;
			else
				main_bridge->stalls[i][0] = -1;
		}
	}
}

bool check_if_bus_free() {
	bool result = true;
	for (int i = 0; i < 4; i++) {
		if (main_bridge->stalls[i][0] == 0)
			continue;
		else {
			result = false;
			break;
		}
	}
	return result;

}

int get_address(int tag, int index) {
	return (tag << 8) + index;
}

// Public Functions:
void bridge_initiation(FILE* bus_trace, Cache* Cache0, Cache* Cache1, Cache* Cache2, Cache* Cache3, bus* main_bus) {
	main_bridge->cache_array[0] = Cache0;
	main_bridge->cache_array[1] = Cache1;
	main_bridge->cache_array[2] = Cache2;
	main_bridge->cache_array[3] = Cache3;
	main_bridge->main_bus = main_bus;
	main_bridge->bus_trace = bus_trace;
	main_bridge->stalls = (int**)malloc(NUM_OF_CORES * sizeof(int*));
	if (main_bridge->stalls == NULL) {
		printf("An error occurred while allocating memory for the bridge stall");
		exit(1); /*exiting the program after an error occured */
	}
	for (int i = 0; i < NUM_OF_CORES; i++) {
		main_bridge->stalls[i] = (int*)malloc(2 * sizeof(int));
		if (main_bus->memory_state[i] == NULL) {
			printf("An error occurred while allocating memory for the bus");
			exit(1); /*exiting the program after an error occured */
		}
		for (int j = 0; j < 2; j++) {
			main_bridge->stalls[i][j] = 0;
		}
	}
}

void update_tsram(Cache* cache, Cache_inst current_request, int new_state) {
	cache->tsram[current_request.index]->mesi_state = new_state;
	cache->tsram[current_request.index]->tag = current_request.tag;
}

int update_from_other_core(int index, int coreID) {
	int modified_data = main_bridge->cache_array[coreID]->dsram[index];
	return modified_data;
}

void update_status_cache(int coreID, int index, int state) {
	main_bridge->cache_array[coreID]->tsram[index]->mesi_state = state;
}

int connection_between_bus_and_cache(Cache_inst current_request, int coreID, int cycle, int* stall_counter, int tagConflict) {
	int address = current_request.address;
	int state_cache = current_request.mesi_state;
	char* transaction = current_request.mesi_transaction;
	int instruction = current_request.opcode;
	int result = 0;
	int index = current_request.index;
	cycle += 1; //We simulates that our bus is a FF
	if (!strcmp(transaction, READ) && state_cache == INVALID) {
		//update_status_bus(main_bridge->main_bus,BUSRD,address,coreID,0,cycle, main_bridge->bustrace,true);
		if (memory_block_modified_bool(main_bridge->main_bus, address, coreID)) {
			*stall_counter += 2;
			cycle++;
			int modifiedID = memory_block_modified_ID(main_bridge->main_bus, address);
			if (modifiedID == 4) {
				printf("Error in memory_block_modified_ID \n");
				exit(1);
			}
			int modified_data = update_from_other_core(index, modifiedID);
			result = modified_data;
			update_status_bus(main_bridge->main_bus, FLUSH, address, modifiedID, modified_data, cycle - 3, main_bridge->bus_trace, true);
			update_memory(main_bridge->main_bus, address, modified_data);
			update_status_memory(main_bridge->main_bus, address, modifiedID, SHARED, instruction,  false);
			update_status_cache(modifiedID, index, SHARED);
		}
		else {
			if (tagConflict) {
				int previousTag = main_bridge->cache_array[coreID]->tsram[current_request.index]->tag;
				int dataToFlush = main_bridge->cache_array[coreID]->dsram[current_request.index];
				int addressToFlush = get_address(previousTag, current_request.index);
				update_memory(main_bridge->main_bus, addressToFlush, dataToFlush);
				update_status_memory(main_bridge->main_bus, address, coreID, INVALID, instruction,  false);
				update_status_cache(coreID, index, INVALID);
			}
			*stall_counter += 65; //2 cycles for bus and 64 to get from memory
			update_status_bus(main_bridge->main_bus, FLUSH, address, MEMORY_4, main_bridge->main_bus->main_memory[address], cycle - 2, main_bridge->bus_trace, true);
		}
		update_status_memory(main_bridge->main_bus, address, coreID, SHARED, instruction,  true);
		result = main_bridge->main_bus->bus_data;
	}
	else if ((!strcmp(transaction, READ) && state_cache == SHARED) || (!strcmp(transaction, READ) && state_cache == MODIFIED)) {
		int bus_data = main_bridge->cache_array[coreID]->dsram[current_request.index];
		update_status_bus(main_bridge->main_bus, BUSRD, address, coreID, bus_data, cycle, main_bridge->bus_trace, false);
		update_status_memory(main_bridge->main_bus, address, coreID, state_cache, instruction,  true);
		result = main_bridge->main_bus->bus_data;
	}
	else if (!strcmp(transaction, WRITE) && state_cache == MODIFIED) {
		update_status_bus(main_bridge->main_bus, BUSRDX, address, coreID, 0, cycle, main_bridge->bus_trace, false);
		update_status_memory(main_bridge->main_bus, address, coreID, state_cache, instruction,  true);
		result = main_bridge->main_bus->bus_data;
	}
	else if (!strcmp(transaction, WRITE) && state_cache == INVALID) {
		//update_status_bus(main_bridge->main_bus,BUSRDX,address,coreID,0,cycle, main_bridge->bustrace,true);
		if (memory_block_modified_bool(main_bridge->main_bus, address, coreID)) {
			*stall_counter += 2;
			cycle++;
			int modifiedID = memory_block_modified_ID(main_bridge->main_bus, address);
			int modified_data = update_from_other_core(index, modifiedID);
			update_status_bus(main_bridge->main_bus, FLUSH, address, modifiedID, modified_data, cycle - 3, main_bridge->bus_trace, true);
			update_memory(main_bridge->main_bus, address, modified_data);
			update_status_memory(main_bridge->main_bus, address, modifiedID, INVALID, instruction,  false);
			update_status_cache(modifiedID, index, INVALID);
		}
		else {
			if (tagConflict) {
				int previousTag = main_bridge->cache_array[coreID]->tsram[current_request.index]->tag;
				int dataToFlush = main_bridge->cache_array[coreID]->dsram[current_request.index];
				int addressToFlush = get_address(previousTag, current_request.index);
				if (dataToFlush == 7) {
					int h = 0;
				}
				update_memory(main_bridge->main_bus, addressToFlush, dataToFlush);
				update_status_memory(main_bridge->main_bus, address, coreID, INVALID, instruction,  false);
				update_status_cache(coreID, index, INVALID);
			}
			invalidate_others(main_bridge->main_bus, address);
			for (int i = 0; i < NUM_OF_CORES; i++) {
				update_status_cache(i, index, INVALID);
			}

			*stall_counter += 65; //2 cycles for bus and 64 to get from memory
			update_status_bus(main_bridge->main_bus, FLUSH, address, MEMORY_4, main_bridge->main_bus->main_memory[address], cycle - 2, main_bridge->bus_trace, true);
		}
		update_status_memory(main_bridge->main_bus, address, coreID, MODIFIED, instruction,  true);
		result = main_bridge->main_bus->bus_data;
	}
	else if (!strcmp(transaction, WRITE) && state_cache == SHARED) {
		//update_status_bus(main_bridge->main_bus,BUSRDX,address,coreID,0,cycle, main_bridge->bustrace,true);
		invalidate_others(main_bridge->main_bus, address);
		for (int i = 0; i < NUM_OF_CORES; i++) {
			update_status_cache(i, index, INVALID);
		}

		*stall_counter += 65; //2 cycles for bus and 64 to get from memory
		update_status_bus(main_bridge->main_bus, FLUSH, address, MEMORY_4, main_bridge->main_bus->main_memory[address], cycle - 2, main_bridge->bus_trace, true);
		update_status_memory(main_bridge->main_bus, address, coreID, MODIFIED, instruction,  true);
		result = main_bridge->main_bus->bus_data;
	}

	else {
		printf("Error_Bridge_1: Parameters given are invalid");
		exit(1);
	}

	return result;

}


int cycles_to_wait(Cache_inst current_request, int coreID) {
	int address = current_request.address;
	int state_cache = current_request.mesi_state;
	char* transaction = current_request.mesi_transaction;
	int result = 0;
	if (!strcmp(transaction, READ) && state_cache == INVALID) {
		if (memory_block_modified_bool(main_bridge->main_bus, address, coreID)) {
			result = 3;
		}
		else {
			result = 66; //2 cycles for bus and 64 to get from memory
		}
	}
	else if (!strcmp(transaction, WRITE) && state_cache == INVALID) {
		if (memory_block_modified_bool(main_bridge->main_bus, address, coreID)) {
			result = 3;
		}
		else {
			result = 66; //2 cycles for bus and 64 to get from memory
		}
	}
	else if (!strcmp(transaction, WRITE) && state_cache == SHARED) {
		result = 66; //2 cycles for bus and 64 to get from memory
	}

	return result;
}

int handle_request(struct cache* cache, Cache_inst current_request, int coreID, int cycle, int* stall_counter, int tagConflict) {
	int data_from_pipe = 0;
	int data_to_return = 0;
	char* operation = current_request.mesi_transaction;
	int current_state = current_request.mesi_state;
	int new_state;
	cycle++;
	if (!strcmp(operation, READ)) {
		switch (current_state) {
		case SHARED:
			data_to_return = cache->dsram[current_request.index];
			data_from_pipe = connection_between_bus_and_cache(current_request, coreID, cycle, stall_counter, tagConflict);
			new_state = mesi_state_machine(operation, current_state);
			update_tsram(cache, current_request, new_state);
			break;
		case MODIFIED:
			data_to_return = cache->dsram[current_request.index];
			data_from_pipe = connection_between_bus_and_cache(current_request, coreID, cycle, stall_counter, tagConflict);
			new_state = mesi_state_machine(operation, current_state);
			cache->tsram[current_request.index]->mesi_state = new_state;
			break;
		case INVALID:
			*stall_counter = *stall_counter + 1;
			data_from_pipe = connection_between_bus_and_cache(current_request, coreID, cycle, stall_counter, tagConflict);
			cache->dsram[current_request.index] = data_from_pipe;
			data_to_return = cache->dsram[current_request.index];
			new_state = mesi_state_machine(operation, current_state);
			update_tsram(cache, current_request, new_state);
			break;
		}
	}
	else if (!strcmp(operation, WRITE)) {
		switch (current_state) {
		case INVALID:
			*stall_counter = *stall_counter + 1;
			data_from_pipe = connection_between_bus_and_cache(current_request, coreID, cycle, stall_counter, tagConflict);
			cache->dsram[current_request.index] = current_request.rd;
			data_to_return = cache->dsram[current_request.index];
			new_state = mesi_state_machine(operation, current_state);
			update_tsram(cache, current_request, new_state);
			break;
		case SHARED:
			*stall_counter = *stall_counter + 1;
			data_from_pipe = connection_between_bus_and_cache(current_request, coreID, cycle, stall_counter, tagConflict);
			cache->dsram[current_request.index] = current_request.rd;
			data_to_return = cache->dsram[current_request.index];
			new_state = mesi_state_machine(operation, current_state);
			update_tsram(cache, current_request, new_state);
			break;
		case MODIFIED:
			data_from_pipe = connection_between_bus_and_cache(current_request, coreID, cycle, stall_counter, tagConflict);
			cache->dsram[current_request.index] = current_request.rd;
			data_to_return = cache->dsram[current_request.index];
			new_state = mesi_state_machine(operation, current_state);
			update_tsram(cache, current_request, new_state);
			break;
		}
	}
	else {
		printf("Error_Bridge_2: Operation given to cache is not legal");
		exit(1);
	}

	return data_to_return;
}

// First we need to write all bus and cache functions
int request(Cache* cache, int opcode, int address, int rd_value, int coreID, int cycle, bool* isStall) {
	int result = 0;
	int stall_counter = 0;
	bool tagConflict = false;
	if ((opcode != LW) && !(opcode == SW)) {
		printf("Error_Bridge _3 : Trying to access cache with invalid instruction");
		return 1;
	}
	current_request.coreID = coreID;
	current_request.opcode = opcode;
	current_request.rd = rd_value;
	current_request.address = translate_address(address);
	current_request.tag = translate_tag(current_request.address);
	current_request.index = translate_index(current_request.address);
	current_request.mesi_transaction = translate_mesi_transaction(opcode);
	current_request.mesi_state = get_mesi_state(cache, current_request.index, current_request.tag, &tagConflict);


	int stall = cycles_to_wait(current_request, coreID);

	if (stall != 0) {
		*isStall = true;
		if (check_if_bus_free()) {
			update_stall_list(coreID, false);
			first_bus_trace(current_request, coreID, cycle);
			main_bridge->stalls[coreID][1] += stall;
			return 0;
		}
		else {
			if (main_bridge->stalls[coreID][0] == 1) {
				main_bridge->stalls[coreID][1]--;
				if (main_bridge->stalls[coreID][1] != 0) {
					return 0;
				}
			}
			else {
				main_bridge->stalls[coreID][1] = -1;
				return 0;
			}
		}
	}

	result = handle_request(cache, current_request, coreID, cycle, &stall_counter, tagConflict);
	if (main_bridge->stalls[coreID][0] == 1) {
		update_stall_list(coreID, true);
	}

	*isStall = false;

	if (!strcmp(current_request.mesi_transaction, READ)) {
		if (stall_counter == 0) {
			cache->read_hit++;
		}
		else {
			cache->read_miss++;
			cache->mem_stall += stall_counter;
		}
	}
	else {
		if (stall_counter == 0) {
			cache->write_hit++;
		}
		else {
			cache->write_miss++;
			cache->mem_stall += stall_counter;
		}
	}
	return result;

}

void free_bridge(Bridge* main_bridge) {
	for (int i = 0; i < NUM_OF_CORES; i++)
		free(main_bridge->stalls[i]);
	free(main_bridge->stalls);
	free(main_bridge);
}