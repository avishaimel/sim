#include "bus.h"
#pragma warning(disable:4996)
#define _CRT_SECURE_NO_WARNINGS


// Private Functions:

void print_to_trace(FILE* trace, int cycle, int bus_origid, int bus_cmd, int bus_addr, int bus_data, int bus_shared) {
	fprintf(trace, TRACE_FORMAT, cycle, bus_origid, bus_cmd, bus_addr, bus_data, bus_shared);
}

int round_robin_arbiter(bus* main_bus) {
	int next = (main_bus->last_served + 1) % NUM_OF_CORES;
	while (next != main_bus->last_served) {
		if (main_bus->queue[next]->first != NULL) {
			return next;
		}
		next = (next + 1) % NUM_OF_CORES;
	}
	if (main_bus->queue[next]->first != NULL) {
		return next;
	}
	// No core requires bus, return -1
	return -1;
}

void flush_cycle(bus* main_bus) {
	main_bus->bus_data = main_bus->flush_source_addr[main_bus->flush_offset];
	main_bus->flush_offset = (main_bus->flush_offset + 1) % BLOCK_SIZE;
}

void initiate_transaction(bus* main_bus, request* req, int origid) {
	if (origid == 4) { // flush from main memory - incur delay
		main_bus->delay_cycles = 15;
	}
	else {
		main_bus->bus_cmd = req->cmd;
	}
	main_bus->bus_origid = origid;
	main_bus->bus_addr = req->addr;
	main_bus->bus_shared = 0;
	main_bus->flush_source_addr = req->flush_source_addr;
	main_bus->flush_offset = 0;
	if (req->cmd == FLUSH && origid != 4) {
		flush_cycle(main_bus);
	}
	free(req);
}

// Public functions:

bus* bus_initiation(int* main_memory) {
	bus* main_bus = (struct bus*)malloc(sizeof(struct bus));
	if (main_bus == NULL) {
		printf("An error occurred while allocating memory for bus");
		exit(1); /*exiting the program after an error occured */
	}
	main_bus->main_memory = main_memory;
	main_bus->memory_state = (int**)malloc(MAX_WORDS * sizeof(int*));
	if (main_bus->memory_state == NULL) {
		printf("An error occurred while allocating memory for memory state");
		exit(1); /*exiting the program after an error occured */
	}
	main_bus->bus_origid = 0;
	main_bus->bus_cmd = 0;
	main_bus->bus_addr = 0;
	main_bus->bus_data = 0;
	main_bus->bus_shared = 0;
	main_bus->last_served = 0;
	main_bus->last_cmd = 0;
	main_bus->delay_cycles = 0;
	main_bus->flush_source_addr = NULL;
	main_bus->flush_offset = 0;
	for (int i = 0; i < MAX_WORDS; i++) {
		main_bus->memory_state[i] = (int*)malloc(NUM_OF_CORES * sizeof(int));
		if (main_bus->memory_state[i] == NULL) {
			printf("An error occurred while allocating memory for the bus");
			exit(1); /*exiting the program after an error occured */
		}
		for (int j = 0; j < NUM_OF_CORES; j++) {
			main_bus->memory_state[i][j] = 0;
		}
	}
	for (int i = 0; i < NUM_OF_CORES; ++i) {
		main_bus->queue[i] = (request_queue*)malloc(sizeof(request_queue));
		main_bus->queue[i]->first = NULL;
		main_bus->queue[i]->last = NULL;
	}
	return main_bus;
}

bool memory_block_modified_bool(struct bus* main_bus, int address, int coreID) {
	int* memory_row = main_bus->memory_state[address];
	for (int i = 0; i < NUM_OF_CORES; i++) {
		if (i == coreID) {
			continue;
		}
		if (memory_row[i] == MODIFIED) {
			return true;
		}
	}
	return false;
}

bool memory_block_shared_bool(struct bus* main_bus, int address, int coreID) {
	int* memory_row = main_bus->memory_state[address];
	for (int i = 0; i < NUM_OF_CORES; i++) {
		if (i == coreID) {
			continue;
		}
		if (memory_row[i] == SHARED) {
			return true;
		}
	}
	return false;
}

int memory_block_modified_ID(bus* main_bus, int address) {
	int result = 4;
	int* memory_row = main_bus->memory_state[address];
	for (int i = 0; i < NUM_OF_CORES; i++) {
		if (memory_row[i] == MODIFIED) {
			result = i;
			break;
		}
	}
	return result;
}

void update_status_bus(bus* main_bus, int command, int address, int device, int data, int cycle, FILE* file, bool isPrint) {
	main_bus->bus_origid = device;
	main_bus->bus_addr = address;
	main_bus->bus_cmd = command;
	if (command == BUSRDX) {
		main_bus->bus_data = 0;
	}
	else {
		main_bus->bus_data = data;
	}
	if ((command == BUSRD)) {
		main_bus->bus_shared = 0;
	}
	if (memory_block_shared_bool(main_bus, address, device))
			main_bus->bus_shared = 1;

	if (isPrint) {
		print_to_trace(file, cycle, main_bus->bus_origid, main_bus->bus_cmd, main_bus->bus_addr, main_bus->bus_data, main_bus->bus_shared);
	}
}

void update_memory(bus* bus, int index, int value) {
	bus->main_memory[index] = value;
}


void update_status_memory(bus* main_bus, int address, int coreID, int state_mesi, int instruction, bool LL_update) {
	main_bus->memory_state[address][coreID] = state_mesi;
}

void invalidate_others(bus* main_bus, int address) {
	int* memory_row = main_bus->memory_state[address];
	for (int i = 0; i < NUM_OF_CORES; i++) {
		memory_row[i] = INVALID;
	}
}

void enqueue(bus* main_bus, int core_index, request* req) {
	if (main_bus->queue[core_index]->first == NULL) {
		main_bus->queue[core_index]->first = req;
		main_bus->queue[core_index]->last = req;
	}
	else {
		main_bus->queue[core_index]->last->next = req;
		main_bus->queue[core_index]->last = req;
	}
}

void dequeue(bus* main_bus, int core_index, request* req) {
	// Round Robin policy
	int next_core = round_robin_arbiter(main_bus);
	if (next_core == -1) {
		return;
	}
	request* temp = main_bus->queue[next_core]->first;
	main_bus->queue[next_core]->first = temp->next;
	if (temp->next == NULL) {
		main_bus->queue[next_core]->last = NULL;
	}
	temp->next = NULL;
	initiate_transaction(main_bus, temp, next_core);
	main_bus->last_served = next_core;
}

void free_bus(struct bus* main_bus) {
	free_memory(main_bus->main_memory);
	for (int i = 0; i < MAX_WORDS; i++) {
			free(main_bus->memory_state[i]);
		}
	
	free(main_bus->memory_state);
	free(main_bus);
}