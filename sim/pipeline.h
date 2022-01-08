#pragma once
#ifndef PIPELINE_H
#define PIPELINE_H

#include <stdio.h>
#include "core.h"
#define SIGN_EXTEND_NUM 20

/**
  * Enumerate pipeline stages
  */
typedef enum PipelineStages {
	FETCH,
	DECODE,
	EXECUTE,
	MEMORY,
	WRITE_BACK
} PipelineStages;

//Functions:
//documentation in pipeline.c
void Fetch(CoreRegisters* current_Reg, Core* current_core, StallData* stall);	
void Decode(CoreRegisters* currentRegisters, Core* current_core, StallData* stallData);
void EX(CoreRegisters* current_Reg, Core* current_core, StallData* stallData);
void MEM(CoreRegisters* current_Reg, Core* current_core, StallData* stallData, int cycleNumber);
void WriteBack(CoreRegisters* currentRegisters, Core* core, StallData* stallData, int cycleNumber);
void update_core_registers(Core* core);

#endif