#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "core.h"

/**
 * Function that initializes the core statistics
 * @param core - the current core
 */
void initiate_cote_stats(Core* core) {
	core->coreStatistics.cycles = 0;
	core->coreStatistics.decode_stall = 0;
	core->coreStatistics.instructions = 0;
}

/**
 * Function that initializes the core parameters
 * @param core - the core to be initialized
 */
void Core_status_initiation(Core* current_core) {
	current_core->state.doFetch = true;
	current_core->state.doDecode = current_core->state.doExecute = current_core->state.doMemory = current_core->state.doWriteBack = false;
	current_core->state.fetchExecuted = current_core->state.decodeExecuted = current_core->state.executeExecuted = current_core->state.memoryExecuted = current_core->state.writeBackExecuted = false;
	current_core->state.PC = 0;
	current_core->state.haltPipeline = false;
	current_core->state.haltProgram = false;
	current_core->state.haltNextOperation = false;
	current_core->state.isDelaySlot = false;
	for (int i = 0; i < PIPLINE_LENGTH; i++) {//init stall data
		current_core->stallData[i].active = false;
		current_core->stallData[i].cyclesToStall = 0;
	}
}

/**
 * Function that initializes the core private and pipeline registers
 * @return CoreRegisters object with the initialized registers
 */
CoreRegisters* Core_Registers_Initiation() {
	CoreRegisters* registers = NULL;
	IF_ID* if_id = NULL;
	ID_EX* id_ex = NULL;
	EX_MEM* ex_mem = NULL;
	MEM_WB* mem_wb = NULL;

	registers = (CoreRegisters *)calloc(1, sizeof(CoreRegisters));
	if (registers == NULL) {
		printf("Un error occurred while allocating memory for the core registers");
		exit(1); /*an error occured - exiting */
	}

	// initialize pipeline registers
	if_id = calloc(1, sizeof(IF_ID));
	id_ex = calloc(1, sizeof(ID_EX));
	ex_mem = calloc(1, sizeof(EX_MEM));
	mem_wb = calloc(1, sizeof(MEM_WB));
	if (if_id == NULL || id_ex == NULL || ex_mem == NULL || mem_wb == NULL) {
		printf("Un error occurred while allocating memory for the pipeline registers");
		exit(1);/*exiting the program after an error occured */
	}
	registers->if_id = if_id;
	registers->id_ex = id_ex;
	registers->ex_mem = ex_mem;
	registers->mem_wb = mem_wb;

	return registers;
}

/**
 * Function that initializes the cores parameters such as registers, statistics etc.
 * @param cache_array - the caches of of each core
 * @return Core* object - the initialized cores
 */
Core* cores_initiation(Cache** cache_array) {
	Core* cores = NULL;
	cores = (Core *)malloc(sizeof(Core) * CORE_NUM);
	if (cores == NULL) {
		printf("An error occurred while allocating memory for the cores");
		exit(1); /*exiting the program after an error occured */
	}

	for (int core_idx = 0; core_idx < CORE_NUM; core_idx++) {
		// Init core and core registers
		cores[core_idx].coreID = core_idx;
		cores[core_idx].new_state_Reg = Core_Registers_Initiation();
		cores[core_idx].current_state_Reg = Core_Registers_Initiation();
		cores[core_idx].Cache = cache_array[core_idx];
		Core_status_initiation(&(cores[core_idx]));
		initiate_cote_stats(&(cores[core_idx]));
		cores[core_idx].new_state_Reg->if_id->New_PC = 1;

		memset(cores[core_idx].instructions, 0, INSTRUCTIONS_SIZE * sizeof(Instruction)); //? - no defenition
	}

	return cores;
}

/**
 * Function that frees all the core registers
 * @param registers - the current private and pipeling registers of the core
 */
void freeCoreRegisters(CoreRegisters* registers)
{
	free(registers->if_id);
	free(registers->id_ex);
	free(registers->ex_mem);
	free(registers->mem_wb);
}

/**
 * Function that frees all the cores objects
 * @param core - the cores objects
 */
void free_Cores(Core* cores)
{
	for (int i = 0; i < CORE_NUM; i++) {
		freeCoreRegisters(cores[i].current_state_Reg);
		freeCoreRegisters(cores[i].new_state_Reg);
		free(cores[i].new_state_Reg);
		free(cores[i].current_state_Reg);
		free_cache(cores[i].Cache); /*freeing the cache object of the core */
	}
	free(cores);
}

/**
 * Function that writes the current cycle of the execution to the core trace
 * @param file handler trace - the relevant core trace file to write to
 * @param Core core - the current core 
 * @param int cycleNumber - the current cycle number
 */
void write_Core_Trace(FILE* trace, Core* core, int cycleNumber) {
	if (!core->state.fetchExecuted)
		fprintf(trace, "%d %s ", cycleNumber, "---");/*write --- if the fetch stage is in stall */
	else {
		fprintf(trace, "%d %03X ", cycleNumber, core->state.PC);
	}
	if (!core->state.decodeExecuted)
		fprintf(trace, "%s ", "---");/*write --- if the decode stage is in stall */
	else {
		int decode = core->current_state_Reg->if_id->PC;
		fprintf(trace, "%03X ", decode);
	}
	if (!core->state.executeExecuted)
		fprintf(trace, "%s ", "---");/*write --- if the execute stage is in stall */
	else {
		int execute = core->current_state_Reg->id_ex->PC;
		fprintf(trace, "%03X ", execute);
	}
	if (!core->state.memoryExecuted)
		fprintf(trace, "%s ", "---");/*write --- if the memory stage is in stall */
	else {
		int memory = core->current_state_Reg->ex_mem->PC;
		fprintf(trace, "%03X ", memory);
	}
	if (!core->state.writeBackExecuted)
		fprintf(trace, "%s ", "---");/*write --- if the write back stage is in stall */
	else {
		int writeBack = core->current_state_Reg->mem_wb->PC;
		fprintf(trace, "%03X ", writeBack);
	}
	for (int i = 2; i < NUM_OF_REGISTERS; i++) {/*write all the registers except R0 and R1*/
		if (i != 15)
			fprintf(trace, "%08X ", core->current_state_Reg->privateRegisters[i]);
		else
			fprintf(trace, "%08X \n", core->current_state_Reg->privateRegisters[i]);
	}
}