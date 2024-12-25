#include "cpu_regs.h"
#include "cpu_decode.h"

static inline mucode_entry
base_entry_ (mucode_entry_spec spec)
{
	mucode_entry prg;
	
	if (spec.size == SIZE_24_BIT && !spec.mem_access_suppress)
	{
		prg.next = (mucode_entry_spec)
		{
			MU_IND_MAR_AUTO,
			spec.reg_select,
			SIZE_8_BIT,
			spec.is_write,
			spec.mem_access_suppress
		};
	}
	else
	{
		prg.next = (mucode_entry_spec)
		{
			MU_NONE,
			spec.reg_select,
			spec.size,
			spec.is_write,
			spec.mem_access_suppress
		};
	}
	
	prg.operation.srcs[0].location = DATA_ZERO;
	prg.operation.srcs[0].size = SIZE_24_BIT;
	prg.operation.srcs[0].sign_extend = FALSE;

	prg.operation.srcs[1].location = DATA_ZERO;
	prg.operation.srcs[1].size = spec.size;
	prg.operation.srcs[1].sign_extend = FALSE;
	
	prg.operation.flag_write_mask = 0;
	prg.operation.invert_carries = FALSE;
	
	prg.operation.src2_add1 = FALSE;
	prg.operation.src2_add_carry = FALSE;
	prg.operation.src2_negate = FALSE;
	prg.operation.src2_and_with_overflow = FALSE;
	
	prg.operation.temp_z_as_extend = FALSE;
	
	prg.operation.flag_z_mode = FLAG_Z_NORMAL;
	prg.operation.flag_v_mode = FLAG_V_NORMAL;
	
	prg.operation.operation = ALU_OFF;
	prg.operation.shifter_mode = SHIFTER_NONE;
	prg.operation.dest = DATA_ZERO;
	
	prg.operation.mem_latch_ctl = MEM_NO_LATCH;
	prg.operation.is_16bit = (spec.size >= SIZE_16_BIT);
	prg.operation.mem_access_suppress = spec.mem_access_suppress;
	
	return prg;
}

static mucode_entry
ind_1cyc_imm_ (mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	prg.operation.srcs[0].location = DATA_LATCH_IMM_1;
	prg.operation.srcs[0].sign_extend = spec.reg_select & 0x8;
	prg.operation.srcs[0].size = spec.size;
	prg.operation.srcs[1].size = !(spec.reg_select & 0x8) ? SIZE_24_BIT : SIZE_16_BIT;
	
	prg.operation.mem_latch_ctl = MEM_LATCH_HALF1;
	prg.operation.mem_write_ctl = !(spec.is_write) ? MEM_READ : MEM_WRITE_FROM_MDR;
	
	return prg;
}

static mucode_entry
ind_1cyc_imm_rm_ (mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	prg.operation.srcs[0].location = DATA_LATCH_RM_1;
	prg.operation.srcs[0].sign_extend = spec.reg_select & 0x8;
	prg.operation.srcs[0].size = spec.size;
	prg.operation.srcs[1].size = !(spec.reg_select & 0x8) ? SIZE_24_BIT : SIZE_16_BIT;

	prg.operation.mem_latch_ctl = MEM_LATCH_HALF1;
	prg.operation.mem_write_ctl = !(spec.is_write) ? MEM_READ : MEM_WRITE_FROM_MDR;
	
	return prg;
}

static mucode_entry
ind_1cyc_imm0_ (mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	prg.operation.srcs[0].location = DATA_LATCH_IMM_0;
	prg.operation.srcs[0].size = SIZE_8_BIT;
	
	prg.operation.mem_latch_ctl = MEM_LATCH_HALF1;
	prg.operation.mem_write_ctl = !(spec.is_write) ? MEM_READ : MEM_WRITE_FROM_MDR;
	
	return prg;
}

static mucode_entry
ind_1cyc_reg_ (mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	prg.operation.srcs[0].size = SIZE_24_BIT;
	prg.operation.srcs[0].location = DATA_REG_P0 + (spec.reg_select & 0x7);
	
	prg.operation.mem_latch_ctl = MEM_LATCH_HALF1;
	prg.operation.mem_write_ctl = !(spec.is_write) ? MEM_READ : MEM_WRITE_FROM_MDR;
	
	return prg;
}

static mucode_entry
ind_1cyc_reg_post_auto_ (mucode_entry_spec spec)
{
	mucode_entry prg = ind_1cyc_reg_(spec);
	prg.next.entry_idx = (spec.size != SIZE_24_BIT) ? MU_POST_AUTOIDX : MU_IND_MAR_POST_AUTO;
	
	return prg;
}

static mucode_entry
ind_1cyc_reg_auto_ (mucode_entry_spec spec)
{
	mucode_entry prg = ind_1cyc_reg_(spec);
	
	prg.operation.srcs[1].location = DATA_SIZE;
	prg.operation.src2_negate = TRUE;
	
	prg.operation.operation = ALU_ADD;
	
	prg.operation.dest = prg.operation.srcs[0].location;
	
	prg.operation.mem_latch_ctl = MEM_LATCH_HALF2;
	
	return prg;
}

static mucode_entry
ind_2cyc_withimm_ (mucode_entry_spec spec)
{
	mucode_entry prg = ind_1cyc_reg_(spec);
	prg.operation.srcs[1].location = (!(spec.reg_select & 0x10)) ? DATA_LATCH_IMM_1 : DATA_LATCH_RM_1;
	prg.operation.srcs[1].size = SIZE_16_BIT;
	prg.operation.srcs[1].sign_extend = TRUE;
	
	prg.operation.operation = ALU_ADD;
	
	prg.operation.dest = DATA_ZERO;
	
	return prg;
}

static mucode_entry
ind_2cyc_imm_withbits_ (mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	prg.operation.srcs[0].location = (!(spec.reg_select & 0x10)) ? DATA_LATCH_IMM_HML_RM : DATA_LATCH_RM_HML;
	
	prg.operation.srcs[1].location = (!(spec.reg_select & 0x10)) ? DATA_REG_IMM_2_8 : DATA_REG_RM_2_8;
	prg.operation.srcs[1].sign_extend = spec.reg_select & 0x8;
	
	prg.operation.operation = ALU_ADD;
	
	prg.operation.dest = DATA_LATCH_MEM_ADDR;
	
	prg.operation.mem_latch_ctl = MEM_LATCH_HALF2;
	
	return prg;
}

static mucode_entry
ind_2cyc_reg_withbits_ (mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	prg.operation.srcs[0].location = (!(spec.reg_select & 0x10)) ? DATA_REG_IMM_1_2 : DATA_REG_RM_1_2;
	
	prg.operation.srcs[1].location = (!(spec.reg_select & 0x10)) ? DATA_REG_IMM_1_8 : DATA_REG_RM_1_8;
	prg.operation.srcs[1].sign_extend = spec.reg_select & 0x8;
	
	prg.operation.operation = ALU_ADD;
	
	prg.operation.dest = DATA_LATCH_MEM_ADDR;
	
	prg.operation.mem_latch_ctl = MEM_LATCH_HALF2;
	
	return prg;
}

static mucode_entry
ind_2cyc_pgc_withimm_ (mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	prg.operation.srcs[0].location = DATA_REG_PGC;
	
	prg.operation.srcs[1].location = DATA_LATCH_IMM_0;
	prg.operation.srcs[1].size = SIZE_8_BIT;
	prg.operation.srcs[1].sign_extend = TRUE;
	
	prg.operation.operation = ALU_ADD;
	
	prg.operation.dest = DATA_LATCH_MEM_ADDR;
	
	prg.operation.mem_latch_ctl = MEM_LATCH_HALF2;
	
	return prg;
}

static mucode_entry
ind_2cyc_pgc_withimm_shift_ (mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	prg.operation.srcs[0].location = DATA_REG_PGC;
	
	prg.operation.srcs[1].location = DATA_LATCH_IMM_0;
	prg.operation.srcs[1].size = SIZE_8_BIT;
	prg.operation.srcs[1].sign_extend = TRUE;
	
	prg.operation.operation = ALU_ADD;
	prg.operation.shifter_mode = SHIFTER_LEFT;
	
	prg.operation.dest = DATA_ZERO;
	
	prg.operation.mem_latch_ctl = MEM_LATCH_HALF2;
	prg.operation.mem_access_suppress = TRUE;
	
	return prg;
}

static mucode_entry
ind_2cyc_pgc_withimm_rm_ (mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	prg.operation.srcs[0].location = DATA_REG_PGC;
	
	prg.operation.srcs[1].location = (!(spec.reg_select & 0x10)) ? DATA_LATCH_IMM_1 : DATA_LATCH_RM_1;
	prg.operation.srcs[1].size = spec.size;
	prg.operation.srcs[1].sign_extend = spec.reg_select & 0x8;
	
	prg.operation.operation = ALU_ADD;
	
	prg.operation.dest = DATA_LATCH_MEM_ADDR;
	
	prg.operation.mem_latch_ctl = MEM_LATCH_HALF2;
	
	return prg;
}

static mucode_entry
ind_2cyc_pgc_withhml_ (mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	prg.operation.srcs[0].location = DATA_REG_PGC;
	
	prg.operation.srcs[1].location = DATA_LATCH_IMM_HML;
	prg.operation.srcs[1].size = SIZE_24_BIT;
	prg.operation.srcs[1].sign_extend = FALSE;
	
	prg.operation.operation = ALU_ADD;
	
	prg.operation.dest = DATA_LATCH_MEM_ADDR;
	
	prg.operation.mem_latch_ctl = MEM_LATCH_HALF2;
	
	return prg;
}

static mucode_entry
ind_2cyc_pgc_withhml_rm_ (mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	prg.operation.srcs[0].location = DATA_REG_PGC;
	
	prg.operation.srcs[1].location = (!(spec.reg_select & 0x10)) ? DATA_LATCH_IMM_HML_RM : DATA_LATCH_RM_HML;
	prg.operation.srcs[1].size = SIZE_24_BIT;
	prg.operation.srcs[1].sign_extend = FALSE;
	
	prg.operation.operation = ALU_ADD;
	
	prg.operation.dest = DATA_LATCH_MEM_ADDR;
	
	prg.operation.mem_latch_ctl = MEM_LATCH_HALF2;
	
	return prg;
}

static mucode_entry
ind_2cyc_mar_auto_ (mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	prg.operation.srcs[0].location = DATA_LATCH_MEM_ADDR;
	
	prg.operation.srcs[1].location = DATA_SIZE;
	prg.operation.srcs[1].size = SIZE_16_BIT;
	prg.operation.srcs[1].sign_extend = FALSE;
	
	prg.operation.operation = ALU_ADD;
	
	prg.operation.dest = DATA_ZERO;
	
	prg.operation.mem_latch_ctl = MEM_LATCH_HALF2;
	prg.operation.mem_write_ctl = !(spec.is_write) ? MEM_READ : MEM_WRITE_FROM_MDR_HIGH;
	
	return prg;
}

static mucode_entry
ind_2cyc_mar_post_auto_ (mucode_entry_spec spec)
{
	mucode_entry prg = ind_2cyc_mar_auto_(spec);
	prg.operation.mem_latch_ctl = MEM_LATCH_HALF2;
	
	prg.next.entry_idx = MU_POST_AUTOIDX;
	prg.next.size = SIZE_24_BIT;
	
	return prg;
}

static mucode_entry
after_autoidx_ (mucode_entry_spec spec)
{
	mucode_entry prg = ind_1cyc_reg_(spec);
	prg.operation.srcs[1].location = DATA_SIZE;
	
	prg.operation.src2_add1 = FALSE;
	prg.operation.src2_negate = FALSE;
	
	prg.operation.operation = ALU_ADD;
	
	prg.operation.dest = prg.operation.srcs[0].location;
	prg.operation.mem_latch_ctl = MEM_NO_LATCH;
	
	prg.next.entry_idx = MU_NONE;
	
	return prg;
}

static mucode_entry
repi_ (mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	prg.operation.srcs[0].location = DATA_LATCH_REPI;
	prg.operation.srcs[0].size = SIZE_8_BIT;
	
	prg.operation.srcs[1].location = DATA_ZERO;
	prg.operation.srcs[1].size = SIZE_8_BIT;
	prg.operation.srcs[1].sign_extend = TRUE;
	
	prg.operation.operation = ALU_ADD;
	prg.operation.src2_add1 = TRUE;
	prg.operation.src2_negate = TRUE;
	
	prg.operation.flag_z_mode = FLAG_Z_SAVE;
	
	prg.operation.dest = DATA_LATCH_REPI;
	prg.operation.mem_latch_ctl = MEM_NO_LATCH;
	
	prg.next.entry_idx = MU_NONE;
	
	return prg;
}

static mucode_entry
repr_ (mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	prg.operation.srcs[0].location = DATA_REG_REPR;
	prg.operation.srcs[0].size = SIZE_24_BIT;
	
	prg.operation.srcs[1].location = DATA_ZERO;
	prg.operation.srcs[1].size = SIZE_24_BIT;
	prg.operation.srcs[1].sign_extend = TRUE;
	
	prg.operation.operation = ALU_ADD;
	prg.operation.src2_add1 = TRUE;
	prg.operation.src2_negate = TRUE;
	
	prg.operation.dest = DATA_REG_REPR;
	prg.operation.mem_latch_ctl = MEM_NO_LATCH;
	
	prg.next.entry_idx = MU_NONE;
	
	return prg;
}

static mucode_entry
adjust_pgc_ (mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	prg.operation.srcs[0].location = DATA_REG_PGC;
	prg.operation.srcs[0].size = SIZE_24_BIT;
	
	prg.operation.srcs[1].location = DATA_SIZE;
	prg.operation.srcs[1].size = SIZE_16_BIT;
	prg.operation.srcs[1].sign_extend = TRUE;
	
	prg.operation.operation = ALU_ADD;
	prg.operation.src2_negate = TRUE;
	
	prg.operation.dest = DATA_REG_PGC;
	prg.operation.mem_latch_ctl = MEM_NO_LATCH;
	
	prg.next.entry_idx = MU_NONE;
	
	return prg;
}

static mucode_entry
mul_1cyc_ld_factor_a_(mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	
	prg.operation.srcs[0].size = spec.size;
	prg.operation.srcs[0].location = DATA_REG_IMM_0_8;
	prg.operation.srcs[1].location = DATA_ZERO;
	prg.operation.operation = ALU_OR;
	
	prg.operation.dest = DATA_LATCH_FACTOR_A;
	prg.operation.mem_latch_ctl = MEM_NO_LATCH;
	
	prg.next.entry_idx = MU_MUL_LD_PRODUCT_LO;
	prg.next.size = spec.size;
	
	return prg;
}

static mucode_entry
mul_2cyc_ld_product_lo_(mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	
	prg.operation.srcs[0].size = spec.size;
	prg.operation.srcs[0].location = DATA_ZERO;
	prg.operation.srcs[1].location = DATA_ZERO;
	prg.operation.operation = ALU_OR;
	
	prg.operation.dest = DATA_REG_IMM_0_8;
	prg.operation.mem_latch_ctl = MEM_NO_LATCH;
	
	prg.next.entry_idx = MU_MUL_LD_PRODUCT_HI;
	prg.next.size = spec.size;
	
	return prg;
}

static mucode_entry
mul_3cyc_ld_product_hi_(mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	
	prg.operation.srcs[0].size = spec.size;
	prg.operation.srcs[0].location = DATA_ZERO;
	prg.operation.srcs[1].location = DATA_ZERO;
	prg.operation.operation = ALU_OR;
	
	prg.operation.dest = DATA_REG_R0;
	prg.operation.mem_latch_ctl = MEM_NO_LATCH;
	
	prg.next.entry_idx = MU_MULDIV_LD_REPI;
	prg.next.size = spec.size;
	
	return prg;
}

static mucode_entry
muldiv_34cyc_ld_repi_(mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	
	prg.operation.srcs[0].size = SIZE_8_BIT;
	prg.operation.srcs[0].location = DATA_NUM_BITS;
	prg.operation.srcs[1].size = spec.size;
	prg.operation.srcs[1].location = DATA_ZERO;
	prg.operation.operation = ALU_OR;
	
	prg.operation.dest = DATA_LATCH_REPI;
	prg.operation.mem_latch_ctl = MEM_NO_LATCH;
	
	prg.next.entry_idx = MU_NONE;
	
	return prg;
}

static mucode_entry
mul_ncyc_shift_product_lo_left_ (mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	
	prg.operation.srcs[0].size = spec.size;
	prg.operation.srcs[0].location = DATA_ZERO;
	prg.operation.srcs[1].location = DATA_REG_IMM_0_8;
	prg.operation.operation = ALU_OR;
	
	prg.operation.dest = DATA_REG_IMM_0_8;
	prg.operation.mem_latch_ctl = MEM_NO_LATCH;
	
	prg.operation.shifter_mode = SHIFTER_LEFT;
	prg.operation.temp_z_as_extend = TRUE;
	
	prg.next.entry_idx = MU_MUL_SHIFT_PRODUCT_HI_LEFT;
	prg.next.size = spec.size;
	
	return prg;
}

static mucode_entry
mul_n1cyc_shift_product_hi_left_ (mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	
	prg.operation.srcs[0].size = spec.size;
	prg.operation.srcs[0].location = DATA_ZERO;
	prg.operation.srcs[1].location = DATA_REG_R0;
	prg.operation.operation = ALU_OR;
	
	prg.operation.dest = DATA_REG_R0;
	prg.operation.mem_latch_ctl = MEM_NO_LATCH;
	
	prg.operation.shifter_mode = SHIFTER_LEFT_CARRY;
	prg.operation.temp_z_as_extend = TRUE;
	
	prg.next.entry_idx = MU_MUL_SHIFT_FACTOR_B_LEFT;
	prg.next.size = spec.size;
	
	return prg;
}

static mucode_entry
mul_n2cyc_shift_factor_b_left_ (mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	
	prg.operation.srcs[0].size = spec.size;
	prg.operation.srcs[0].location = DATA_ZERO;
	prg.operation.srcs[1].location = DATA_LATCH_FACTOR_B;
	prg.operation.operation = ALU_OR;
	
	prg.operation.dest = DATA_LATCH_FACTOR_B;
	prg.operation.mem_latch_ctl = MEM_NO_LATCH;
	
	prg.operation.shifter_mode = SHIFTER_LEFT_CARRY;
	prg.operation.flag_v_mode = FLAG_V_SHIFTER_CARRY;
	prg.operation.temp_z_as_extend = TRUE;
	
	prg.operation.flag_write_mask = F_OVERFLOW;
	
	prg.next.entry_idx = MU_MUL_ADD_PRODUCT_LO;
	prg.next.size = spec.size;
	
	return prg;
}

static mucode_entry
mul_n3cyc_add_product_lo_ (mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	
	prg.operation.srcs[0].size = spec.size;
	prg.operation.srcs[0].location = DATA_REG_IMM_0_8;
	prg.operation.srcs[1].location = DATA_LATCH_FACTOR_A;
	prg.operation.srcs[1].sign_extend = (spec.reg_select & 0x8) != 0;
	prg.operation.operation = ALU_ADD;
	
	prg.operation.dest = DATA_REG_IMM_0_8;
	prg.operation.mem_latch_ctl = MEM_NO_LATCH;
	
	prg.operation.shifter_mode = SHIFTER_NONE;
	prg.operation.src2_negate = (spec.reg_select & 0x10) != 0;
	prg.operation.src2_and_with_overflow = TRUE;
	prg.operation.temp_z_as_extend = TRUE;
	prg.operation.invert_carries = (spec.reg_select & 0x10) != 0;
	
	prg.operation.flag_write_mask = F_ZERO;
	
	prg.next.entry_idx = MU_MUL_ADD_PRODUCT_HI;
	prg.next.size = spec.size;
	
	return prg;
}

static mucode_entry
mul_n4cyc_add_product_hi_ (mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	
	prg.operation.srcs[0].size = spec.size;
	prg.operation.srcs[0].location = DATA_REG_R0;
	prg.operation.srcs[1].location = DATA_ZERO;
	prg.operation.srcs[1].sign_extend = (spec.reg_select & 0x8) != 0;
	prg.operation.operation = ALU_ADD;
	
	prg.operation.dest = DATA_REG_R0;
	prg.operation.mem_latch_ctl = MEM_NO_LATCH;
	
	prg.operation.shifter_mode = SHIFTER_NONE;
	prg.operation.src2_add_carry = TRUE;
	prg.operation.src2_and_with_overflow = TRUE;
	prg.operation.src2_negate = (spec.reg_select & 0x10) != 0;
	prg.operation.temp_z_as_extend = TRUE;
	prg.operation.flag_z_mode = FLAG_Z_ACCUM;
	prg.operation.flag_v_mode = FLAG_V_CLEAR;
	
	prg.operation.flag_write_mask = F_OVERFLOW | F_SIGN | F_CARRY | F_ZERO;
	
	prg.next.entry_idx = MU_REPI_LOOP;
	
	return prg;
}

// todo: DIV microcode

static mucode_entry
push_pgc_1cyc_ind_sp_auto_(mucode_entry_spec spec)
{
	mucode_entry prg = ind_1cyc_reg_auto_(spec);
	
	prg.operation.srcs[0].location = DATA_REG_SP;
	prg.operation.srcs[0].size = SIZE_24_BIT;
	
	prg.operation.mem_access_suppress = TRUE;
	
	prg.next.entry_idx = MU_PUSH_PGC_WR_PGC;
	prg.next.size = SIZE_24_BIT;
	prg.next.mem_access_suppress = FALSE;
	
	return prg;
}

static mucode_entry
push_pgc_2cyc_wr_pgc_(mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	
	prg.operation.srcs[0].size = SIZE_24_BIT;
	prg.operation.srcs[0].location = DATA_REG_PGC;
	prg.operation.srcs[1].size = SIZE_24_BIT;
	prg.operation.srcs[1].location = DATA_ZERO;
	prg.operation.operation = ALU_OR;
	
	prg.operation.mem_latch_ctl = MEM_LATCH_HALF2_MAR;
	prg.operation.mem_write_ctl = MEM_WRITE_FROM_DEST;
	
	return prg;
}

static mucode_entry
push_wf_1cyc_ind_sp_auto_(mucode_entry_spec spec)
{
	mucode_entry prg = ind_1cyc_reg_auto_(spec);
	
	prg.operation.srcs[0].location = DATA_REG_SP;
	prg.operation.srcs[0].size = SIZE_24_BIT;
	
	prg.operation.mem_access_suppress = TRUE;
	
	prg.next.entry_idx = MU_PUSH_WF_WR_WF;
	prg.next.size = SIZE_16_BIT;
	prg.next.mem_access_suppress = FALSE;
	
	return prg;
}

static mucode_entry
push_wf_2cyc_wr_wf_(mucode_entry_spec spec)
{
	mucode_entry prg = base_entry_(spec);
	
	prg.operation.srcs[0].size = SIZE_16_BIT;
	prg.operation.srcs[0].location = DATA_REG_WF;
	prg.operation.srcs[1].size = SIZE_16_BIT;
	prg.operation.srcs[1].location = DATA_ZERO;
	prg.operation.operation = ALU_OR;
	
	prg.operation.mem_latch_ctl = MEM_LATCH_HALF2_MAR;
	prg.operation.mem_write_ctl = MEM_WRITE_FROM_DEST;
	
	return prg;
}

mucode_entry
decode_mucode_entry (mucode_entry_spec spec)
{
	mucode_entry result;
	switch (spec.entry_idx)
	{
		case MU_IND_IMM:
			return result = ind_1cyc_imm_(spec);
		case MU_IND_IMM_RM:
			return result = ind_1cyc_imm_rm_(spec);
		case MU_IND_REG:
			return result = ind_1cyc_reg_(spec);
		case MU_IND_REG_POST_AUTO:
			return result = ind_1cyc_reg_post_auto_(spec);
		case MU_IND_REG_AUTO:
			return result = ind_1cyc_reg_auto_(spec);
		case MU_IND_REG_WITH_IMM:
			return result = ind_2cyc_withimm_(spec);
		case MU_IND_IMM0:
			return result = ind_1cyc_imm0_(spec);
		case MU_IND_IMM_WITH_BITS:
			return result = ind_2cyc_imm_withbits_(spec);
		case MU_IND_REG_WITH_BITS:
			return result = ind_2cyc_reg_withbits_(spec);
		case MU_IND_PGC_WITH_IMM:
			return result = ind_2cyc_pgc_withimm_(spec);
		case MU_IND_PGC_WITH_IMM_RM:
			return result = ind_2cyc_pgc_withimm_rm_(spec);
		case MU_IND_PGC_WITH_IMM_SHIFT:
			return result = ind_2cyc_pgc_withimm_shift_(spec);
		case MU_IND_PGC_WITH_HML:
			return result = ind_2cyc_pgc_withhml_(spec);
		case MU_IND_PGC_WITH_HML_RM:
			return result = ind_2cyc_pgc_withhml_rm_(spec);
		case MU_IND_MAR_AUTO:
			return result = ind_2cyc_mar_auto_(spec);
		case MU_IND_MAR_POST_AUTO:
			return result = ind_2cyc_mar_post_auto_(spec);
		case MU_POST_AUTOIDX:
			return result = after_autoidx_(spec);
		case MU_REPI:
		case MU_REPI_LOOP:
			return result = repi_(spec);
		case MU_REPR:
			return result = repr_(spec);
		case MU_ADJUST_PGC:
			return result = adjust_pgc_(spec);
		case MU_MUL_LD_FACTOR_A:
			return result = mul_1cyc_ld_factor_a_(spec);
		case MU_MUL_LD_PRODUCT_LO:
			return result = mul_2cyc_ld_product_lo_(spec);
		case MU_MUL_LD_PRODUCT_HI:
			return result = mul_3cyc_ld_product_hi_(spec);
		case MU_MULDIV_LD_REPI:
			return result = muldiv_34cyc_ld_repi_(spec);
		case MU_MUL_SHIFT_PRODUCT_LO_LEFT:
			return result = mul_ncyc_shift_product_lo_left_(spec);
		case MU_MUL_SHIFT_PRODUCT_HI_LEFT:
			return result = mul_n1cyc_shift_product_hi_left_(spec);
		case MU_MUL_SHIFT_FACTOR_B_LEFT:
			return result = mul_n2cyc_shift_factor_b_left_(spec);
		case MU_MUL_ADD_PRODUCT_LO:
			return result = mul_n3cyc_add_product_lo_(spec);
		case MU_MUL_ADD_PRODUCT_HI:
			return result = mul_n4cyc_add_product_hi_(spec);
		
		// DIVU / DIVS microcode would be here
		
		case MU_PUSH_PGC_IND_SP_AUTO:
			return result = push_pgc_1cyc_ind_sp_auto_(spec);
		case MU_PUSH_PGC_WR_PGC:
			return result = push_pgc_2cyc_wr_pgc_(spec);
		case MU_PUSH_WF_IND_SP_AUTO:
			return result = push_wf_1cyc_ind_sp_auto_(spec);
		case MU_PUSH_WF_WR_WF:
			return result = push_wf_2cyc_wr_wf_(spec);
		default:
			decode_unreachable_();
			return base_entry_(spec);
	}
}
