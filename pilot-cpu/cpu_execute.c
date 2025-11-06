#include "cpu_regs.h"
#include "cpu_execute.h"
#include "cpu_mucode.h"
#include "memory.h"
#include "types.h"

#define ACCESS_REG_BITS_(state, r, size) state->sys->core.regs[r] & (((size) == SIZE_8_BIT) ? 0xff : (((size) == SIZE_16_BIT) ? 0xffff : 0xffffff))
#define READ_IMM_LATCH_(state, imm, size) (size == SIZE_24_BIT ? (((state->decoded_inst.imm_words[imm + 1] & 0xff) << 16) | state->decoded_inst.imm_words[imm]) : state->decoded_inst.imm_words[imm])

static void
execute_invalid_opcode_ (pilot_execute_state *state)
{
	state->sequencer_phase = EXEC_SEQ_EVAL_CONTROL;
	state->decoded_inst.illegal = TRUE;
}

static uint32_t
fetch_data_ (pilot_execute_state *state, alu_src_control src)
{
	switch (src.location)
	{
		case DATA_ZERO:
			return 0;
		case DATA_LATCH_CARRY:
		{
			if ((state->sys->core.wf & F_CARRY) == 0)
			{
				return 0;
			}
			else
			{
				data_size_spec size = src.size;
				if (size == SIZE_8_BIT)
					return 0xff;
				else if (size == SIZE_16_BIT)
					return 0xffff;
				else
					return 0xffffff;
			}
		}
		case DATA_SIZE:
		{
			data_size_spec size = src.size;
			if (size == SIZE_8_BIT)
			{
				if (state->mucode_decoded_buffer.operation.srcs[0].location == DATA_REG_SP || state->mucode_decoded_buffer.operation.srcs[1].location == DATA_REG_SP)
					return 2;
				else
					return 1;
			}
			else if (size == SIZE_16_BIT)
				return 2;
			else
			{
				if (state->mucode_decoded_buffer.operation.srcs[0].location == DATA_REG_SP || state->mucode_decoded_buffer.operation.srcs[1].location == DATA_REG_SP)
					return 2;
				else
					return 4;
			}
		}
		case DATA_NUM_BITS:
		{
			data_size_spec size = src.size;
			if (size == SIZE_8_BIT)
				return 8;
			else if (size == SIZE_16_BIT)
				return 16;
			else
				return 24;
		}
		case DATA_REG_L0:
			return state->sys->core.regs[0] & 0xff;
		case DATA_REG_L1:
			return state->sys->core.regs[1] & 0xff;
		case DATA_REG_L2:
			return state->sys->core.regs[2] & 0xff;
		case DATA_REG_L3:
			return state->sys->core.regs[3] & 0xff;
		case DATA_REG_M0:
			return (state->sys->core.regs[0] >> 8) & 0xff;
		case DATA_REG_M1:
			return (state->sys->core.regs[1] >> 8) & 0xff;
		case DATA_REG_M2:
			return (state->sys->core.regs[2] >> 8) & 0xff;
		case DATA_REG_M3:
			return (state->sys->core.regs[3] >> 8) & 0xff;
		case DATA_REG_W0:
			return state->sys->core.regs[0] & 0xffff;
		case DATA_REG_W1:
			return state->sys->core.regs[1] & 0xffff;
		case DATA_REG_W2:
			return state->sys->core.regs[2] & 0xffff;
		case DATA_REG_W3:
			return state->sys->core.regs[3] & 0xffff;
		case DATA_REG_W4:
			return state->sys->core.regs[4] & 0xffff;
		case DATA_REG_W5:
			return state->sys->core.regs[5] & 0xffff;
		case DATA_REG_W6:
			return state->sys->core.regs[6] & 0xffff;
		case DATA_REG_W7:
			return state->sys->core.regs[7] & 0xffff;
		case DATA_REG_P0:
			return state->sys->core.regs[0];
		case DATA_REG_P1:
			return state->sys->core.regs[1];
		case DATA_REG_P2:
			return state->sys->core.regs[2];
		case DATA_REG_P3:
			return state->sys->core.regs[3];
		case DATA_REG_P4:
			return state->sys->core.regs[4];
		case DATA_REG_P5:
			return state->sys->core.regs[5];
		case DATA_REG_P6:
			return state->sys->core.regs[6];
		case DATA_REG_SP:
			return state->sys->core.regs[7];
		case DATA_REG_PGC:
			return state->sys->core.pgc;
		case DATA_REG_F:
			return state->sys->core.wf & 0xff;
		case DATA_REG_IRL:
			return (state->sys->core.wf >> 8) & 0x7;
		case DATA_REG_WF:
			return state->sys->core.wf;
		case DATA_LATCH_REPI:
			return state->sys->core.repi;
		case DATA_LATCH_REPR:
			return state->sys->core.repr;
		case DATA_LATCH_FACTOR_A:
			return state->sys->core.factor_a;
		case DATA_LATCH_FACTOR_B:
			return state->sys->core.factor_b;
		case DATA_REG_R0:
			return ACCESS_REG_BITS_(state, 0, src.size);
		case DATA_LATCH_MEM_ADDR:
			return state->mem_addr;
		case DATA_LATCH_MEM_DATA:
			return state->mem_data;
		case DATA_LATCH_IMM_0:
			return READ_IMM_LATCH_(state, 0, src.size);
		case DATA_LATCH_IMM_1:
			return READ_IMM_LATCH_(state, 1, src.size);
		case DATA_LATCH_IMM_2:
			return READ_IMM_LATCH_(state, 2, src.size);
		case DATA_LATCH_IMM_HML:
			return ((state->decoded_inst.imm_words[0] & 0xff) << 16) | state->decoded_inst.imm_words[1];
		case DATA_LATCH_IMM_HML_RM:
			return ((state->decoded_inst.imm_words[2] & 0xff) << 16) | state->decoded_inst.imm_words[1];
		case DATA_LATCH_SFI_1:
			return (state->decoded_inst.imm_words[0] >> 2) & 0x000f;
		case DATA_LATCH_SFI_2:
			return (state->decoded_inst.imm_words[0] >> 8) & 0x000f;
		case DATA_LATCH_RM_1:
			return READ_IMM_LATCH_(state, state->decoded_inst.rm2_offset, src.size);
		case DATA_LATCH_RM_2:
			return READ_IMM_LATCH_(state, state->decoded_inst.rm2_offset + 1, src.size);
		case DATA_LATCH_RM_HML:
			return ((state->decoded_inst.imm_words[state->decoded_inst.rm2_offset + 1] & 0xff) << 16) | state->decoded_inst.imm_words[state->decoded_inst.rm2_offset];
		case DATA_REG_IMM_0_8:
			return ACCESS_REG_BITS_(state, (state->decoded_inst.imm_words[0] >> 8) & 0x7, src.size);
		case DATA_REG_IMM_1_8:
			return ACCESS_REG_BITS_(state, (state->decoded_inst.imm_words[1] >> 8) & 0x7, src.size);
		case DATA_REG_IMM_1_2:
			return ACCESS_REG_BITS_(state, (state->decoded_inst.imm_words[1] >> 2) & 0x7, src.size);
		case DATA_REG_IMM_2_8:
		{
			if (state->decoded_inst.imm_words[2] >= 0xc000) 
			{
				execute_invalid_opcode_(state);
				return 0;
			}
			state->mucode_decoded_buffer.operation.srcs[1].sign_extend = ((state->decoded_inst.imm_words[2] & 0x0800) != 0);
			return ACCESS_REG_BITS_(state, (state->decoded_inst.imm_words[2] >> 8) & 0x7, state->decoded_inst.imm_words[2] >> 14);
		}
		case DATA_REG_RM_1_8:
			return ACCESS_REG_BITS_(state, (state->decoded_inst.imm_words[state->decoded_inst.rm2_offset] >> 8) & 0x7, src.size);
		case DATA_REG_RM_1_2:
			return ACCESS_REG_BITS_(state, (state->decoded_inst.imm_words[state->decoded_inst.rm2_offset] >> 2) & 0x7, src.size);
		case DATA_REG_RM_2_8:
		{
			if (state->decoded_inst.imm_words[state->decoded_inst.rm2_offset + 1] >= 0xc000) 
			{
				execute_invalid_opcode_(state);
				return 0;
			}
			state->mucode_decoded_buffer.operation.srcs[1].sign_extend = ((state->decoded_inst.imm_words[state->decoded_inst.rm2_offset + 1] & 0x0800) != 0);
			return ACCESS_REG_BITS_(state, (state->decoded_inst.imm_words[state->decoded_inst.rm2_offset + 1] >> 8) & 0x7, state->decoded_inst.imm_words[state->decoded_inst.rm2_offset + 1] >> 14);
		}
		case DATA_REG_REPR:
			return ACCESS_REG_BITS_(state, state->sys->core.repr, src.size);
		case DATA_DMX_IMM_BITS:
			return 1 << ((state->decoded_inst.imm_words[0] >> 8) & 0x7);
		case DATA_DMX_P0_BITS:
			return 1 << ((state->sys->core.regs[0] >> 8) & 0x7);
		default:
			execute_unreachable_();
	}
}

static void
write_data_ (pilot_execute_state *state, alu_src_control dest, uint32_t *src)
{
	switch (dest.location)
	{
		case DATA_ZERO:
		case DATA_LATCH_CARRY:
		case DATA_SIZE:
			return;
		case DATA_REG_L0:
			state->sys->core.regs[0] &= 0xffff00;
			state->sys->core.regs[0] |= *src & 0xff;
			return;
		case DATA_REG_L1:
			state->sys->core.regs[1] &= 0xffff00;
			state->sys->core.regs[1] |= *src & 0xff;
			return;
		case DATA_REG_L2:
			state->sys->core.regs[2] &= 0xffff00;
			state->sys->core.regs[2] |= *src & 0xff;
			return;
		case DATA_REG_L3:
			state->sys->core.regs[3] &= 0xffff00;
			state->sys->core.regs[3] |= *src & 0xff;
			return;
		case DATA_REG_M0:
			state->sys->core.regs[0] &= 0xff00ff;
			state->sys->core.regs[0] |= ((*src & 0xff) << 8);
			return;
		case DATA_REG_M1:
			state->sys->core.regs[1] &= 0xff00ff;
			state->sys->core.regs[1] |= ((*src & 0xff) << 8);
			return;
		case DATA_REG_M2:
			state->sys->core.regs[2] &= 0xff00ff;
			state->sys->core.regs[2] |= ((*src & 0xff) << 8);
			return;
		case DATA_REG_M3:
			state->sys->core.regs[3] &= 0xff00ff;
			state->sys->core.regs[3] |= ((*src & 0xff) << 8);
			return;
		case DATA_REG_W0:
			state->sys->core.regs[0] &= 0xff0000;
			state->sys->core.regs[0] |= *src & 0xffff;
			return;
		case DATA_REG_W1:
			state->sys->core.regs[1] &= 0xff0000;
			state->sys->core.regs[1] |= *src & 0xffff;
			return;
		case DATA_REG_W2:
			state->sys->core.regs[2] &= 0xff0000;
			state->sys->core.regs[2] |= *src & 0xffff;
			return;
		case DATA_REG_W3:
			state->sys->core.regs[3] &= 0xff0000;
			state->sys->core.regs[3] |= *src & 0xffff;
			return;
		case DATA_REG_W4:
			state->sys->core.regs[4] &= 0xff0000;
			state->sys->core.regs[4] |= *src & 0xffff;
			return;
		case DATA_REG_W5:
			state->sys->core.regs[5] &= 0xff0000;
			state->sys->core.regs[5] |= *src & 0xffff;
			return;
		case DATA_REG_W6:
			state->sys->core.regs[6] &= 0xff0000;
			state->sys->core.regs[6] |= *src & 0xffff;
			return;
		case DATA_REG_W7:
			state->sys->core.regs[7] &= 0xff0000;
			state->sys->core.regs[7] |= *src & 0xffff;
			return;
		case DATA_REG_P0:
			state->sys->core.regs[0] = *src & 0xffffff;
			return;
		case DATA_REG_P1:
			state->sys->core.regs[1] = *src & 0xffffff;
			return;
		case DATA_REG_P2:
			state->sys->core.regs[2] = *src & 0xffffff;
			return;
		case DATA_REG_P3:
			state->sys->core.regs[3] = *src & 0xffffff;
			return;
		case DATA_REG_P4:
			state->sys->core.regs[4] = *src & 0xffffff;
			return;
		case DATA_REG_P5:
			state->sys->core.regs[5] = *src & 0xffffff;
			return;
		case DATA_REG_P6:
			state->sys->core.regs[6] = *src & 0xffffff;
			return;
		case DATA_REG_SP:
			state->sys->core.regs[7] = *src & 0xffffff;
			return;
		case DATA_REG_F:
			state->sys->core.wf &= 0xff00;
			state->sys->core.wf |= *src & 0xff;
			return;
		case DATA_REG_IRL:
			state->sys->core.wf &= 0xff;
			state->sys->core.wf |= (*src << 8) & 0x7;
			return;
		case DATA_REG_WF:
			state->sys->core.wf = *src & 0xffff;
			return;
		case DATA_LATCH_REPI:
			state->sys->core.repi = *src & 0x1f;
			return;
		case DATA_LATCH_REPR:
			state->sys->core.repr = (*src >> 8) & 0x7;
			return;
		case DATA_LATCH_FACTOR_A:
			state->sys->core.factor_a = *src & 0xffffff;
			return;
		case DATA_LATCH_FACTOR_B:
			state->sys->core.factor_b = *src & 0xffffff;
			return;
		case DATA_REG_R0:
		{
			switch (dest.size)
			{
				case SIZE_8_BIT:
					state->sys->core.regs[0] &= 0xffff00;
					state->sys->core.regs[0] |= *src & 0xff;
					return;
				case SIZE_16_BIT:
					state->sys->core.regs[0] &= 0xff0000;
					state->sys->core.regs[0] |= *src & 0xffff;
					return;
				case SIZE_24_BIT:
					state->sys->core.regs[0] = *src & 0xffffff;
					return;
				default:
					execute_unreachable_();
					return;
			}
		}
		case DATA_REG_PGC:
			state->sys->core.pgc = *src & 0xfffffe;
			state->branched = TRUE;
			state->sys->interconnects.execute_branch_addr = *src & 0xfffffe;
			state->sys->interconnects.execute_branch = TRUE;
			return;
		case DATA_LATCH_MEM_ADDR:
			state->mem_addr = *src & 0xffffff;
			return;
		case DATA_LATCH_MEM_DATA:
			state->mem_data = *src & 0xffffff;
			return;
		case DATA_REG_IMM_0_8:
		{
			switch (dest.size)
			{
				case SIZE_8_BIT:
					state->sys->core.regs[(state->decoded_inst.imm_words[0] >> 8) & 0x7] &= 0xffff00;
					state->sys->core.regs[(state->decoded_inst.imm_words[0] >> 8) & 0x7] |= *src & 0xff;
					return;
				case SIZE_16_BIT:
					state->sys->core.regs[(state->decoded_inst.imm_words[0] >> 8) & 0x7] &= 0xff0000;
					state->sys->core.regs[(state->decoded_inst.imm_words[0] >> 8) & 0x7] |= *src & 0xffff;
					return;
				case SIZE_24_BIT:
					state->sys->core.regs[(state->decoded_inst.imm_words[0] >> 8) & 0x7] = *src & 0xffffff;
					return;
				default:
					execute_unreachable_();
					return;
			}
		}
		case DATA_REG_IMM_1_8:
			state->sys->core.regs[(state->decoded_inst.imm_words[1] >> 8) & 0x7] = *src & 0xffffff;
			return;
		case DATA_REG_IMM_1_2:
			state->sys->core.regs[(state->decoded_inst.imm_words[1] >> 2) & 0x7] = *src & 0xffffff;
			return;
		case DATA_REG_RM_1_8:
			state->sys->core.regs[(state->decoded_inst.imm_words[state->decoded_inst.rm2_offset] >> 8) & 0x7] = *src & 0xffffff;
			return;
		case DATA_REG_RM_1_2:
			state->sys->core.regs[(state->decoded_inst.imm_words[state->decoded_inst.rm2_offset] >> 2) & 0x7] = *src & 0xffffff;
			return;
		case DATA_REG_REPR:
			state->sys->core.regs[state->sys->core.repr] = *src;
			return;
		case DATA_REG_IMM_2_8:
		case DATA_REG_RM_2_8:
		case DATA_LATCH_IMM_0:
		case DATA_LATCH_IMM_1:
		case DATA_LATCH_IMM_2:
		case DATA_LATCH_IMM_HML:
		case DATA_LATCH_IMM_HML_RM:
		case DATA_LATCH_SFI_1:
		case DATA_LATCH_SFI_2:
		case DATA_LATCH_RM_1:
		case DATA_LATCH_RM_2:
		case DATA_LATCH_RM_HML:
		case DATA_DMX_IMM_BITS:
		case DATA_DMX_P0_BITS:
			return;
		default:
			execute_unreachable_();
	}
}

static void
execute_half1_mem_wait_ (pilot_execute_state *state)
{
	if (state->mem_access_waiting)
	{
		if (Pilot_mem_data_wait(state->sys))
		{
			state->mem_access_waiting = FALSE;
			if (state->mem_access_was_read)
			{
				state->mem_data = Pilot_mem_get_data(state->sys);
			}
		}
		else if (state->control->srcs[0].location == DATA_LATCH_MEM_DATA
			|| state->control->srcs[1].location == DATA_LATCH_MEM_DATA
			|| state->control->dest.location == DATA_LATCH_MEM_DATA)
		{
			return;
		}
	}
	state->execution_phase = EXEC_HALF1_OPERAND_LATCH;
}

static void
execute_half1_mem_prepare_ (pilot_execute_state *state)
{
	if (state->control->mem_latch_ctl == MEM_LATCH_HALF1)
	{
		state->mem_addr = state->alu_input_latches[0];
		
		if (state->control->mem_write_ctl != MEM_READ)
		{
			switch (state->control->mem_write_ctl)
			{
				case MEM_WRITE_FROM_SRC2:
					state->mem_data = state->alu_input_latches[1];
					break;
				case MEM_WRITE_FROM_DEST:
					// MEM_WRITE_FROM_DEST should only be used in cycle 2!
					execute_unreachable_();
					state->mem_data = state->alu_output_latch;
					break;
				case MEM_WRITE_FROM_MDR:
					break;
				case MEM_WRITE_FROM_MDR_HIGH:
					state->mem_data = (state->mem_data >> 16) & 0xff;
					break;
				default:
					execute_unreachable_();
			}
		}
	}
	state->execution_phase = EXEC_HALF1_MEM_ASSERT;
}

static void
execute_half1_mem_assert_ (pilot_execute_state *state)
{
	if (state->control->mem_latch_ctl == MEM_LATCH_HALF1 && !state->control->mem_access_suppress)
	{
		if (state->control->mem_write_ctl == MEM_READ)
		{
			if (!Pilot_mem_addr_read_assert(state->sys, state->control->is_16bit, state->mem_addr))
			{
				return;
			}
			state->mem_access_was_read = TRUE;
		}
		else
		{
			if (!Pilot_mem_addr_write_assert(state->sys, state->control->is_16bit, state->mem_addr, state->mem_data & 0xffff))
			{
				return;
			}
			state->mem_access_was_read = FALSE;
		}
		
		state->mem_access_waiting = TRUE;
		state->control->mem_latch_ctl = MEM_NO_LATCH;
	}
	state->execution_phase = EXEC_HALF2_READY;
}

void
pilot_execute_half1 (pilot_execute_state *state)
{
	if (state->sys->core.disable_clk)
	{
		return;
	}
	
	if (state->execution_phase == EXEC_HALF1_READY)
	{
		state->execution_phase = EXEC_HALF1_MEM_WAIT;
	}
	
	if (state->execution_phase == EXEC_HALF1_MEM_WAIT)
	{
		execute_half1_mem_wait_(state);
	}
	
	if (state->execution_phase == EXEC_HALF1_OPERAND_LATCH)
	{
		state->alu_input_latches[0] = fetch_data_(state, state->control->srcs[0]);
		state->alu_input_latches[1] = fetch_data_(state, state->control->srcs[1]);
		state->execution_phase = EXEC_HALF1_MEM_PREPARE;
	}
	
	if (state->execution_phase == EXEC_HALF1_MEM_PREPARE)
	{
		execute_half1_mem_prepare_(state);
	}
	
	if (state->execution_phase == EXEC_HALF1_MEM_ASSERT)
	{
		execute_half1_mem_assert_(state);
	}
}

static inline uint32_t
alu_operate_shifter_ (pilot_execute_state *state, uint32_t operand)
{
	bool inject_bit;
	bool msb_bit;
	bool lsb_bit = operand & 1;
	bool carry_flag = !(state->control->latch_aux_mode == LATCH_AUX_CARRY) ? ((fetch_data_(state, (alu_src_control){DATA_REG_F, SIZE_8_BIT, FALSE}) & F_EXTEND) != 0) : state->sys->core.latch_aux;
	
	if (state->control->srcs[1].size == SIZE_8_BIT)
	{
		msb_bit = (operand & 0x80) != 0;
	}
	else if (state->control->srcs[1].size == SIZE_16_BIT)
	{
		msb_bit = (operand & 0x8000) != 0;
	}
	else
	{
		msb_bit = (operand & 0x800000) != 0;
	}
	
	switch (state->control->shifter_mode)
	{
		case SHIFTER_NONE:
		case SHIFTER_LEFT:
		case SHIFTER_RIGHT_LOGICAL:
			inject_bit = 0;
			break;
		case SHIFTER_LEFT_CARRY:
		case SHIFTER_RIGHT_CARRY:
			inject_bit = carry_flag;
			break;
		case SHIFTER_LEFT_BARREL:
		case SHIFTER_RIGHT_ARITH:
			inject_bit = msb_bit;
			break;
		case SHIFTER_RIGHT_BARREL:
			inject_bit = lsb_bit;
			break;
		case SHIFTER_SWAP:
			break;
		default:
			execute_unreachable_();
	}
	
	switch (state->control->shifter_mode)
	{
		case SHIFTER_NONE:
			break;
		case SHIFTER_LEFT:
		case SHIFTER_LEFT_CARRY:
		case SHIFTER_LEFT_BARREL:
			state->alu_shifter_carry_bit = msb_bit ^ state->control->invert_carries;
			operand <<= 1;
			operand |= inject_bit;
			break;
		case SHIFTER_RIGHT_LOGICAL:
		case SHIFTER_RIGHT_ARITH:
		case SHIFTER_RIGHT_CARRY:
		case SHIFTER_RIGHT_BARREL:
			state->alu_shifter_carry_bit = lsb_bit ^ state->control->invert_carries;
			operand >>= 1;
			if (state->control->srcs[1].size == SIZE_8_BIT)
			{
				operand |= inject_bit << 7;
			}
			else if (state->control->srcs[1].size == SIZE_16_BIT)
			{
				operand |= inject_bit << 15;
			}
			else
			{
				operand |= inject_bit << 23;
			}
			break;
		case SHIFTER_SWAP:
			if (state->control->srcs[1].size == SIZE_8_BIT)
			{
				operand = ((operand & 0xf0) >> 4) | ((operand & 0x0f) << 4);
			}
			else if (state->control->srcs[1].size == SIZE_16_BIT)
			{
				operand = ((operand & 0xff00) >> 8) | ((operand & 0x00ff) << 8);
			}
			else
			{
				operand = ((operand & 0xff0000) >> 16) | (operand & 0xff00) | ((operand & 0x0000ff) << 16);
			}
			break;
		default:
			execute_unreachable_();
	}
	
	return operand;
}

static inline uint8_t
alu_modify_flags_ (pilot_execute_state *state, uint8_t flags, uint32_t operands[2], uint32_t result, uint32_t carries)
{
	bool alu_carry;
	bool alu_overflow;
	bool alu_zero;
	bool alu_sign;
	uint8_t flag_source_word = 0;
	
	uint32_t alu_parity = result;
	alu_parity = alu_parity ^ alu_parity >> 4;
	alu_parity = alu_parity ^ alu_parity >> 2;
	alu_parity = alu_parity ^ alu_parity >> 1;
	
	if (state->control->srcs[0].size == SIZE_8_BIT)
	{
		alu_carry = (carries & 0x80) != 0;
		alu_sign = (result & 0x80) != 0;
		alu_overflow = ((operands[1] ^ result) & (operands[0] ^ result) & 0x80) != 0;
		alu_zero = (result & 0xff) == 0;
	}
	else if (state->control->srcs[0].size == SIZE_16_BIT)
	{
		alu_carry = (carries & 0x8000) != 0;
		alu_sign = (result & 0x8000) != 0;
		alu_overflow = ((operands[1] ^ result) & (operands[0] ^ result) & 0x8000) != 0;
		alu_zero = (result & 0xffff) == 0;
		alu_parity ^= (alu_parity >> 8);
	}
	else
	{
		alu_carry = (carries & 0x800000) != 0;
		alu_sign = (result & 0x800000) != 0;
		alu_overflow = ((operands[1] ^ result) & (operands[0] ^ result) & 0x800000) != 0;
		alu_zero = (result & 0xffffff) == 0;
		alu_parity ^= (alu_parity >> 8) ^ (alu_parity >> 16);
	}
	alu_carry ^= state->control->invert_carries;
	
	// S - Sign/negative flag
	flag_source_word |= (alu_sign) ? F_SIGN : 0;
	// Z - Zero flag
	switch (state->control->flag_z_mode)
	{
		case FLAG_Z_NORMAL:
			flag_source_word |= (alu_zero) ? F_ZERO : 0;
			state->used_z = alu_zero;
			break;
		case FLAG_Z_ACCUM:
			flag_source_word |= (state->used_z && alu_zero) ? F_ZERO : 0;
			state->used_z = state->used_z && alu_zero;
			break;
		case FLAG_Z_BIT_TEST:
			flag_source_word |= ((operands[0] & operands[1]) == 0) ? F_ZERO : 0;
			state->used_z = ((operands[0] & operands[1]) == 0);
			break;
		default:
			execute_unreachable_();
	}
	
	switch (state->control->latch_aux_mode)
	{
		case LATCH_AUX_NONE:
			break;
		case LATCH_AUX_CLEAR:
			state->sys->core.latch_aux = FALSE;
			break;
		case LATCH_AUX_ZERO:
			state->sys->core.latch_aux = state->used_z;
			break;
		case LATCH_AUX_CARRY:
			state->sys->core.latch_aux = !(state->control->operation == ALU_ADD) ? (state->alu_shifter_carry_bit != 0) : alu_carry;
			break;
		default:
			execute_unreachable_();
	}
	
	// V - Overflow/parity flag
	switch (state->control->flag_v_mode)
	{
		case FLAG_V_NORMAL:
			flag_source_word |= (!(state->control->operation == ALU_ADD) ? alu_parity : alu_overflow) ? F_OVERFLOW : 0;
			break;
		case FLAG_V_CLEAR:
			break;
		case FLAG_V_SHIFTER_CARRY:
			flag_source_word |= (state->alu_shifter_carry_bit != 0) ? F_OVERFLOW : 0;
			break;
		case FLAG_V_ACCUM:
			flag_source_word |= ((!(state->control->operation == ALU_ADD) ? alu_parity : alu_overflow) || (flags & F_OVERFLOW) != 0) ? F_OVERFLOW : 0;
			break;
		default:
			execute_unreachable_();
	}
	
	// C, X - Carry/borrow flags
	flag_source_word |= (alu_carry) ? F_CARRY : 0;
	flag_source_word |= (alu_carry) ? F_EXTEND : 0;
	
	flags &= ~state->control->flag_write_mask;
	flags |= (flag_source_word & state->control->flag_write_mask);
	
	return flags;
}

static void
execute_half2_result_latch_ (pilot_execute_state *state)
{
	int i;
	uint32_t operands[2];
	uint32_t carries;
	
	alu_src_control *src2 = &state->control->srcs[1];
	uint8_t flags = fetch_data_(state, (alu_src_control){DATA_REG_F, SIZE_8_BIT, FALSE});
	bool carry_flag_status = !(state->control->latch_aux_mode == LATCH_AUX_CARRY) ? ((flags & F_EXTEND) != 0) : state->sys->core.latch_aux;
	
	operands[0] = state->alu_input_latches[0];
	operands[1] = state->alu_input_latches[1];
	
	if (state->control->src2_add1)
	{
		operands[1] += 1;
	}
	
	if (state->control->src2_add_carry)
	{
		operands[1] += carry_flag_status;
	}
	
	if (state->control->src2_negate)
	{
		operands[1] = ~operands[1] + 1;
		if (src2->size == SIZE_8_BIT)
		{
			operands[1] &= 0xff;
		}
		else if (src2->size == SIZE_16_BIT)
		{
			operands[1] &= 0xffff;
		}
		else
		{
			operands[1] &= 0xffffff;
		}
	}
	
	if (state->control->src2_and_with_aux && state->sys->core.latch_aux == 0)
	{
		operands[1] = 0;
	}
	
	for (i = 0; i < 2; i++)
	{
		alu_src_control *src = &state->control->srcs[i];
		if (src->size == SIZE_8_BIT)
		{
			operands[i] &= 0xff;
			if (src->sign_extend && (operands[i] & 0x80))
			{
				operands[i] |= 0xffff00;
			}
		}
		else if (src->size == SIZE_16_BIT)
		{
			operands[i] &= 0xffff;
			if (src->sign_extend && (operands[i] & 0x8000))
			{
				operands[i] |= 0xff0000;
			}
		}
		else
		{
			operands[i] &= 0xffffff;
		}
	}
	
	state->alu_shifter_carry_bit = FALSE;
	operands[1] = alu_operate_shifter_(state, operands[1]);
	
	switch (state->control->operation)
	{
		case ALU_OFF:
			break;
		case ALU_ADD:
			state->alu_output_latch = (operands[0] + operands[1]) & 0xffffff;
			carries = operands[0] ^ operands[1] ^ state->alu_output_latch;
			break;
		case ALU_AND:
			state->alu_output_latch = operands[0] & operands[1];
			carries = 0;
			break;
		case ALU_OR:
			state->alu_output_latch = operands[0] | operands[1];
			carries = 0;
			break;
		case ALU_XOR:
			state->alu_output_latch = operands[0] ^ operands[1];
			carries = 0;
			break;
		default:
			execute_unreachable_();
	}
	
	if ((state->control->dest.size == SIZE_8_BIT) && (flags & F_DECIMAL) && (carries & 0x08))
	{
		state->alu_output_latch = state->alu_output_latch + 0x10;
		carries = (carries & 0x0f) | ((operands[0] ^ operands[1] ^ state->alu_output_latch) & 0xf0);
	}
	
	if (state->control->operation != ALU_OFF)
	{
		uint32_t flags_data = alu_modify_flags_(state, flags, operands, state->alu_output_latch, carries);
		write_data_(state, (alu_src_control){DATA_REG_F, SIZE_8_BIT, FALSE}, &flags_data);
		write_data_(state, state->control->dest, &state->alu_output_latch);
		
		state->control->operation = ALU_OFF;
	}
	
	state->execution_phase = EXEC_HALF2_MEM_PREPARE;
}

static void
execute_half2_mem_prepare_ (pilot_execute_state *state)
{
	if (state->control->mem_latch_ctl == MEM_LATCH_HALF2)
	{
		state->mem_addr = state->alu_output_latch;
	}
	if (state->control->mem_latch_ctl >= MEM_LATCH_HALF2)
	{
		if (state->control->mem_write_ctl != MEM_READ)
		{
			switch (state->control->mem_write_ctl)
			{
				case MEM_WRITE_FROM_SRC2:
					// MEM_WRITE_FROM_SRC2 should only be used in cycle 1!
					execute_unreachable_();
					state->mem_data = state->alu_input_latches[1];
					break;
				case MEM_WRITE_FROM_DEST:
					state->mem_data = state->alu_output_latch;
					break;
				case MEM_WRITE_FROM_MDR:
					break;
				case MEM_WRITE_FROM_MDR_HIGH:
					state->mem_data = (state->mem_data >> 16) & 0xff;
					break;
				default:
					execute_unreachable_();
			}
		}
	}
	
	state->execution_phase = EXEC_HALF2_MEM_ASSERT;
}

static void
execute_half2_mem_assert_ (pilot_execute_state *state)
{
	if (state->control->mem_latch_ctl >= MEM_LATCH_HALF2 && !state->control->mem_access_suppress)
	{
		if (state->control->mem_write_ctl == MEM_READ)
		{
			if (!Pilot_mem_addr_read_assert(state->sys, state->control->is_16bit, state->mem_addr))
			{
				return;
			}
			state->mem_access_was_read = TRUE;
		}
		else
		{
			if (!Pilot_mem_addr_write_assert(state->sys, state->control->is_16bit, state->mem_addr, state->mem_data & 0xffff))
			{
				return;
			}
			state->mem_access_was_read = FALSE;
		}
		state->mem_access_waiting = TRUE;
		state->control->mem_latch_ctl = MEM_NO_LATCH;
	}
	
	state->execution_phase = EXEC_HALF2_ADVANCE_SEQUENCER;
}

static bool
execute_sequencer_branch_test_ (pilot_execute_state *state)
{
	uint8_t flags = fetch_data_(state, (alu_src_control){DATA_REG_F, SIZE_8_BIT, FALSE});
	bool overflow = (flags & F_OVERFLOW) != 0;
	bool carry = (flags & F_CARRY) != 0;
	bool zero = (flags & F_ZERO) != 0;
	bool sign = (flags & F_SIGN) != 0;
	
	bool branched = TRUE;
	
	switch (state->mucode_decoded_buffer.branch_cond)
	{
		case COND_LE:
			branched = zero || (sign != overflow);
			break;
		case COND_GT:
			branched = (!zero) && (sign == overflow);
			break;
		case COND_LT:
			branched = sign != overflow;
			break;
		case COND_GE:
			branched = sign == overflow;
			break;
		case COND_U_LE:
			branched = carry || zero;
			break;
		case COND_U_GT:
			branched = (!carry) && (!zero);
			break;
		case COND_C:
			branched = carry;
			break;
		case COND_NC:
			branched = !carry;
			break;
		case COND_M:
			branched = sign;
			break;
		case COND_P:
			branched = !sign;
			break;
		case COND_OV:
			branched = overflow;
			break;
		case COND_NOV:
			branched = !overflow;
			break;
		case COND_Z:
			branched = zero;
			break;
		case COND_NZ:
			branched = !zero;
			break;
		case COND_ALWAYS:
		case COND_ALWAYS_CALL:
			branched = TRUE;
			break;
		case COND_DJNZ:
			branched = !state->sys->core.latch_aux;
			break;
		default:
			execute_unreachable_();
			return FALSE;
	}
	
	state->mucode_control = branched ? state->mucode_decoded_buffer.next : state->mucode_decoded_buffer.next_no_branch;
	return state->mucode_control.entry_idx != MU_NONE;
}

static bool
execute_sequencer_mucode_run_ (pilot_execute_state *state)
{
	if (state->mucode_decoded_buffer.branch && !execute_sequencer_branch_test_(state))
	{
		return FALSE;
	}
	
	state->mucode_decoded_buffer = decode_mucode_entry(state->mucode_control);
	state->control = &state->mucode_decoded_buffer.operation;
	
	if (!state->mucode_decoded_buffer.branch)
	{
		state->mucode_control = state->mucode_decoded_buffer.next;
	}
	
	if (state->mucode_decoded_buffer.branch || state->mucode_decoded_buffer.next.entry_idx != MU_NONE) {
		return TRUE;
	}
	
	return FALSE;
}

static bool
execute_sequencer_interrupt_test_ (pilot_execute_state *state)
{
	if (state->decoded_inst.interrupt_cond >= COND_IRQ1 && state->decoded_inst.interrupt_cond <= COND_IRQ7)
	{
		return fetch_data_(state, (alu_src_control){DATA_REG_IRL, SIZE_8_BIT, FALSE}) >= state->decoded_inst.interrupt_cond;
	}
	
	return TRUE;
}

static void
execute_half2_advance_sequencer_ (pilot_execute_state *state)
{
	if (state->sequencer_phase == EXEC_SEQ_CORE_OP_EXECUTED)
	{
		if (state->decoded_inst.run_after.entry_idx != MU_NONE)
		{
			state->sequencer_phase = EXEC_SEQ_RUN_AFTER;
			state->mucode_control = state->decoded_inst.run_after;
		}
		else
		{
			state->sequencer_phase = EXEC_SEQ_FINAL_STEPS;
		}
	}
	
	if (state->sequencer_phase == EXEC_SEQ_REPEAT_OP_BRANCH)
	{
		if (state->used_z)
		{
			state->repeat_type.entry_idx = MU_NONE;
			state->sequencer_phase = EXEC_SEQ_FINAL_STEPS;
		}
		else if (state->repeat_type.entry_idx == MU_REPR && (fetch_data_(state, (alu_src_control){DATA_REG_F, SIZE_8_BIT, FALSE}) & F_ZERO) != 0)
		{
			state->repeat_type.entry_idx = MU_NONE;
			state->sequencer_phase = EXEC_SEQ_FINAL_STEPS;
		}
		else
		{
			state->sequencer_phase = EXEC_SEQ_WAIT_CACHED_INS;
		}
	}
	
	if (state->sequencer_phase == EXEC_SEQ_FINAL_STEPS)
	{
		if (state->decoded_inst.repeat_op.entry_idx != MU_NONE)
		{
			state->repeat_type = state->decoded_inst.repeat_op;
			state->mucode_control = state->repeat_type;
			state->sequencer_phase = EXEC_SEQ_REPEAT_OP;
			state->decoded_inst.repeat_op.entry_idx = MU_NONE;
		}
		else if (state->repeat_type.entry_idx != MU_NONE)
		{
			state->sequencer_phase = EXEC_SEQ_REPEAT_OP;
			state->mucode_control = state->repeat_type;
		}
		else if (state->branched)
		{
			state->branched = FALSE;
			state->sys->interconnects.decoded_inst_semaph = FALSE;
			state->sequencer_phase = EXEC_SEQ_WAIT_NEXT_INS;
		}
		else if (state->decoded_inst.interrupt)
		{
			state->sequencer_phase = EXEC_SEQ_SIGNAL_INTERRUPT;
		}
		else if (state->decoded_inst.disable_clk)
		{
			state->sys->core.disable_clk = TRUE;
		}
		else
		{
			state->sequencer_phase = EXEC_SEQ_WAIT_NEXT_INS;
		}
	}
	
	if (state->sequencer_phase == EXEC_SEQ_WAIT_NEXT_INS)
	{
		if (state->sys->interconnects.decoded_inst_semaph)
		{
			state->decoded_inst = *state->sys->interconnects.decoded_inst;
			state->sys->interconnects.decoded_inst_semaph = FALSE;
			
			if (state->repeat_type.entry_idx == MU_REPR)
			{
				state->repeat_type.entry_idx = MU_NONE;
			}
			else
			{
				state->sequencer_phase = EXEC_SEQ_EVAL_CONTROL;
				state->sys->core.pgc = state->decoded_inst.inst_pgc;
			}
		}
	}
	
	if (state->sequencer_phase == EXEC_SEQ_PUSH_WF)
	{
		if (!execute_sequencer_mucode_run_(state))
		{
			state->sequencer_phase = EXEC_SEQ_WAIT_NEXT_INS;
		}
	}
	
	if (state->sequencer_phase == EXEC_SEQ_PUSH_PGC)
	{
		if (!execute_sequencer_mucode_run_(state))
		{
			if (state->decoded_inst.interrupt)
			{
				state->mucode_control.entry_idx = MU_PUSH_WF_IND_SP_AUTO;
				state->sequencer_phase = EXEC_SEQ_PUSH_WF;
			}
			else
			{
				state->sequencer_phase = EXEC_SEQ_WAIT_NEXT_INS;
			}
		}
	}
	
	if (state->sequencer_phase == EXEC_SEQ_WAIT_CACHED_INS)
	{
		state->decoded_inst = *state->sys->interconnects.decoded_inst;
		state->sequencer_phase = EXEC_SEQ_EVAL_CONTROL;
	}
	
	if (state->sequencer_phase == EXEC_SEQ_EVAL_CONTROL)
	{
		if (state->decoded_inst.div_zero)
		{
			state->sequencer_phase = EXEC_SEQ_WAIT_NEXT_INS;
			
			uint32_t branch_addr = 0xffcfd0;
			write_data_(state, (alu_src_control){DATA_REG_PGC, SIZE_24_BIT, FALSE}, &branch_addr);
		}
		else if (state->decoded_inst.illegal)
		{
			state->sequencer_phase = EXEC_SEQ_WAIT_NEXT_INS;
			
			uint32_t branch_addr = 0xffcfe0;
			write_data_(state, (alu_src_control){DATA_REG_PGC, SIZE_24_BIT, FALSE}, &branch_addr);
		}
		else if (state->decoded_inst.restart)
		{
			state->mucode_control.entry_idx = MU_PUSH_PGC_IND_SP_AUTO;
			state->sequencer_phase = EXEC_SEQ_PUSH_PGC;
			
			uint32_t branch_addr = 0xffd000 | (fetch_data_(state, (alu_src_control){DATA_LATCH_IMM_0, SIZE_8_BIT, FALSE}) << 4);
			write_data_(state, (alu_src_control){DATA_REG_PGC, SIZE_24_BIT, FALSE}, &branch_addr);
		}
		else if (state->decoded_inst.run_before.entry_idx != MU_NONE)
		{
			state->sequencer_phase = EXEC_SEQ_RUN_BEFORE;
			state->mucode_control = state->decoded_inst.run_before;
		}
		else
		{
			state->sequencer_phase = EXEC_SEQ_CORE_OP;
		}
	}
	
	if (state->sequencer_phase == EXEC_SEQ_CORE_OP)
	{
		state->control = &state->decoded_inst.core_op;
		state->sequencer_phase = EXEC_SEQ_CORE_OP_EXECUTED;
	}
	
	if (state->sequencer_phase == EXEC_SEQ_RUN_BEFORE)
	{
		if (!execute_sequencer_mucode_run_(state))
		{
			state->sequencer_phase = EXEC_SEQ_CORE_OP;
		}
	}
	
	if (state->sequencer_phase == EXEC_SEQ_RUN_AFTER)
	{
		if (!execute_sequencer_mucode_run_(state))
		{
			state->sequencer_phase = EXEC_SEQ_FINAL_STEPS;
		}
	}
	
	if (state->sequencer_phase == EXEC_SEQ_REPEAT_OP)
	{
		if (!execute_sequencer_mucode_run_(state))
		{
			state->sequencer_phase = EXEC_SEQ_REPEAT_OP_BRANCH;
		}
	}
	
	if (state->sequencer_phase == EXEC_SEQ_SIGNAL_INTERRUPT)
	{
		if (execute_sequencer_interrupt_test_(state))
		{
			state->mucode_control.entry_idx = MU_PUSH_PGC_IND_SP_AUTO;
			state->sequencer_phase = EXEC_SEQ_PUSH_PGC;
		}
		else
		{
			state->sequencer_phase = EXEC_SEQ_WAIT_NEXT_INS;
		}
	}
}

void
pilot_execute_half2 (pilot_execute_state *state)
{
	if (state->sys->core.disable_clk)
	{
		return;
	}
	
	if (state->execution_phase == EXEC_START)
	{
		if (state->sys->interconnects.decoded_inst_semaph)
		{
			state->execution_phase = EXEC_HALF2_ADVANCE_SEQUENCER;
		}
	}
	
	if (state->execution_phase == EXEC_HALF2_READY)
	{
		state->execution_phase = EXEC_HALF2_RESULT_LATCH;
	}
	
	if (state->execution_phase == EXEC_HALF2_RESULT_LATCH)
	{
		execute_half2_result_latch_(state);
	}
	
	if (state->execution_phase == EXEC_HALF2_MEM_PREPARE)
	{
		execute_half2_mem_prepare_(state);
	}
	
	if (state->execution_phase == EXEC_HALF2_MEM_ASSERT)
	{
		execute_half2_mem_assert_(state);
	}
	
	if (state->execution_phase == EXEC_HALF2_ADVANCE_SEQUENCER)
	{
		execute_half2_advance_sequencer_(state);
		state->sys->interconnects.execute_memory_backoff = (state->control->mem_latch_ctl != MEM_NO_LATCH && !state->control->mem_access_suppress);
		
		state->execution_phase = EXEC_HALF1_READY;
	}
}
