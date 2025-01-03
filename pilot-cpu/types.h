#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdint.h>

typedef int_fast8_t bool;
#define TRUE 1
#define FALSE 0

typedef uint_fast8_t rm_spec;

typedef enum
{
	SIZE_8_BIT = 0,
	SIZE_16_BIT,
	SIZE_24_BIT
} data_size_spec;

typedef enum
{
	// no-op
	MU_NONE = 0,
	
	// request memory
	MU_IND_IMM,
	MU_IND_IMM0,
	MU_IND_IMM_RM,
	MU_IND_IMM_WITH_BITS,
	
	MU_IND_REG,
	MU_IND_REG_POST_AUTO,
	MU_IND_REG_AUTO,
	MU_IND_REG_WITH_IMM,
	MU_IND_REG_WITH_BITS,
	MU_IND_PGC_WITH_IMM,
	MU_IND_PGC_WITH_IMM_RM,
	MU_IND_PGC_WITH_IMM_SHIFT,
	MU_IND_PGC_WITH_HML,
	MU_IND_PGC_WITH_HML_RM,
	
	// used for 24-bit accesses
	MU_IND_MAR_AUTO,
	MU_IND_MAR_POST_AUTO,
	
	// post-increment
	MU_POST_AUTOIDX,
	
	// repetition of an implicit instruction
	MU_REPI,
	MU_REPI_LOOP,
	MU_REPR,
	
	// used to adjust PGC for the REPR instruction (to make sure that it doesn't increment until after its completion)
	MU_ADJUST_PGC,
	
	// MULS / MULU setup steps
	MU_MUL_LD_FACTOR_A,
	MU_MUL_LD_PRODUCT_LO,
	MU_MUL_LD_PRODUCT_HI,
	MU_MULDIV_LD_REPI,
	
	// MULS / MULU loop steps
	MU_MUL_SHIFT_PRODUCT_LO_LEFT,
	MU_MUL_SHIFT_PRODUCT_HI_LEFT,
	MU_MUL_SHIFT_FACTOR_B_LEFT,
	MU_MUL_ADD_PRODUCT_LO,
	MU_MUL_ADD_PRODUCT_HI,
	
	// DIVS / DIVU setup steps
	MU_DIV_TEST_DIVIDEND_HI,
	MU_DIV_TEST_FACTOR_B,
	// (MU_MULDIV_LD_REPI goes after these two)
	
	// DIVS / DIVU loop steps (todo: sign handling?)
	MU_DIV_SHIFT_DIVIDEND_LO_LEFT,
	MU_DIV_SHIFT_DIVIDEND_HI_LEFT,
	MU_DIV_SUB_FACTOR_B,
	MU_DIV_ADD_DIVIDEND_LO_CARRY,
	MU_DIV_ST_PRODUCT_LO,
	MU_DIV_ST_PRODUCT_HI,
	
	// used for calls and exceptions/interrupts
	MU_PUSH_PGC_IND_SP_AUTO,
	MU_PUSH_PGC_WR_PGC,
	
	// used for exceptions/interrupts
	MU_PUSH_WF_IND_SP_AUTO,
	MU_PUSH_WF_WR_WF
} mucode_entry_idx;

typedef struct
{
	mucode_entry_idx entry_idx;
	// bit 3: sign extend
	// bit 4: RM operand number, or "first loop" flag for MULS
	uint8_t reg_select;
	data_size_spec size;
	bool is_write;
	bool mem_access_suppress;
} mucode_entry_spec;

typedef enum
{
	// zero, or n/a
	DATA_ZERO = 0,

	// the current micro-operation size
	DATA_SIZE,
	
	// the number of bits in the current micro-operation size (used for MULS / MULU / DIVS / DIVU initialization)
	DATA_NUM_BITS,
	
	// 8-bit registers
	DATA_REG_L0,
	DATA_REG_L1,
	DATA_REG_L2,
	DATA_REG_L3,
	DATA_REG_M0,
	DATA_REG_M1,
	DATA_REG_M2,
	DATA_REG_M3,
	DATA_REG_F,
	DATA_REG_IRL,
	
	// 16-bit registers
	DATA_REG_W0,
	DATA_REG_W1,
	DATA_REG_W2,
	DATA_REG_W3,
	DATA_REG_W4,
	DATA_REG_W5,
	DATA_REG_W6,
	DATA_REG_W7,
	DATA_REG_WF,
	
	// 24-bit registers
	DATA_REG_P0,
	DATA_REG_P1,
	DATA_REG_P2,
	DATA_REG_P3,
	DATA_REG_P4,
	DATA_REG_P5,
	DATA_REG_P6,
	DATA_REG_SP,
	DATA_REG_PGC,
	
	// Special registers
	DATA_LATCH_REPI,
	DATA_LATCH_REPR,
	DATA_LATCH_FACTOR_A,
	DATA_LATCH_FACTOR_B,
	DATA_LATCH_MEM_ADDR,
	DATA_LATCH_MEM_DATA,
	DATA_LATCH_IMM_0,
	DATA_LATCH_IMM_1,
	DATA_LATCH_IMM_2,
	DATA_LATCH_IMM_HML,
	DATA_LATCH_IMM_HML_RM,
	
	// Short Form Immediate bits of each RM operand
	DATA_LATCH_SFI_1,
	DATA_LATCH_SFI_2,
	
	// Second RM operand latches
	DATA_LATCH_RM_1,
	DATA_LATCH_RM_2,
	DATA_LATCH_RM_HML,

	// Registers from a 3-bit value
	DATA_REG_IMM_0_8,
	DATA_REG_IMM_1_8,
	DATA_REG_IMM_1_2,
	DATA_REG_IMM_2_8,
	DATA_REG_RM_1_8,
	DATA_REG_RM_1_2,
	DATA_REG_RM_2_8,
	DATA_REG_REPR,
	
	// outputs from a demultiplexer hooked up to bits 8-10 of the opcode
	DATA_DMX_IMM_BITS,
	
	// outputs from a demultiplexer hooked up to bits 8-10 of the P0 register
	DATA_DMX_P0_BITS,
	
	// L0, W0, or P0, depending on the instruction size
	DATA_REG_R0,
} data_bus_specifier;

typedef struct
{
	data_bus_specifier location;
	data_size_spec size;
	bool sign_extend;
} alu_src_control;

typedef struct
{
	alu_src_control srcs[2];
	data_bus_specifier dest;

	enum
	{
		ALU_OFF,
		ALU_ADD,
		ALU_AND,
		ALU_OR,
		ALU_XOR
	} operation;
	
	// Source transformations (in order, performed before sign extension)
	bool src2_add1;
	bool src2_add_carry;
	bool src2_negate;
	bool src2_and_with_overflow;
	
	// Shifter control
	enum
	{
		SHIFTER_NONE,
		SHIFTER_LEFT,
		SHIFTER_LEFT_CARRY,
		SHIFTER_LEFT_BARREL,
		SHIFTER_RIGHT_LOGICAL,
		SHIFTER_RIGHT_ARITH,
		SHIFTER_RIGHT_CARRY,
		SHIFTER_RIGHT_BARREL,
		SHIFTER_SWAP
	} shifter_mode;
	
	// this uses the temp_z latch instead of the X flag
	bool temp_z_as_extend;
	
	// Flag control
	uint8_t flag_write_mask;
	bool invert_carries;
	
	enum
	{
		FLAG_Z_NORMAL,
		// this is for checking if multi-step calculations result in zero
		FLAG_Z_ACCUM,
		// this ANDs src2 with src1 and sets the Z flag accordingly
		FLAG_Z_BIT_TEST,
		// this uses the temp_z latch instead of the Z flag
		FLAG_Z_SAVE,
	} flag_z_mode;
	
	enum
	{
		FLAG_V_NORMAL,
		FLAG_V_SHIFTER_CARRY,
		FLAG_V_CLEAR,
	} flag_v_mode;
	
	// Memory control
	/* Latching an address follows the cycle below:
	 * 1. If memory unit is not ready, execution is blocked with address to be latched being held
	 * 2. Address is latched, data is read or written, takes 1+ cycles
	 * 3. While data is being read or written, 
	 */
	enum
	{
		// Don't latch address
		MEM_NO_LATCH = 0,
		// Latches at first half of cycle, address from ALU src1
		MEM_LATCH_HALF1,
		// Latches at second half of cycle, address from ALU dest
		MEM_LATCH_HALF2,
		// Latches at second half of cycle, address is whatever was left in MAR (used for destination writeback)
		MEM_LATCH_HALF2_MAR,
	} mem_latch_ctl;
	
	// If set, suppresses memory access assertion if memory is latched in this cycle
	bool mem_access_suppress;
	enum
	{
		MEM_READ = 0,
		// Data is latched from ALU src2
		MEM_WRITE_FROM_SRC2,
		// Data is latched from ALU dest
		MEM_WRITE_FROM_DEST,
		// Data is not latched; whatever was left in the lower 16 bits of MDR is what's written back
		MEM_WRITE_FROM_MDR,
		// Data is not latched; whatever was left in the upper 8 bits of MDR is what's written back
		// (this destroys the lower 16 bits of MDR but that shouldn't matter)
		MEM_WRITE_FROM_MDR_HIGH,
	} mem_write_ctl;
	
	// The Pilot has a 24-bit internal data bus, but this is reduced by glue logic to 16 bits for any accesses outside the CPU.
	bool is_16bit;
} execute_control_word;

typedef struct
{
	execute_control_word operation;
	mucode_entry_spec next;
} mucode_entry;

typedef struct
{
	// Immediate data sources
	uint16_t imm_words[5];
	
	// PGC for this instruction
	uint32_t inst_pgc;
	
	// Sequencer control
	// run_before: if not MU_NONE, is executed before core_op - usually for memory reads
	mucode_entry_spec run_before;
	// core_op: single-cycle execution control word assembled by the decode stage
	execute_control_word core_op;
	// run_after: if not MU_NONE, is executed after core_op - usually for memory writes
	mucode_entry_spec run_after;
	// repeat_op: if not MU_NONE, is executed after entire instruction - for the repeat instructions
	mucode_entry_spec repeat_op;
	
	// Branch flags
	bool branch;
	bool interrupt;
	
	enum
	{
		COND_LE = 0,		// less than or equal
		COND_GT,		// greater than
		COND_LT,		// less than
		COND_GE,		// greater than or equal
		COND_U_LE,		// unsigned less than or equal
		COND_U_GT,		// unsigned greater than
		COND_C,			// carry set; unsigned less than
		COND_NC,		// carry clear; unsigned greater than or equal
		COND_M,			// minus; sign set
		COND_P,			// plus; sign clear
		COND_OV,		// overflow; parity even
		COND_NOV,		// not overflow; parity odd
		COND_Z,			// equal; zero
		COND_NZ,		// not equal; nonzero
		COND_ALWAYS,		// always
		COND_ALWAYS_CALL,	// always, but used for calls
		COND_DJNZ,		// nonzero, but uses the temp_z latch
	} branch_cond;
	
	enum
	{
		COND_NMI = 0,		// NMI
		COND_IRQ1,		// interrupt request level >= IRQ number
		COND_IRQ2,
		COND_IRQ3,
		COND_IRQ4,
		COND_IRQ5,
		COND_IRQ6,
		COND_IRQ7
	} interrupt_cond;
	
	enum
	{
		BR_MAR,			// JR.S / CR.S / JP rm24 / JEA / CALL rm24 / CEA / DJNZ (address is resolved and in MAR)
		BR_HML,			// JP hml / JR.L / CALL hml / CR.L (deferred resolution, since the specific type depends on bit 0 of HML)
		BR_RESTART,		// RST
		BR_DIV_ZERO,		// Divide by Zero Exception
		BR_ILLEGAL		// Illegal Instruction Exception
	} branch_dest_type;
	
	// Offset of the second RM operand
	uint8_t rm2_offset;
	
	// Disable the clock after this instruction
	bool disable_clk;
} inst_decoded_flags;

#endif
