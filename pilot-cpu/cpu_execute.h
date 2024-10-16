#ifndef __CPU_EXECUTE_H__
#define __CPU_EXECUTE_H__

#include "pilot.h"
#include "types.h"

typedef struct {
	Pilot_system *sys;
	
	inst_decoded_flags decoded_inst;
	mucode_entry_spec mucode_control;
	execute_control_word mucode_decoded_buffer;
	execute_control_word *control;
	mucode_entry_spec repeat_type;
	mucode_entry_spec repeat_reg_type;
	
	uint32_t alu_input_latches[2];
	uint32_t alu_output_latch;
	bool alu_shifter_carry_bit;
	
	bool used_z;
	
	// Memory address and data registers for requesting memory accesses
	uint32_t mem_addr;
	uint32_t mem_data;
	
	// When reading memory, this flag will be high until the memory access has been completed.
	// During this time, any reads from mem_data will block until this flag goes low.
	bool mem_access_waiting;
	bool mem_access_was_read;
	
	enum
	{
		EXEC_HALF1_READY,
		EXEC_HALF1_MEM_WAIT,
		EXEC_HALF1_OPERAND_LATCH,
		EXEC_HALF1_MEM_PREPARE,
		EXEC_HALF1_MEM_ASSERT,
		
		EXEC_HALF2_READY,
		EXEC_HALF2_RESULT_LATCH,
		EXEC_HALF2_MEM_PREPARE,
		EXEC_HALF2_MEM_ASSERT,
		EXEC_HALF2_ADVANCE_SEQUENCER,
		
		EXEC_EXCEPTION
	} execution_phase;
	
	enum
	{
		EXEC_SEQ_WAIT_NEXT_INS,
		EXEC_SEQ_WAIT_CACHED_INS,
		EXEC_SEQ_EVAL_CONTROL,
		EXEC_SEQ_RUN_BEFORE,
		EXEC_SEQ_CORE_OP,
		EXEC_SEQ_CORE_OP_EXECUTED,
		EXEC_SEQ_RUN_AFTER,
		EXEC_SEQ_REPEAT_OP,
		EXEC_SEQ_REPEAT_REG_OP,
		EXEC_SEQ_FINAL_STEPS,
		EXEC_SEQ_SIGNAL_BRANCH,
		EXEC_SEQ_BRANCH_OP,
		EXEC_SEQ_PUSH_PGC,
		EXEC_SEQ_PUSH_WF
	} sequencer_phase;
} pilot_execute_state;

void pilot_execute_half1 (pilot_execute_state *state);
void pilot_execute_half2 (pilot_execute_state *state);

#endif
