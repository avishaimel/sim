#pragma once 

#include <stdbool.h>
#include "pipeline.h"


//Private functions:

/**
 * Function that extends the sign of the immediate in each instruction
 * @param immediate: the immediate value of the instruction to sign extend
 * @return int sign extended immediate
 */
int sign_extention(int immediate) {
	immediate <<= SIGN_EXTEND_NUM;
	immediate >>= SIGN_EXTEND_NUM;
	return immediate;
}

/**
 * Functions that checks whether an opcode is a branch opcode or not
 * @param opcode: opcode
 * @return bool
*/
bool isBranchOpcode(int opcode)
{
	return opcode >= 9 && opcode <= 15;
}

/**
 * Function that executes branch resolution - 
 * The functions calculates the result of each part of the branch condition and compares them
 * according to the current operation.
 * Then, the function updates the New_PC (the PC of the next operation):
 * - if the jump is taken it will be the immediate of the instruction. 
 * - if not the New_PC will be PC + 1.
 * If the jump is taken the delaySlotPC is also updated and the state is changed to be delaySlot, since the old PC
 * that is currently in fetch stage is wrong
 * if the instruction is a jal instruction, PC + 1 is stored in R15 for return
 * @param core: pointer to a Core object
 */
void branchResolution(Core* core) {
	bool isJump = false;
	bool isLink = false;
	int left_condition_reg_val;
	int right_condition_reg_val;
	/*calculating the two variables which we compare in jump instructions*/
	int immidiate_value = sign_extention(core->current_state_Reg->if_id->IR->imm);
	right_condition_reg_val = core->current_state_Reg->if_id->IR->rt == 1 ? immidiate_value : core->current_state_Reg->privateRegisters[core->current_state_Reg->if_id->IR->rt];
	left_condition_reg_val = core->current_state_Reg->if_id->IR->rs == 1 ? immidiate_value : core->current_state_Reg->privateRegisters[core->current_state_Reg->if_id->IR->rs];
	/*Branch Resolution*/
	if (isBranchOpcode(core->current_state_Reg->if_id->IR->opcode) || core->current_state_Reg->if_id->IR->opcode == 15) {/*jump instructions*/
		if (core->current_state_Reg->if_id->IR->opcode == 9) /*BEQ*/
			isJump = left_condition_reg_val == right_condition_reg_val;
		else if (core->current_state_Reg->if_id->IR->opcode == 10)/*BNE*/
			isJump = left_condition_reg_val != right_condition_reg_val;
		else if (core->current_state_Reg->if_id->IR->opcode == 11)/*BLT*/
			isJump = left_condition_reg_val < right_condition_reg_val;
		else if (core->current_state_Reg->if_id->IR->opcode == 12)/*BGT*/
			isJump = left_condition_reg_val > right_condition_reg_val;
		else if (core->current_state_Reg->if_id->IR->opcode == 13)/*BLE*/
			isJump = left_condition_reg_val <= right_condition_reg_val;
		else if (core->current_state_Reg->if_id->IR->opcode == 14)/*BGE*/
			isJump = left_condition_reg_val >= right_condition_reg_val;
		else if (core->current_state_Reg->if_id->IR->opcode == 15) {/*JAL*/
			isJump = true;
			isLink = true;
		}
	}
	if (core->state.isDelaySlot) {
		core->new_state_Reg->if_id->New_PC = core->new_state_Reg->if_id->delaySlotPC;
		core->state.isDelaySlot = false;
	}
	if (isJump) {
		if (isLink) {
			core->new_state_Reg->privateRegisters[15] = core->state.PC + 1;
		}
		int newPC = core->current_state_Reg->if_id->IR->rd == 1 ?
			immidiate_value & 0x000003FF :
			core->current_state_Reg->privateRegisters[core->current_state_Reg->if_id->IR->rd] & 0x000003FF; /*Low bits 9:0 of Rd*/
		if (!core->state.isDelaySlot) {
			core->state.isDelaySlot = true;
			core->new_state_Reg->if_id->New_PC = newPC;
			core->new_state_Reg->if_id->delaySlotPC = newPC + 1;
		}
		else {
			core->new_state_Reg->if_id->New_PC = newPC;
		}
	}
	else /*jump not taken*/
		core->new_state_Reg->if_id->New_PC = core->state.PC + 1;
}

/**
* The Function performs signe extension on the immediate and loads data from the relevant registers to A and B in the ALU. 
 * @param core: pointer to a Core object
*/
void loadRegsForExecute(Core* core) {
	core->current_state_Reg->if_id->IR->imm = sign_extention(core->current_state_Reg->if_id->IR->imm);
	if (core->current_state_Reg->if_id->IR->rs == 1)/*gets the value of immediate*/
		core->new_state_Reg->id_ex->A = core->current_state_Reg->if_id->IR->imm;
	else
		core->new_state_Reg->id_ex->A = core->current_state_Reg->privateRegisters[core->current_state_Reg->if_id->IR->rs];
	if (core->current_state_Reg->if_id->IR->rt == 1)/*gets the value of immediate*/
		core->new_state_Reg->id_ex->B = core->current_state_Reg->if_id->IR->imm;
	else
		core->new_state_Reg->id_ex->B = core->current_state_Reg->privateRegisters[core->current_state_Reg->if_id->IR->rt];
}

/**
 * Function that indicates whether there are data hazards between 2 instructions.
 * The function checks all cases where a register is used before it is written to with WB
 * Rd_inst is checked only only if it's an instruciton that updates Rd
 * @param Inst_to_check: the instruction we want to run - need to check that it doesn't use Rd from instRd
 * @param Rd_inst: instruction that updates Rd
 * @return bool -  whether there are data hazards between 2 instructions.
*/
bool check_if_Data_Hazard(Instruction* Inst_to_check, Instruction* Rd_inst) {
	if (Rd_inst->rd == 0 || Rd_inst->rd == 1 ||
		(Rd_inst->opcode >= 9 && Rd_inst->opcode <= 15) || Rd_inst->opcode == 17) // branch instructions or sw -> Rd doesn't change - no hazard
		return false;
	if (Inst_to_check->opcode != 15 && (Inst_to_check->opcode != 20)) // not halt/jal instruction data- optional hazard in rs/rt
		if (Inst_to_check->rs == Rd_inst->rd || Inst_to_check->rt == Rd_inst->rd)
			return true;
	if ((Inst_to_check->opcode >= 9 && Inst_to_check->opcode <= 15) || Inst_to_check->opcode == 17) /*
																	for branches and sw to validate that rd was written before the instruction*/
		if (Inst_to_check->rd == Rd_inst->rd)
			return true;
	return false;
}

/**
 * Function that finds data hazards in the pipeline and returns stall data. Checks for data hazard
 * between the different stages of pipeline to check if decode / execute is stalled or runs
 * @param stage: the stage in which to update the stallData (only EXECUTE in our pipeline)
 * @param core: pointer to a Core object
 * @return StallData struct data - contaning updated stall status in the stage
*/
StallData findDataHazards(PipelineStages stage, Core* core) {
	StallData data;
	data.active = false;
	if (check_if_Data_Hazard(core->current_state_Reg->if_id->IR, core->current_state_Reg->id_ex->IR)) {
		data.active = true;
		data.cyclesToStall = 3;
	}
	else if (core->state.memoryExecuted && check_if_Data_Hazard(core->current_state_Reg->if_id->IR, core->current_state_Reg->ex_mem->IR)) {
		data.active = true;
		data.cyclesToStall = 2;
	}
	else if (core->state.writeBackExecuted && check_if_Data_Hazard(core->current_state_Reg->if_id->IR, core->current_state_Reg->mem_wb->IR)) {
		data.active = true;
		data.cyclesToStall = 1;
	}
	return data;
}

/**
 * Function that runs the ALU and returns the updated ALUOUT according to the opcode
 * @param opcode: the ALU operation to perform
 * @param A: first input
 * @param B: second input
 * @return ALU output
*/
int runALU(int opcode, int A, int B) {
	switch (opcode) {
	case 0://ADD
		return A + B;
	case 1://SUB
		return A - B;
	case 2://AND
		return A & B;
	case 3://OR
		return A | B;
	case 4://XOR
		return A ^ B;
	case 5://MUL
		return A * B;
	case 6://SLL
		return A << B;
	case 7://SRA
		return sign_extention(A >> B);
	case 8://SRL
		return A >> B;
	default:
		return A + B;// for memory address
	}
	return 0;
}



// Public functions:
/**
 * Function that calls the cache access function for LW and ST instructions. For other instructions the function doesn't do anything
 * @param core: pointer to a Core object
 * @param StallData struct stall_data 
*/
void mem_access(Core* core, StallData* stall_data) {
	// return if opcode is not lw/sw
	int opcode = core->current_state_Reg->ex_mem->IR->opcode;
	if (opcode != 16 && opcode != 17) {
		return;
	}
	if (opcode == 16)
		cache_access(core, core->Cache, stall_data, 1);
	else
		cache_access(core, core->Cache, stall_data, 2);
}

/**
  * Function that executes the fetch stage in the pipeline
  * @param currentReg: the relevant registers - Q regs
  * @param current_core: pointer to a Core object
  * @param stallData stall: the stall information from the previous cycle
*/
void Fetch(CoreRegisters* current_Reg, Core* current_core, StallData* stall) {
	if (!current_core->state.doFetch || current_core->state.PC == -1) { // reached the end of the program for the core(halt in decode -> PC = -1)
		current_core->new_state_Reg->if_id->PC = current_core->state.PC;
		current_core->state.fetchExecuted = false;/*for printing in the trace*/
		return;
	}
	if (stall[EXECUTE].active || stall[MEMORY].active) {
		current_core->state.fetchExecuted = true;
		return;
	}
	current_core->new_state_Reg->if_id->PC = current_core->state.PC;
	current_core->new_state_Reg->if_id->IR = &(current_core->instructions[current_core->state.PC]);
	current_core->state.fetchExecuted = true;
	if (current_core->new_state_Reg->if_id->New_PC != -1) {
		current_core->state.doDecode = true;
		current_core->coreStatistics.instructions++; // number of the executed instructions
	}
}

/**
  * Function that executes the decode stage in the pipeline, performs branch resolution and loads registers for ALU
  * @param currentReg: the relevant registers - Q regs
  * @param current_core: pointer to a Core object
  * @param stallData stall: the stall information from the previous cycle
*/
void Decode(CoreRegisters* currentRegisters, Core* current_core, StallData* stallData) {
	if (!current_core->state.doDecode) {
		current_core->state.decodeExecuted = false;
		return;
	}
	if (stallData[EXECUTE].active || stallData[MEMORY].active) {
		current_core->state.decodeExecuted = true;
		return;
	}
	current_core->new_state_Reg->id_ex->PC = currentRegisters->if_id->PC;
	current_core->new_state_Reg->id_ex->IR = currentRegisters->if_id->IR;
	if (currentRegisters->if_id->PC == -1) { // empty pipeline
		current_core->state.decodeExecuted = false;
		return;
	}
	current_core->state.doExecute = true;
	current_core->state.decodeExecuted = true;
	if (currentRegisters->if_id->IR->opcode == 20) { // reached halt
		current_core->new_state_Reg->if_id->New_PC = -1;
		current_core->state.doDecode = false;
		return;
	}

	loadRegsForExecute(current_core);
	branchResolution(current_core);
}

/**
  * Function that executes the execute stage in the pipeline, checks for data hazards and configures stalls accordingly
  * @param currentReg: the relevant registers - Q regs
  * @param current_core: pointer to a Core object
  * @param stallData stall: the stall information from the previous cycle
*/
void EX(CoreRegisters* current_Reg, Core* current_core, StallData* stallData) {
	if (!current_core->state.doExecute) {
		current_core->state.executeExecuted = false;
		return;
	}
	if (stallData[MEMORY].active || stallData[WRITE_BACK].active) { // stalls in memory / write back
		if (stallData[EXECUTE].active) { // is stall in execute as well make sure trace is ---
			current_core->state.executeExecuted = false;
		}
		return;
	}
	if (stallData[EXECUTE].active) { // if stalling in execute
		current_core->coreStatistics.decode_stall++; // number of cycles a pipeline stall was inserted in decode stage
		current_core->new_state_Reg->ex_mem->isStall = true; // stall next memory
		current_core->state.executeExecuted = false;
		if ((--stallData[EXECUTE].cyclesToStall) == 0)
			stallData[EXECUTE].active = false;
		return;
	}
	stallData[EXECUTE] = findDataHazards(EXECUTE, current_core);

	current_core->new_state_Reg->ex_mem->PC = current_Reg->id_ex->PC;
	current_core->new_state_Reg->ex_mem->IR = current_Reg->id_ex->IR;
	current_core->new_state_Reg->ex_mem->isStall = false;

	if (current_Reg->id_ex->PC == -1) { // empty pipeline after halt
		current_core->state.executeExecuted = false;
		return;
	}
	if (current_Reg->id_ex->IR->opcode == 20) { // reached halt
		current_core->state.doMemory = true;
		current_core->state.executeExecuted = true;
		current_core->state.doExecute = false;
		return;
	}
	if (!current_core->state.doDecode) {
		current_core->state.doExecute = false;
	}
	current_core->state.doMemory = true;
	current_core->state.executeExecuted = true;
	if ((current_Reg->id_ex->IR->opcode >= 0 && current_Reg->id_ex->IR->opcode <= 8) || (current_Reg->id_ex->IR->opcode >= 16 && current_Reg->id_ex->IR->opcode <= 17)) { /*arithmetic instructions*/
		current_core->new_state_Reg->ex_mem->ALUOutput = runALU(current_Reg->id_ex->IR->opcode,
			current_Reg->id_ex->A, current_Reg->id_ex->B);
	}
}

/**
  * Function that executes the memory stage in the pipeline, uses function mem_access to access cache and memory operations
  * @param currentReg: the relevant registers - Q regs
  * @param current_core: pointer to a Core object
  * @param stallData stall: the stall information from the previous cycle
  * @param int cycleNumber: the number of the cycles the core was running
*/
void MEM(CoreRegisters* current_Reg, Core* current_core, StallData* stallData, int cycleNumber) {
	bool isStall = false;
	if (!current_core->state.doMemory) {
		current_core->state.memoryExecuted = false;
		return;
	}
	if (current_Reg->ex_mem->isStall) { // stalling from execute - move stall down the pipeline
		current_core->state.memoryExecuted = false;
		current_core->new_state_Reg->mem_wb->isStall = true;
		return;
	}

	if (stallData[MEMORY].active) {  // stall in memory 
		current_core->new_state_Reg->mem_wb->isStall = true;
		current_core->state.memoryExecuted = true;
		mem_access(current_core, stallData);
		return;
	}

	current_core->new_state_Reg->mem_wb->isStall = false;
	current_core->new_state_Reg->mem_wb->IR = current_Reg->ex_mem->IR;
	current_core->new_state_Reg->mem_wb->PC = current_Reg->ex_mem->PC;
	if (current_Reg->ex_mem->PC == -1) { // empty pipeline
		current_core->state.memoryExecuted = false;
		return;
	}

	if (current_Reg->ex_mem->IR->opcode == 20) { // halt reached
		current_core->state.doWriteBack = true;
		current_core->state.memoryExecuted = true;
		current_core->state.doMemory = false;
		return;
	}
	if (!current_core->state.doExecute) {
		current_core->state.doMemory = false;
	}

	current_core->state.doWriteBack = true;
	current_core->state.memoryExecuted = true;
	current_core->new_state_Reg->mem_wb->ALUOutput = current_Reg->ex_mem->ALUOutput;

	if (current_core->current_state_Reg->ex_mem->IR->opcode >= 16 && current_core->current_state_Reg->ex_mem->IR->opcode <= 17) { // lw, sw
		mem_access(current_core, stallData);
	}
}

/**
  * Function that executes the write-back stage in the pipeline
  * @param currentReg: the relevant registers - Q regs
  * @param current_core: pointer to a Core object
  * @param stallData stall: the stall information from the previous cycle
  * @param int cycleNumber: the number of the cycles the core was running
*/
void WriteBack(CoreRegisters* currentRegisters, Core* core, StallData* stallData, int cycleNumber) {
	if (currentRegisters->mem_wb->isStall) { // stall
		core->state.writeBackExecuted = false;
		return;
	}
	if (!core->state.doWriteBack) //haven't reached the first WB yet
		return;
	if (currentRegisters->mem_wb->IR->opcode == 20) { // the core reached halt - stop program WB ended
		core->state.haltProgram = true;
		core->coreStatistics.cycles = cycleNumber + 1;// number of clock cycles the core was running untill halt
		core->state.writeBackExecuted = true;
		core->state.doWriteBack = false;
		return;
	}
	core->state.writeBackExecuted = true;
	if (core->new_state_Reg->mem_wb->IR->opcode == 16) {
		core->new_state_Reg->privateRegisters[currentRegisters->mem_wb->IR->rd] = currentRegisters->mem_wb->LMD;
		return;
	}
	else if (core->new_state_Reg->mem_wb->IR->opcode == 17 || (core->new_state_Reg->mem_wb->IR->opcode >= 9 && core->new_state_Reg->mem_wb->IR->opcode <= 15)) {
		return;
	}
	if (currentRegisters->mem_wb->IR->rd != 0 && currentRegisters->mem_wb->IR->rd != 1)/*shouldn't change the ZERO and IMM registers*/
		core->new_state_Reg->privateRegisters[currentRegisters->mem_wb->IR->rd] = currentRegisters->mem_wb->ALUOutput;
}

/**
  * Function that moves all the data from D registers to Q registers between cycles
  * @param current_core: pointer to a Core object containing the Q and D registers
*/
void update_core_registers(Core* core) {
	memcpy(core->current_state_Reg->if_id, core->new_state_Reg->if_id, sizeof(IF_ID));
	memcpy(core->current_state_Reg->id_ex, core->new_state_Reg->id_ex, sizeof(ID_EX));
	memcpy(core->current_state_Reg->ex_mem, core->new_state_Reg->ex_mem, sizeof(EX_MEM));
	memcpy(core->current_state_Reg->mem_wb, core->new_state_Reg->mem_wb, sizeof(MEM_WB));

	for (int i = 0; i < NUM_OF_REGISTERS; i++) {
		core->current_state_Reg->privateRegisters[i] = core->new_state_Reg->privateRegisters[i];
	}
}