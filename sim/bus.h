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
#define TRACE_FORMAT "%d %01X %01X %05X %08X %d\n"


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
	MAIN_MEMORY
}bus_device;

/* Transaction structure:*/
typedef struct transaction {
	int cmd;
	int addr;
	int* flush_source_addr;
	struct transaction* next;
}transaction;

/*Transaction queue struct*/
typedef struct transaction_queue {
	transaction* first;
	transaction* last;
}transaction_queue;

/* Bus structure:*/
typedef struct bus {
	int* main_memory;
	int bus_origid;
	int bus_cmd;
	int bus_addr;
	int bus_data;
	int bus_shared;
	transaction_queue* queue[NUM_OF_CORES];
	transaction* fast_pass;
	int fast_pass_core_id;
	int last_served;
	int delay_cycles;
	int* flush_source_addr;
	int flush_offset;
	FILE* trace;
}bus;

bus* main_bus;

//Functions:
//Documentation in bus.c
void bus_initiation(int* main_memory, FILE* trace);
void enqueue(int core_index, transaction* req);
void run_bus(int cycle);
void free_bus();

#endif // !BUS_H
