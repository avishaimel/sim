#include "bus.h"
#pragma warning(disable:4996)
#define _CRT_SECURE_NO_WARNINGS


// Private Functions:

/**
 * Function that writes to the bustrace.txt
 * @param trace: file to write to
 * @param cycle: current cycle
 * @param bus_origid: bus device
 * @param bus_cmd: bus command
 * @param bus_addr: bus address
 * @param bus_data: bus data
 */
void print_to_trace(FILE* trace, int cycle, int bus_origid, int bus_cmd, int bus_addr, int bus_data, int bus_shared) {
	if (bus_cmd) fprintf(trace, TRACE_FORMAT, cycle, bus_origid, bus_cmd, bus_addr, bus_data, bus_shared);
}

/**
 * Function that finds the number of the next core in the priority for the bus access
 * The function makes sure the core that most recently got the bus will be last in the priority (round-robin)
 * @return int - the number of the next core in the priority for the bus access
 */
int round_robin_arbiter() {
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

/**
* Performs a single flush cycle
* Writes next word in the block to bus_data and updates bus_addr and flush_offset
*/
void flush_cycle() {
	main_bus->bus_data = main_bus->flush_source_addr[main_bus->flush_offset];
	if (main_bus->flush_offset == 0) main_bus->bus_addr = main_bus->bus_addr & ~OFFSET_MASK;
	else main_bus->bus_addr++;
	if (main_bus->bus_origid != MAIN_MEMORY) {
		// if a core initiated the flush, write to memory as well
		main_bus->main_memory[main_bus->bus_addr] = main_bus->bus_data;
	}
	main_bus->flush_offset = (main_bus->flush_offset + 1) % BLOCK_SIZE;
}

/**
* Starts a new bus transaction and initializes bus lines
* @param req: the request to create the transaction from
* @param origid: the tranasction originator
*/
void initiate_transaction(transaction* req, int origid) {
	if (origid == MAIN_MEMORY) { // flush from main memory - incur delay
		main_bus->delay_cycles = 15;
		main_bus->bus_cmd = NO_COMMAND;
	}
	else {
		main_bus->bus_cmd = req->cmd;
	}
	main_bus->bus_origid = origid;
	main_bus->bus_addr = req->addr;
	if (req->cmd != FLUSH) main_bus->bus_shared = 0; // only reset bus_shared on busrd/x
	main_bus->flush_source_addr = req->flush_source_addr;
	main_bus->flush_offset = 0;
	if (req->cmd == FLUSH && origid != MAIN_MEMORY) {
		flush_cycle();
	}
	free(req);
}

/**
* Removes a request from the queue of a given core. Assumes queue is not empty.
* @param core_id: the core to dequeue a request from
* @return transaction: the dequeued request
*/
transaction* dequeue_from_core(int core_id) {
	transaction* temp = main_bus->queue[core_id]->first;
	main_bus->queue[core_id]->first = temp->next;
	if (temp->next == NULL) {
		main_bus->queue[core_id]->last = NULL;
	}
	temp->next = NULL;
	return temp;
}

/**
* Finds the next transaction to be handled according to round robin and initializes it on the bus
*/
void dequeue() {
	// Round Robin policy
	int next_core = round_robin_arbiter();
	if (next_core == -1) {
		return;
	}
	initiate_transaction(dequeue_from_core(next_core), next_core);
	main_bus->last_served = next_core;
}

// Public functions:
/**
 * Function that initializes the bus and to allocates the space needed to store it
 * @param main_memory: the main memory array
 * @param file handler trace: the bus trace file
 */
void bus_initiation(int* main_memory, FILE* trace) {
	main_bus = (struct bus*)calloc(1, sizeof(struct bus));
	if (main_bus == NULL) {
		printf("An error occurred while allocating memory for bus");
		exit(1); /*exiting the program after an error occured */
	}
	main_bus->main_memory = main_memory;
	main_bus->last_served = NUM_OF_CORES - 1; // so that core0 will be first at RR when program begins
	main_bus->trace = trace;
	for (int i = 0; i < NUM_OF_CORES; ++i) {
		main_bus->queue[i] = (transaction_queue*)malloc(sizeof(transaction_queue));
		if (main_bus->queue[i] == NULL) {
			printf("An error occurred while allocating memory for bus");
			exit(1); /*exiting the program after an error occured */
		}
		main_bus->queue[i]->first = NULL;
		main_bus->queue[i]->last = NULL;
	}
}
/**
 * Function that enters a request of a bus transaction in the bus queue
 * @param core_index: the number of the core that sent the request
 * @param transaction* req: the type of the bus transaction needed
 */
void enqueue(int core_index, transaction* req) {
	if (main_bus->queue[core_index]->first == NULL) {
		main_bus->queue[core_index]->first = req;
		main_bus->queue[core_index]->last = req;
	}
	else {
		main_bus->queue[core_index]->last->next = req;
		main_bus->queue[core_index]->last = req;
	}
}

/**
* Main bus simulation function.
* At every iteration, checks if the bus is free and starts a new transaction, 
* or handles the current transaction
* @param cycle: current clock cycle, required for printing the trace
*/
void run_bus(int cycle) {
	if ((main_bus->bus_cmd == NO_COMMAND && main_bus->delay_cycles == 0) ||
		(main_bus->bus_cmd == FLUSH && main_bus->flush_offset == 0)) {
		// bus is free, give it to next core in line
		main_bus->bus_cmd = NO_COMMAND;
		dequeue();
	}
	else {
		if (main_bus->delay_cycles > 0) {
			if (--main_bus->delay_cycles == 0) {// bus waits to start Flush
				main_bus->bus_cmd = FLUSH;
				flush_cycle();
			}
		}
		else if (main_bus->bus_cmd == FLUSH) { // bus is occupied because of Flush
			flush_cycle();
		}
		else { // bus_cmd == BusRd || BusRdX
			if (main_bus->fast_pass != NULL) {
				if (main_bus->queue[main_bus->fast_pass_core_id]->first != NULL &&
					main_bus->queue[main_bus->fast_pass_core_id]->first->flush_source_addr == main_bus->fast_pass->flush_source_addr) {
					transaction* trans = dequeue_from_core(main_bus->fast_pass_core_id);
					free(trans);
				}
				initiate_transaction(main_bus->fast_pass, main_bus->fast_pass_core_id);
				main_bus->fast_pass = NULL;
			}
			else {
				transaction* mem_flush = (transaction*)malloc(sizeof(transaction));
				mem_flush->addr = main_bus->bus_addr;
				mem_flush->cmd = FLUSH;
				mem_flush->flush_source_addr = main_bus->main_memory + (main_bus->bus_addr & ~OFFSET_MASK);
				mem_flush->next = NULL;
				initiate_transaction(mem_flush, MAIN_MEMORY);
			}
		}
	}
	print_to_trace(main_bus->trace, cycle, main_bus->bus_origid, main_bus->bus_cmd, main_bus->bus_addr, main_bus->bus_data, main_bus->bus_shared);
}

/**
 * Function that frees the bus and the space allocated for it
 */
void free_bus() {
	free_memory(main_bus->main_memory);
	free(main_bus);
}