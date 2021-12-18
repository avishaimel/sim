#pragma once

#ifndef BUS_H
#define BUS_H


#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "memory.h"
#include "cache.h"


#define NUM_OF_CORES 4
#define TRACE_FORMAT "%d %01X %01X %05X %08X\n"


typedef enum bus_state {
	NO_COMMAND,
	BUSRD,
	BUSRDX,
	FLUSH
}bus_state;

typedef enum bus_device {
	CORE_0,
	CORE_1,
	CORE_2,
	CORE_3,
	MEMORY_4
}bus_device;

typedef struct bus {
	int* main_memory;
	int*** memory_state;  //[0] is for msi operation, [1] is for storing type of opcoding in case of LL or SC
	int bus_origid;
	int bus_cmd;
	int bus_addr;
	int bus_data;
}bus;


//Functions:
bool memory_block_modified_bool(bus* main_bus, int address, int coreID);
int memory_block_modified_ID(bus* main_bus, int address);
void update_status_bus(bus* main_bus, int command, int address, int device, int data, int cycle, FILE* file, bool isPrint);
void update_memory(bus* bus, int index, int value);
void update_status_memory(bus* main_bus, int address, int coreID, int state_msi, int instruction,  bool LL_update);
void invalidate_others(bus* main_bus, int address);
bus* bus_initiation(int* main_memory);
void free_bus(struct bus* main_bus);

#endif // !BUS_H
