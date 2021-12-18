#include "bus.h"
#pragma warning(disable:4996)




// Private Functions:

void print_to_trace(FILE* trace, int cycle, int bus_origid, int bus_cmd, int bus_addr, int bus_data) {
	fprintf(trace, TRACE_FORMAT, cycle, bus_origid, bus_cmd, bus_addr, bus_data);
}

// Public functions:

bus* bus_initiation(int* main_memory) {
	bus* main_bus = (struct bus*)malloc(sizeof(struct bus));
	if (main_bus == NULL) {
		printf("An error occurred while allocating memory for bus");
		exit(1); /*exiting the program after an error occured */
	}
	main_bus->main_memory = main_memory;
	main_bus->memory_state = (int***)malloc(MAX_WORDS * sizeof(int**));
	if (main_bus->memory_state == NULL) {
		printf("An error occurred while allocating memory for memory state");
		exit(1); /*exiting the program after an error occured */
	}
	main_bus->bus_origid = 0;
	main_bus->bus_cmd = 0;
	main_bus->bus_addr = 0;
	main_bus->bus_data = 0;
	for (int i = 0; i < MAX_WORDS; i++) {
		main_bus->memory_state[i] = (int**)malloc(NUM_OF_CORES * sizeof(int*));
		if (main_bus->memory_state[i] == NULL) {
			printf("An error occurred while allocating memory for the bus");
			exit(1); /*exiting the program after an error occured */
		}
		for (int j = 0; j < NUM_OF_CORES; j++) {
			main_bus->memory_state[i][j] = (int*)malloc(2 * sizeof(int));
			if (main_bus->memory_state[i][j] == NULL) {
				printf("An error occurred while allocating memory for the bus");
				exit(1); /*exiting the program after an error occured */
			}
			for (int g = 0; g < 2; g++)
				main_bus->memory_state[i][j][g] = 0;
		}
	}
	return main_bus;
}

bool memory_block_modified_bool(struct bus* main_bus, int address, int coreID) {
	bool result = false;
	int** memory_row = main_bus->memory_state[address];
	for (int i = 0; i < NUM_OF_CORES; i++) {
		if (i == coreID) {
			continue;
		}
		if (memory_row[i][0] == MODIFIED) {
			result = true;
			break;
		}
	}
	return result;
}

int memory_block_modified_ID(bus* main_bus, int address) {
	int result = 4;
	int** memory_row = main_bus->memory_state[address];
	for (int i = 0; i < NUM_OF_CORES; i++) {
		if (memory_row[i][0] == MODIFIED) {
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
	if (isPrint) {
		print_to_trace(file, cycle, main_bus->bus_origid, main_bus->bus_cmd, main_bus->bus_addr, main_bus->bus_data);
	}
}

void update_memory(bus* bus, int index, int value) {
	bus->main_memory[index] = value;
}


void update_status_memory(bus* main_bus, int address, int coreID, int state_msi, int instruction, bool LL_update) {
	int state = main_bus->memory_state[address][coreID][0] = state_msi;
	if (state == 0) {
		main_bus->memory_state[address][coreID][1] = 0;
		return;
	}
	if (instruction == 18 && LL_update) //LL
		main_bus->memory_state[address][coreID][1] = 1; //1== LL succeeded, in case it's not true I change it afterwards
	else if (instruction == 16 && LL_update) //LW
		main_bus->memory_state[address][coreID][1] = 0;
	
}

void invalidate_others(bus* main_bus, int address) {
	int** memory_row = main_bus->memory_state[address];
	for (int i = 0; i < NUM_OF_CORES; i++) {
		memory_row[i][0] = INVALID;
	}
}

void free_bus(struct bus* main_bus) {
	free_memory(main_bus->main_memory);
	for (int i = 0; i < MAX_WORDS; i++) {
		for (int j = 0; j < NUM_OF_CORES; j++) {
			free(main_bus->memory_state[i][j]);
		}
		free(main_bus->memory_state[i]);
	}
	free(main_bus->memory_state);
	free(main_bus);
}