/* 046267 Computer Architecture - Spring 2021 - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>

typedef struct _thread
{
	unsigned int return_cycle;
	tcontext regs;
} Thread;

typedef struct _core
{
	unsigned int num_of_threads;
	unsigned int cycle_count;
	unsigned int inst_count;
	unsigned int rr_index;
	Thread *core_threads;
} Core;

Core core_finegrained;

void CORE_init(Core *core)
{
	core->cycle_count = 0;
	core->inst_count = 0;
	core->rr_index = 0;
	core->num_of_threads = SIM_GetThreadsNum();
	core->core_threads = new Thread[core->num_of_threads];

	for (unsigned int i = 0; i < core->num_of_threads; i++)
	{
		core->core_threads[i].return_cycle = 0;
		for (unsigned int j = 0; j < REGS_COUNT; j++)
		{
			core->core_threads[i].regs.reg[j] = 0;
		}
	}
}
void CORE_BlockedMT()
{
}

void CORE_FinegrainedMT()
{

	CORE_init(&core_finegrained);
}

double CORE_BlockedMT_CPI()
{
	return 0;
}

double CORE_FinegrainedMT_CPI()
{
	delete[] core.core_threads;
	return 0;
}

void CORE_BlockedMT_CTX(tcontext *context, int threadid)
{
}

void CORE_FinegrainedMT_CTX(tcontext *context, int threadid)
{
}
