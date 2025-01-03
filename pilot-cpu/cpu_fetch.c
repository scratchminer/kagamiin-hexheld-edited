#include "cpu_fetch.h"
#include "memory.h"

void
pilot_fetch_half1 (pilot_fetch_state *state)
{
	if (state->sys->core.disable_clk)
	{
		return;
	}
	
	if (state->fetch_phase == FETCH_HALF1_READY)
	{
		for (int i = 4; i > 0; i--)
		{
			if (!state->queue_words_full[i])
			{
				state->queue_words[i] = state->queue_words[i - 1];
				state->queue_words_full[i] = state->queue_words_full[i - 1];
				state->queue_words_full[i - 1] = FALSE;
			}
		}
		
		state->fetch_phase = FETCH_HALF1_MEM_WAIT;
	}
	
	if (state->fetch_phase == FETCH_HALF1_MEM_WAIT)
	{
		if (state->mem_access_waiting)
		{
			if (Pilot_mem_data_wait(state->sys))
			{
				state->queue_words[0] = Pilot_mem_get_data(state->sys);
				state->queue_words_full[0] = TRUE;
			}
			else
			{
				return;
			}
			
			state->mem_access_waiting = FALSE;
		}
		
		state->fetch_phase = FETCH_HALF1_DEQUEUE;
	}
	
	if (state->fetch_phase == FETCH_HALF1_DEQUEUE)
	{
		bool *fetch_word_semaph = &state->sys->interconnects.fetch_word_semaph;
		uint16_t *fetch_word = &state->sys->interconnects.fetch_word;
		
		if (!(*fetch_word_semaph) && state->queue_words_full[4])
		{
			*fetch_word = state->queue_words[4];
			state->queue_words_full[4] = FALSE;
			
			state->sys->interconnects.fetch_addr = (state->sys->interconnects.fetch_addr + 2) & 0xfffffe;
			*fetch_word_semaph = TRUE;
		}
		
		state->fetch_phase = FETCH_HALF2_READY;
	}
}

void
pilot_fetch_half2 (pilot_fetch_state *state)
{
	if (state->sys->core.disable_clk)
	{
		return;
	}
	
	if (state->fetch_phase == FETCH_HALF2_READY)
	{
		state->fetch_phase = FETCH_HALF2_BRANCH;
	}
	
	if (state->fetch_phase == FETCH_HALF2_BRANCH)
	{
		bool *execute_branch = &state->sys->interconnects.execute_branch;
		
		if (*execute_branch)
		{
			state->sys->interconnects.fetch_word_semaph = FALSE;
			for (int i = 0; i < 5; i++)
			{
				state->queue_words_full[i] = FALSE;
			}
			
			state->mem_addr = state->sys->interconnects.execute_branch_addr;
			state->sys->interconnects.fetch_addr = state->sys->interconnects.execute_branch_addr;
			*execute_branch = FALSE;
		}
		
		state->fetch_phase = FETCH_HALF2_MEM_ASSERT;
	}
	
	if (state->fetch_phase == FETCH_HALF2_MEM_ASSERT)
	{
		bool all_full = TRUE;
		
		for (int i = 0; i < 5; i++) {
			if (!state->queue_words_full[i])
			{
				all_full = FALSE;
			}
		}
		if (all_full)
		{
			state->fetch_phase = FETCH_HALF1_READY;
			return;
		}
		
		if (state->sys->interconnects.execute_memory_backoff)
		{
			return;
		}
		
		if (!Pilot_mem_addr_read_assert(state->sys, TRUE, state->mem_addr))
		{
			return;
		}
		
		state->mem_access_waiting = TRUE;
		state->mem_addr = (state->mem_addr + 2) & 0xfffffe;
		
		state->fetch_phase = FETCH_HALF1_READY;
	}
}
