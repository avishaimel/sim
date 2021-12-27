#pragma once 

#include <stdbool.h>
#include "pipeline.h"


//Private functions:

int sign_extention(int immediate) {
	immediate <<= SIGN_EXTEND_NUM;
	immediate >>= SIGN_EXTEND_NUM;
	return immediate;
}

bool isBranchOpcode(int opcode)
{
	return opcode >= 9 && opcode <= 15;
}


void branchResolution(Core *core) {
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

bool check_if_Data_Hazard(Instruction *Inst_to_check, Instruction *Rd_inst) {
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
void mem_access(Core* core, int cycleNumber, StallData* stall_data) {
	// return if opcode is not lw/sw
	int opcode = core->current_state_Reg->ex_mem->IR->opcode;
	if (opcode != 16 && opcode != 17) {
		return;
	}
	bool tag_conflict = false;
	if (opcode == 16) {
		int mesi_state = get_mesi_state(core->Cache, core->current_state_Reg->ex_mem->ALUOutput, &tag_conflict);
		if (mesi_state == INVALID || tag_conflict) {
			core->Cache->read_miss++;
			
		}
	}
}

void handle_load_store(Core* core, int cycleNumber, StallData* stallData)
{
	int result_LL;
	bool isStall = false;
	core->new_state_Reg->mem_wb->LMD = request(core->Cache,
		core->current_state_Reg->ex_mem->IR->opcode,
		core->current_state_Reg->ex_mem->ALUOutput, core->current_state_Reg->privateRegisters[core->current_state_Reg->ex_mem->IR->rd],
		core->coreID, cycleNumber,
		&isStall, &result_LL);
	if (core->current_state_Reg->ex_mem->IR->opcode == 19) //sc
	{
		core->new_state_Reg->mem_wb->ALUOutput = result_LL;
	}
	if (isStall) {
		stallData[MEMORY].active = true;
		core->new_state_Reg->mem_wb->isStall = true;
	}
	else {
		stallData[MEMORY].active = false;
		core->new_state_Reg->mem_wb->isStall = false;
	}
}



void Fetch(CoreRegisters* current_Reg, Core* current_core, StallData* stall) {
	if (!current_core->state.doFetch || current_core->state.PC == -1) { // reached end of program (halt in decode -> PC = -1)
		current_core->new_state_Reg->if_id->PC = current_core->state.PC;
		current_core->state.fetchExecuted = false;/*for the trace printing*/
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
		current_core->coreStatistics.instructions++; // number of instructions executed
	}
}


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

void MEM(CoreRegisters* current_Reg, Core* current_core, StallData* stallData, int cycleNumber) {
	bool isStall = false;
	if (!current_core->state.doMemory) {
		current_core->state.memoryExecuted = false;
		return;
	}
	if (current_Reg->ex_mem->isStall) { // stalling from execute - move bubble down the pipeline
		current_core->state.memoryExecuted = false;
		current_core->new_state_Reg->mem_wb->isStall = true;
		return;
	}

	if (stallData[MEMORY].active) {  // stall in memory 
		current_core->new_state_Reg->mem_wb->isStall = true;
		current_core->state.memoryExecuted = true;
		handle_load_store(current_core, cycleNumber, stallData);
		return;
	}

	current_core->new_state_Reg->mem_wb->isStall = false;
	current_core->new_state_Reg->mem_wb->IR = current_Reg->ex_mem->IR;
	current_core->new_state_Reg->mem_wb->PC = current_Reg->ex_mem->PC;
	if (current_Reg->ex_mem->PC == -1) { // empty pipeline
		current_core->state.memoryExecuted = false;
		return;
	}

	if (current_Reg->ex_mem->IR->opcode == 20) { // reached halt
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
		handle_load_store(current_core, cycleNumber, stallData);
	}
}


void WriteBack(CoreRegisters* currentRegisters, Core* core, StallData* stallData, int cycleNumber) {
	if (currentRegisters->mem_wb->isStall) { // stall
		core->state.writeBackExecuted = false;
		return;
	}
	if (!core->state.doWriteBack) // before first WB
		return;
	if (currentRegisters->mem_wb->IR->opcode == 20) { // reached halt- stop program WB ended
		core->state.haltProgram = true;
		core->coreStatistics.cycles = cycleNumber + 1;// actual number of clock cycles the core was running till halt
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
	if (currentRegisters->mem_wb->IR->rd != 0 && currentRegisters->mem_wb->IR->rd != 1)/*for not changing the ZERO and IMM registers*/
		core->new_state_Reg->privateRegisters[currentRegisters->mem_wb->IR->rd] = currentRegisters->mem_wb->ALUOutput;
}

void update_core_registers(Core* core) {
	memcpy(core->current_state_Reg->if_id, core->new_state_Reg->if_id, sizeof(IF_ID));
	memcpy(core->current_state_Reg->id_ex, core->new_state_Reg->id_ex, sizeof(ID_EX));
	memcpy(core->current_state_Reg->ex_mem, core->new_state_Reg->ex_mem, sizeof(EX_MEM));
	memcpy(core->current_state_Reg->mem_wb, core->new_state_Reg->mem_wb, sizeof(MEM_WB));

	for (int i = 0; i < NUM_OF_REGISTERS; i++) {
		core->current_state_Reg->privateRegisters[i] = core->new_state_Reg->privateRegisters[i];
	}
}