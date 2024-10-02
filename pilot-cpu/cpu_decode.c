#include "cpu_regs.h"
#include "cpu_decode.h"
#include "memory.h"
#include <stdint.h>

/*
 * Pipeline stages:
 * 
 * 1. Fetch
 * The fetch stage consists of a 5-word prefetch queue and a 1-word latch connected to the decode stage.
 * 
 * a. First half of cycle
 * - If prefetch queue is not empty and a branch is not signalled from the decode stage:
 *   - If "word ready" is lowered, or "wait" line is not raised:
 *     - Pull word from prefetch queue
 *       - Latch word for decode stage
 *     - Raise "word ready" line, if lowered
 * - Otherwise:
 *   - Lower "word ready" line
 *
 * b. Second half of cycle
 * - If a branch is signalled from decode stage:
 *   - Lower "word ready" line
 *   - Flush prefetch queue
 *   - Set address to predicted PC value from decode stage
 *
 * - If the execute stage isn't trying to read from memory:
 *   - Read next word from next address
 *   - Increment next address (by 2)
 *
 * -----
 *
 * Interconnections fetch->decode:
 * - "Word ready" line - signal to the decode stage that the word latch contains valid data and is ready to be read.
 * - Word latch (16 bits)
 *
 * Interconnections decode->fetch:
 * - "Wait" line - signal from the decode stage that the currently latched word should be held
 * - "Stall" line - signal from the decode stage that a branch misprediction has happened
 * - "Branch" line - signal from the decode stage that a branch has happened and the branch address has been latched
 * - Branch address latch (24 bits)
 *
 * ----
 *
 * 2. Decode
 * The decode stage consists of a combinational logic array that interprets an instruction word and buffers a series of
 * signals for the execute stage, which are latched after the execute stage is done with the last instruction.
 *
 * a. First half of cycle
 * - Read instruction word from fetch stage and decode it
 *   - If immediate operand required, read another word from fetch stage
 *
 *   - If branch instruction is detected, try to predict a branch
 *     - Branches can only be predicted if the branch destination is directly encoded as an immediate value in the
 *       instruction word and/or its following immediate operand
 *     - If branch can be predicted:
 *       - Assume it will be taken
 *       - Latch predicted PC value to fetch stage
 *       - Signal branch to fetch stage
 *     - If branch cannot be predicted:
 *       - Signal stall to fetch stage
 *       - Wait for updated PC value from this instructions's execution stage
 *
 * b. Second half of cycle
 * - If "execute ready" line has been raised:
 *   - If a decoded instruction is available:
 *     - Latch previously decoded signals for execution stage
 *     - Strobe "instruction ready" line
 *   - Else:
 * - Else:
 *   - Keep "wait" line raised
 * 
 * -----
 *
 * Interconnections
 * 
 * -----
 * 
 * 3. Execute
 * The execute stage consists of sequential logic that uses the latched signals from the instruction decoder to perform
 * the instruction.
 * 
 */

// Placeholder
void decode_not_implemented_ ();

// Decodes an RM specifier.
void decode_rm_specifier (pilot_decode_state *state, rm_spec rm, bool is_dest, bool src_is_left, data_size_spec size);

static inline void
decode_inst_branch_ (pilot_decode_state *state, uint16_t opcode)
{
	inst_decoded_flags *work_regs = &state->work_regs;
	execute_control_word *core_op = &work_regs->core_op;
	mucode_entry_spec *repeat_op = &work_regs->repeat_op;
	
	if ((opcode & 0xff00) == 0xff00)
	{
		// RST
		work_regs->branch = TRUE;
		work_regs->branch_cond = COND_ALWAYS_CALL;
		work_regs->branch_dest_type = BR_RESTART;
		return;
	}
	if ((opcode & 0xffe0) == 0xfe00)
	{
		// REPI
		core_op->operation = ALU_OR;
		core_op->shifter_mode = SHIFTER_NONE;
		
		core_op->srcs[0].location = DATA_ZERO;
		core_op->src2_add1 = TRUE;
		core_op->src2_add_carry = FALSE;
		core_op->src2_negate = FALSE;
		core_op->flag_write_mask = 0;
		core_op->flag_v_mode = FLAG_V_NORMAL;
		core_op->flag_z_mode = FLAG_Z_SAVE;
		
		core_op->srcs[1].location = DATA_LATCH_IMM_0;
		core_op->srcs[1].size = SIZE_8_BIT;
		core_op->srcs[1].sign_extend = FALSE;
		core_op->dest = DATA_LATCH_REPI;
		
		repeat_op->entry_idx = MU_REPI;
		
		return;
	}
	if ((opcode & 0xf800) == 0xf000)
	{
		if ((opcode & 0x00ff) == 0x0000)
		{
			// REPR
			core_op->operation = ALU_OR;
			core_op->shifter_mode = SHIFTER_NONE;
			
			core_op->srcs[0].location = DATA_ZERO;
			core_op->src2_add1 = FALSE;
			core_op->src2_add_carry = FALSE;
			core_op->src2_negate = FALSE;
			core_op->flag_write_mask = 0;
			core_op->flag_v_mode = FLAG_V_NORMAL;
			core_op->flag_z_mode = FLAG_Z_NORMAL;
			
			core_op->srcs[1].location = DATA_LATCH_IMM_0;
			core_op->srcs[1].size = SIZE_8_BIT;
			core_op->srcs[1].sign_extend = FALSE;
			core_op->dest = DATA_LATCH_REPR;
			
			repeat_op->entry_idx = MU_REPR;
			
			return;
		}
		else if ((opcode & 0x0080) == 0x0080)
		{
			// DJNZ
			core_op->operation = ALU_ADD;
			core_op->srcs[0].location = DATA_REG_IMM_0_8;
			core_op->srcs[1].location = DATA_ZERO;
			core_op->src2_add1 = FALSE;
			core_op->src2_add_carry = FALSE;
			core_op->src2_negate = TRUE;
			core_op->flag_write_mask = 0;
			core_op->flag_v_mode = FLAG_V_NORMAL;
			core_op->flag_z_mode = FLAG_Z_SAVE;
			core_op->dest = DATA_REG_IMM_0_8;
			
			core_op->shifter_mode = SHIFTER_NONE;
			
			work_regs->branch = TRUE;
			work_regs->branch_cond = COND_DJNZ;
			work_regs->branch_dest_type = BR_BACKWARD;
			
			return;
		}
		else
		{
			decode_invalid_opcode_(state->sys);
			return;
		}
	}
	
	if ((opcode & 0xf000) == 0xe000)
	{
		// JR cond / JR.S / CR.S
		core_op->operation = ALU_OFF;
		work_regs->branch = TRUE;
		work_regs->branch_cond = (opcode >> 8) & 0xf;
		work_regs->branch_dest_type = BR_RELATIVE_SHORT;
	}
	
	switch (opcode & 0x0700)
	{
		case 0x0000:
			// JP / JR.L
			decode_queue_read_word(state);
			core_op->operation = ALU_OFF;
			work_regs->branch = TRUE;
			work_regs->branch_cond = COND_ALWAYS;
			work_regs->branch_dest_type = BR_HML;
			return;
		case 0x0100:
			// CALL / CR.L
			decode_queue_read_word(state);
			core_op->operation = ALU_OFF;
			work_regs->branch = TRUE;
			work_regs->branch_cond = COND_ALWAYS;
			work_regs->branch_dest_type = BR_HML;
			return;
		case 0x0200:
		{
			rm_spec rm_src = opcode & 0x3f;
			
			switch (opcode & 0x01c0)
			{
				case 0x0000:
					// JP rm24
					core_op->shifter_mode = SHIFTER_NONE;
					core_op->src2_add1 = FALSE;
					core_op->flag_v_mode = FLAG_V_NORMAL;
					core_op->flag_z_mode = FLAG_Z_NORMAL;
					
					decode_rm_specifier(state, rm_src, FALSE, FALSE, SIZE_24_BIT);
					
					work_regs->branch = TRUE;
					work_regs->branch_cond = COND_ALWAYS;
					work_regs->branch_dest_type = BR_MAR;
					return;
				case 0x0040:
					// JEA
					core_op->shifter_mode = SHIFTER_NONE;
					core_op->src2_add1 = FALSE;
					core_op->flag_v_mode = FLAG_V_NORMAL;
					core_op->flag_z_mode = FLAG_Z_NORMAL;
					
					decode_rm_specifier(state, rm_src, FALSE, FALSE, SIZE_24_BIT);
					core_op->mem_access_suppress = TRUE;
					
					work_regs->branch = TRUE;
					work_regs->branch_cond = COND_ALWAYS;
					work_regs->branch_dest_type = BR_MAR;
					return;
				case 0x0100:
					// CALL rm24
					core_op->shifter_mode = SHIFTER_NONE;
					core_op->src2_add1 = FALSE;
					core_op->flag_v_mode = FLAG_V_NORMAL;
					core_op->flag_z_mode = FLAG_Z_NORMAL;
					
					decode_rm_specifier(state, rm_src, FALSE, FALSE, SIZE_24_BIT);
					
					work_regs->branch = TRUE;
					work_regs->branch_cond = COND_ALWAYS_CALL;
					work_regs->branch_dest_type = BR_MAR;
					return;
				case 0x0140:
					// CEA
					core_op->shifter_mode = SHIFTER_NONE;
					core_op->src2_add1 = FALSE;
					core_op->flag_v_mode = FLAG_V_NORMAL;
					core_op->flag_z_mode = FLAG_Z_NORMAL;
					
					decode_rm_specifier(state, rm_src, FALSE, FALSE, SIZE_24_BIT);
					core_op->mem_access_suppress = TRUE;
					
					work_regs->branch = TRUE;
					work_regs->branch_cond = COND_ALWAYS_CALL;
					work_regs->branch_dest_type = BR_MAR;
					return;
				default:
					break;
			}
		}
	}
	
	decode_invalid_opcode_(state->sys);
	return;
}

static void
decode_inst_bit_ (pilot_decode_state *state, uint16_t opcode)
{
	execute_control_word *core_op = &state->work_regs.core_op;
	rm_spec rm_src = opcode & 0x3f;
	
	core_op->src2_add1 = FALSE;
	core_op->src2_add_carry = FALSE;
	core_op->src2_negate = FALSE;
	core_op->flag_write_mask = 0;
	core_op->flag_v_mode = FLAG_V_NORMAL;
	core_op->flag_z_mode = FLAG_Z_NORMAL;
	
	core_op->shifter_mode = SHIFTER_NONE;
	
	if ((opcode & 0x0800) == 0x0000)
	{
		decode_rm_specifier(state, rm_src, (opcode & 0x00c0) == 0x0000, TRUE, SIZE_8_BIT);
		
		core_op->srcs[1].location = DATA_DMX_IMM_BITS;
		core_op->srcs[1].size = SIZE_8_BIT;
		core_op->srcs[1].sign_extend = FALSE;
		
		core_op->flag_write_mask = F_ZERO;
		
		core_op->flag_z_mode = FLAG_Z_BIT_TEST;
		
		switch (opcode & 0x00c0)
		{
			case 0x0000:
				// BIT imm, rm8
				core_op->operation = ALU_OR;
				core_op->dest = DATA_ZERO;
				break;
			case 0x0040:
				// CHG imm, rm8
				core_op->operation = ALU_XOR;
				break;
			case 0x0080:
				// RES imm, rm8
				core_op->src2_negate = TRUE;
				core_op->operation = ALU_AND;
				break;
			case 0x00c0:
				// SET imm, rm8
				core_op->operation = ALU_OR;
				break;
		}
		
		return;
	}
	if ((opcode & 0x0800) == 0x0800)
	{
		decode_rm_specifier(state, rm_src, (opcode & 0x00c0) == 0x0000, TRUE, SIZE_8_BIT);
		
		core_op->srcs[1].location = DATA_DMX_P0_BITS;
		core_op->srcs[1].size = SIZE_8_BIT;
		core_op->srcs[1].sign_extend = FALSE;
		
		core_op->flag_write_mask = F_ZERO;
		
		core_op->flag_z_mode = FLAG_Z_BIT_TEST;
		
		switch (opcode & 0x00c0)
		{
			case 0x0000:
				// BIT M0, rm8
				core_op->operation = ALU_OR;
				core_op->dest = DATA_ZERO;
				break;
			case 0x0040:
				// CHG M0, rm8
				core_op->operation = ALU_XOR;
				break;
			case 0x0080:
				// RES M0, rm8
				core_op->src2_negate = TRUE;
				core_op->operation = ALU_AND;
				break;
			case 0x00c0:
				// SET M0, rm8
				core_op->operation = ALU_OR;
				break;
		}
		
		return;
	}
	if ((opcode & 0x0ff8) == 0x0b00)
	{
		// LD IRL, imm
		core_op->srcs[1].location = DATA_LATCH_IMM_0;
		core_op->srcs[1].size = SIZE_8_BIT;
		core_op->srcs[1].sign_extend = FALSE;
		
		core_op->dest = DATA_REG_IRL;
		return;
	}
	if ((opcode & 0x0c00) == 0x0c00)
	{
		core_op->srcs[0].location = DATA_LATCH_IMM_0;
		core_op->srcs[0].size = SIZE_8_BIT;
		core_op->srcs[0].sign_extend = FALSE;
		
		core_op->srcs[1].location = DATA_REG_F;
		core_op->srcs[1].size = SIZE_8_BIT;
		core_op->srcs[1].sign_extend = FALSE;
		
		core_op->dest = DATA_REG_F;
		
		switch (opcode & 0x0300)
		{
			case 0x0000:
				// AND.B F, imm
				core_op->operation = ALU_AND;
				break;
			case 0x0100:
				// XOR.B F, imm
				core_op->operation = ALU_XOR;
				break;
			case 0x0200:
				// OR.B F, imm
				core_op->operation = ALU_OR;
				break;
			case 0x0300:
				// LD.B F, imm
				core_op->operation = ALU_OFF;
				break;
		}
		
		return;
	}
	
	decode_invalid_opcode_(state->sys);
	return;
}

static void
decode_inst_ld_other_ (pilot_decode_state *state, uint16_t opcode)
{
	execute_control_word *core_op = &state->work_regs.core_op;
	core_op->src2_add1 = FALSE;
	core_op->src2_add_carry = FALSE;
	core_op->src2_negate = FALSE;
	core_op->flag_write_mask = 0;
	core_op->flag_v_mode = FLAG_V_NORMAL;
	core_op->flag_z_mode = FLAG_Z_NORMAL;

	// left operand is never fetched and is always zero
	core_op->srcs[0].location = DATA_ZERO;
	core_op->srcs[0].size = SIZE_24_BIT;
	core_op->operation = ALU_OR;

	core_op->dest = DATA_REG_IMM_0_8;
	core_op->shifter_mode = SHIFTER_NONE;
	
	if ((opcode & 0x0800) == 0x0000)
	{
		// LD.P r24, hml
		core_op->srcs[1].location = DATA_LATCH_IMM_HML;
		core_op->srcs[1].size = SIZE_24_BIT;
		core_op->srcs[1].sign_extend = FALSE;
		return;
	}
	else
	{
		// LDQ
		core_op->srcs[1].location = DATA_LATCH_IMM_0;
		core_op->srcs[1].size = SIZE_8_BIT;
		core_op->srcs[1].sign_extend = TRUE;
		return;
	}
	
	decode_unreachable_();
	return;
}

static inline void
decode_inst_arithlogic_ (pilot_decode_state *state, uint16_t opcode)
{
	uint8_t operation = ((opcode & 0x00c0) >> 6) | ((opcode & 0x1800) >> 9);
	data_size_spec size = ((opcode & 0xc000) >> 14);
	execute_control_word *core_op = &state->work_regs.core_op;
	
	bool uses_imm = FALSE;
	
	core_op->shifter_mode = SHIFTER_NONE;
	core_op->src2_add1 = FALSE;
	core_op->flag_v_mode = FLAG_V_NORMAL;
	core_op->flag_z_mode = FLAG_Z_NORMAL;
	
	// Decode core_op operation
	switch (operation)
	{
		case 0: case 1: case 2: case 3: case 8: case 9: case 10: case 11:
			// ADD / ADX / SUB / SBX
			core_op->operation = ALU_ADD;
			core_op->src2_add_carry = (operation & 1) != 0;
			core_op->src2_negate = (operation & 2) != 0;
			core_op->flag_write_mask = F_SIGN | F_ZERO | F_OVERFLOW | F_CARRY | F_EXTEND;
			break;
		case 4: case 12:
			// AND
			core_op->operation = ALU_AND;
			core_op->src2_add_carry = FALSE;
			core_op->src2_negate = FALSE;
			core_op->flag_write_mask = F_SIGN | F_ZERO | F_OVERFLOW | F_CARRY;
			break;
		case 5: case 13:
			// XOR
			core_op->operation = ALU_XOR;
			core_op->src2_add_carry = FALSE;
			core_op->src2_negate = FALSE;
			core_op->flag_write_mask = F_SIGN | F_ZERO | F_OVERFLOW | F_CARRY;
			break;
		case 6: case 14:
			// OR
			core_op->operation = ALU_OR;
			core_op->src2_add_carry = FALSE;
			core_op->src2_negate = FALSE;
			core_op->flag_write_mask = F_SIGN | F_ZERO | F_OVERFLOW | F_CARRY;
			break;
		case 7:
			// CP
			core_op->operation = ALU_ADD;
			core_op->src2_add_carry = FALSE;
			core_op->src2_negate = TRUE;
			core_op->flag_write_mask = F_SIGN | F_ZERO | F_OVERFLOW | F_CARRY;
			break;
		case 15:
		{
			uses_imm = TRUE;
			operation = (opcode & 0x0700) >> 8;
			switch(operation)
			{
				case 0: case 1: case 2: case 3:
					// ADD / ADX / SUB / SBX
					core_op->operation = ALU_ADD;
					core_op->src2_add_carry = operation & 1;
					core_op->src2_negate = operation & 2;
					core_op->flag_write_mask = F_SIGN | F_ZERO | F_OVERFLOW | F_CARRY | F_EXTEND;
					break;
				case 4:
					// AND
					core_op->operation = ALU_AND;
					core_op->src2_add_carry = FALSE;
					core_op->src2_negate = FALSE;
					core_op->flag_write_mask = F_SIGN | F_ZERO | F_OVERFLOW | F_CARRY;
					break;
				case 5:
					// XOR
					core_op->operation = ALU_XOR;
					core_op->src2_add_carry = FALSE;
					core_op->src2_negate = FALSE;
					core_op->flag_write_mask = F_SIGN | F_ZERO | F_OVERFLOW | F_CARRY;
					break;
				case 6:
					// OR
					core_op->operation = ALU_OR;
					core_op->src2_add_carry = FALSE;
					core_op->src2_negate = FALSE;
					core_op->flag_write_mask = F_SIGN | F_ZERO | F_OVERFLOW | F_CARRY;
					break;
				case 7:
					// CP
					core_op->operation = ALU_ADD;
					core_op->src2_add_carry = FALSE;
					core_op->src2_negate = TRUE;
					core_op->flag_write_mask = F_SIGN | F_ZERO | F_OVERFLOW | F_CARRY;
					break;
			default:
				decode_unreachable_();
			}
		}
		default:
			decode_unreachable_();
	}
	
	// Decode operands
	if (!uses_imm)
	{
		// from RM src
		rm_spec rm = opcode & 0x003f;
		decode_rm_specifier(state, rm, FALSE, FALSE, size);
		state->work_regs.core_op.srcs[1].sign_extend = FALSE;
		
		state->work_regs.core_op.srcs[0].location = DATA_REG_IMM_0_8;
		state->work_regs.core_op.srcs[0].size = size;
		state->work_regs.core_op.srcs[0].sign_extend = FALSE;
		return;
	}
	else
	{
		// from immediate src
		rm_spec rm = opcode & 0x003f;
		decode_rm_specifier(state, rm, TRUE, TRUE, size);
		state->work_regs.core_op.srcs[0].sign_extend = FALSE;
		
		state->work_regs.core_op.srcs[1].location = DATA_LATCH_IMM_1;
		state->work_regs.core_op.srcs[1].size = size;
		state->work_regs.core_op.srcs[1].sign_extend = FALSE;
		
		if (operation == 7)
		{
			state->work_regs.core_op.dest = DATA_ZERO;
		}
		return;
	}
}

static void
decode_inst_ld_group_ (pilot_decode_state *state, uint16_t opcode)
{
	execute_control_word *core_op = &state->work_regs.core_op;
	data_size_spec size = ((opcode & 0xc000) >> 14);
	core_op->src2_add1 = FALSE;
	core_op->src2_add_carry = FALSE;
	core_op->src2_negate = FALSE;
	core_op->flag_write_mask = 0;
	core_op->flag_v_mode = FLAG_V_NORMAL;
	core_op->flag_z_mode = FLAG_Z_NORMAL;

	// left operand is never fetched and is always zero
	core_op->srcs[0].location = DATA_ZERO;
	core_op->srcs[0].size = size;
	
	core_op->operation = ALU_OR;
	core_op->shifter_mode = SHIFTER_NONE;
	
	if ((opcode & 0x00c0) == 0x00c0)
	{
		// one RM specifier
		rm_spec rm_src = opcode & 0x3f;

		core_op->srcs[0].size = SIZE_24_BIT;
		core_op->dest = DATA_REG_IMM_0_8;

		if ((opcode & 0x8800) == 0x0800)
		{
			// LDSX
			core_op->srcs[1].sign_extend = TRUE;
		}
		if ((opcode & 0xf000) == 0x9000)
		{
			// LEA (uses the RM decoder to handle the auto-index)
			rm_spec dummy_rm = (opcode & 0x0f80) >> 7;
			decode_rm_specifier(state, dummy_rm, TRUE, FALSE, size);
			core_op->mem_access_suppress = TRUE;
		}
		
		// if LEA, this will technically be treated as the second RM operand, but that operand is never used
		decode_rm_specifier(state, rm_src, FALSE, FALSE, size);
		
		return;
	}
	else
	{
		// LD (two RM specifiers)
		rm_spec rm_src = opcode & 0x3f;
		rm_spec rm_dest = (opcode >> 6) & 0x3f;
		// fetch src (right operand)
		decode_rm_specifier(state, rm_src, FALSE, FALSE, size);
		// write into dest (left operand)
		// does not need to be fetched, hence it's not a source, so don't set src_is_left
		decode_rm_specifier(state, rm_dest, TRUE, FALSE, size);
		return;
	}
	
	decode_unreachable_();
	return;
}

static void
decode_inst_other_ (pilot_decode_state *state, uint16_t opcode)
{
	execute_control_word *core_op = &state->work_regs.core_op;
	data_size_spec size = ((opcode & 0xc000) >> 14);
	
	if (opcode == 0x0000)
	{
		// NOP
		core_op->srcs[0].location = DATA_ZERO;
		core_op->srcs[0].size = size;
		
		core_op->srcs[1].location = DATA_ZERO;
		core_op->srcs[1].size = size;
		
		core_op->operation = ALU_OR;
		core_op->shifter_mode = SHIFTER_NONE;
		core_op->dest = DATA_ZERO;
		return;
	}
	if (opcode == 0x0001)
	{
		// HALT
		decode_not_implemented_();
		return;
	}
	if ((opcode & 0x8fc0) == 0x0100)
	{
		// LD.B F, rm8 / LD.W WF, rm16
		rm_spec rm_src = opcode & 0x3f;
		
		core_op->srcs[0].location = DATA_ZERO;
		core_op->srcs[0].size = size;
		
		core_op->operation = ALU_OR;
		core_op->shifter_mode = SHIFTER_NONE;
		
		decode_rm_specifier(state, rm_src, FALSE, FALSE, size);
		core_op->dest = (size == SIZE_16_BIT) ? DATA_REG_WF : DATA_REG_F;
		return;
	}
	if ((opcode & 0x8fc0) == 0x0700)
	{
		// LD.B rm8, F / LD.W rm16, WF
		rm_spec rm_dst = opcode & 0x3f;
		
		core_op->srcs[0].location = DATA_ZERO;
		core_op->srcs[0].size = size;
		core_op->srcs[1].location = (size == SIZE_16_BIT) ? DATA_REG_WF : DATA_REG_F;
		core_op->srcs[1].size = size;
		
		core_op->operation = ALU_OR;
		core_op->shifter_mode = SHIFTER_NONE;
		core_op->flag_write_mask = 0;
		
		decode_rm_specifier(state, rm_dst, TRUE, FALSE, size);
		return;
	}
	if ((opcode & 0x3840) == 0x0040)
	{
		// ADQ / SBQ
		rm_spec rm_rmw = opcode & 0x3f;
		
		core_op->srcs[1].location = DATA_LATCH_SFI_2;
		core_op->srcs[1].size = size;
		
		core_op->operation = ALU_ADD;
		core_op->shifter_mode = SHIFTER_NONE;
		core_op->flag_write_mask = F_SIGN | F_ZERO | F_OVERFLOW | F_CARRY;
		core_op->flag_v_mode = FLAG_V_NORMAL;
		core_op->flag_z_mode = FLAG_Z_NORMAL;
		
		core_op->src2_add1 = TRUE;
		core_op->src2_negate = ((opcode & 0x0080) != 0);
		
		decode_rm_specifier(state, rm_rmw, TRUE, TRUE, size);
		return;
	}
	if ((opcode & 0x38c0) == 0x0080)
	{
		rm_spec rm_rmw = opcode & 0x3f;
		
		core_op->srcs[0].location = DATA_ZERO;
		core_op->srcs[0].size = size;
		
		core_op->operation = ALU_OR;
		core_op->flag_write_mask = F_SIGN | F_ZERO | F_OVERFLOW | F_CARRY | F_EXTEND;
		core_op->flag_v_mode = FLAG_V_NORMAL;
		core_op->flag_z_mode = FLAG_Z_NORMAL;
		
		switch ((opcode & 0x0700) >> 8)
		{
			case 0:
				// RLC
				core_op->shifter_mode = SHIFTER_LEFT_CARRY;
				break;
			case 1:
				// RRC
				core_op->shifter_mode = SHIFTER_RIGHT_CARRY;
				break;
			case 2:
				// RL
				core_op->shifter_mode = SHIFTER_LEFT_BARREL;
				break;
			case 3:
				// RR
				core_op->shifter_mode = SHIFTER_RIGHT_BARREL;
				break;
			case 4:
				// SLA
				core_op->shifter_mode = SHIFTER_LEFT;
				break;
			case 5:
				// SRA
				core_op->shifter_mode = SHIFTER_RIGHT_ARITH;
				break;
			case 6:
				// SWAP
				core_op->shifter_mode = SHIFTER_SWAP;
				break;
			case 7:
				// SRL
				core_op->shifter_mode = SHIFTER_RIGHT_LOGICAL;
				break;
		}
		
		decode_rm_specifier(state, rm_rmw, TRUE, TRUE, size);
		return;
	}
	if ((opcode & 0x3fc0) == 0x0800)
	{
		// TST
		rm_spec rm_src = opcode & 0x3f;
		core_op->srcs[0].location = DATA_REG_P0;
		core_op->srcs[0].size = size;
		
		core_op->operation = ALU_AND;
		core_op->flag_write_mask = F_SIGN | F_ZERO | F_OVERFLOW;
		core_op->flag_v_mode = FLAG_V_SHIFTER_CARRY;
		core_op->flag_z_mode = FLAG_Z_NORMAL;
		
		decode_rm_specifier(state, rm_src, FALSE, FALSE, size);
		core_op->dest = DATA_ZERO;
		return;
	}
	if ((opcode & 0x3fc0) == 0x0840)
	{
		// CPL
		rm_spec rm_rmw = opcode & 0x3f;
		core_op->srcs[0].location = DATA_ZERO;
		core_op->srcs[0].size = size;
		
		core_op->operation = ALU_OR;
		core_op->flag_write_mask = F_SIGN | F_ZERO | F_OVERFLOW;
		core_op->flag_v_mode = FLAG_V_SHIFTER_CARRY;
		core_op->flag_z_mode = FLAG_Z_NORMAL;
		
		decode_rm_specifier(state, rm_rmw, TRUE, TRUE, size);
		core_op->src2_add1 = TRUE;
		core_op->src2_negate = TRUE;
		core_op->dest = DATA_ZERO;
		return;
	}
	if ((opcode & 0x3fc0) == 0x0880)
	{
		// NEG
		rm_spec rm_rmw = opcode & 0x3f;
		core_op->srcs[0].location = DATA_ZERO;
		core_op->srcs[0].size = size;
		
		core_op->operation = ALU_OR;
		core_op->flag_write_mask = F_SIGN | F_ZERO | F_OVERFLOW;
		core_op->flag_v_mode = FLAG_V_SHIFTER_CARRY;
		core_op->flag_z_mode = FLAG_Z_NORMAL;
		
		decode_rm_specifier(state, rm_rmw, TRUE, TRUE, size);
		core_op->src2_negate = TRUE;
		core_op->dest = DATA_ZERO;
		return;
	}
	if ((opcode & 0x3f00) == 0x0800)
	{
		// NGX
		rm_spec rm_rmw = opcode & 0x3f;
		core_op->srcs[0].location = DATA_ZERO;
		core_op->srcs[0].size = size;
		
		core_op->operation = ALU_OR;
		core_op->flag_write_mask = F_SIGN | F_ZERO | F_OVERFLOW;
		core_op->flag_v_mode = FLAG_V_SHIFTER_CARRY;
		core_op->flag_z_mode = FLAG_Z_NORMAL;
		
		decode_rm_specifier(state, rm_rmw, TRUE, TRUE, size);
		core_op->src2_add_carry = TRUE;
		core_op->src2_negate = (opcode & 0x0400) != 0;
		core_op->dest = DATA_ZERO;
		return;
	}
	if ((opcode & 0x3880) == 0x0800)
	{
		// MULU / MULS
		decode_not_implemented_();
		return;
	}
	if ((opcode & 0x38c0) == 0x0880)
	{
		// DIVU / DIVS
		decode_not_implemented_();
		return;
	}
	
	decode_invalid_opcode_(state->sys);
	return;
}

static void
decode_inst_ (pilot_decode_state *state)
{
	state->rm_ops = 0;
	uint16_t opcode = state->work_regs.imm_words[0];
	
	if ((opcode & 0xf000) >= 0xe000)
	{
		// Branch instructions
		decode_inst_branch_(state, opcode);
	}
	else if ((opcode & 0xf000) == 0xd000)
	{
		// Bit operations
		decode_inst_bit_(state, opcode);
	}
	else if ((opcode & 0xf000) == 0xc000)
	{
		// Two miscellaneous LD instructions
		decode_inst_ld_other_(state, opcode);
	}
	else if ((opcode & 0x2000) == 0x2000)
	{
		// Arithmetic/logic instructions
		decode_inst_arithlogic_(state, opcode);
	}
	else if ((opcode & 0x3000) == 0x1000)
	{
		// Main group of LD instructions
		decode_inst_ld_group_(state, opcode);
	}
	else if ((opcode & 0x3000) == 0x0000)
	{
		// Everything else
		decode_inst_other_(state, opcode);
	}
}

void
decode_queue_read_word (pilot_decode_state *state)
{
	state->inst_length++;
	state->words_to_read++;
}

void
pilot_decode_half1 (pilot_decode_state *state)
{
	if (state->decoding_phase == DECODER_HALF1_DISPATCH_WAIT)
	{
		bool *decoded_inst_semaph = &state->sys->interconnects.decoded_inst_semaph;
		if (!(*decoded_inst_semaph))
		{
			state->decoding_phase = DECODER_HALF1_READY;
		}
	}
	
	if (state->decoding_phase == DECODER_HALF1_READY)
	{
		state->inst_length = 0;
		state->decoding_phase = DECODER_HALF1_READ_INST_WORD;
	}
	
	if (state->decoding_phase == DECODER_HALF1_READ_INST_WORD)
	{
		bool read_ok = decode_try_read_word_(state);
		if (read_ok)
		{
			state->inst_length++;
			decode_inst_(state);
			state->decoding_phase = DECODER_HALF2_READ_OPERANDS;
		}
	}
}


void
pilot_decode_half2 (pilot_decode_state *state)
{
	if (state->decoding_phase == DECODER_HALF2_READ_OPERANDS)
	{
		if (state->words_to_read > 0)
		{
			bool read_ok = decode_try_read_word_(state);
			if (read_ok)
			{
				state->words_to_read--;
			}
		}
		
		if (state->words_to_read == 0)
		{
			state->decoding_phase = DECODER_HALF2_DISPATCH;
		}
	}
	
	if (state->decoding_phase == DECODER_HALF2_DISPATCH)
	{
		state->work_regs.inst_pgc = state->pgc;
		bool *decoded_inst_semaph = &state->sys->interconnects.decoded_inst_semaph;
		*decoded_inst_semaph = TRUE;
		state->decoding_phase = DECODER_HALF1_DISPATCH_WAIT;
	}
}

