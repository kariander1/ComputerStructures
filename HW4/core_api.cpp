/* 046267 Computer Architecture - Spring 2021 - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>


// Thread struct for containing thread properties
typedef struct _thread
{
	unsigned int rip;				// Instruction line #
	unsigned int return_cycle;		// The cycle in which the thread is operational again
	tcontext regs;					// The register file set for this thread
} Thread;

// Core struct for containing core properties
typedef struct _core
{
	unsigned int num_of_threads;	// Number of threads in SIM
	unsigned int cycle_count;		// The cycle count of the SIM
	unsigned int inst_count;		// Instructions retired in the SIM
	unsigned int rr_index;			// Current Round-Robin index for selecting thread
	Thread *core_threads;			// Threads array
} Core;

// Declaration for next usage
void EXECUTE_inst(unsigned int &idle_count, unsigned int &halt_count, Thread &current_thread, Core &core);

Core core_finegrained;				// Core instance for Fine-Grained multithreading
Core core_blocked;					// Core instance for Blocked multithreading

// CORE_init - Helper function for initializing core parameters and properties
void CORE_init(Core *core)
{
	// Zero counts
	core->cycle_count = 0;
	core->inst_count = 0;
	core->rr_index = 0;

	// Get num of threads
	core->num_of_threads = SIM_GetThreadsNum();

	// Allocate thread array
	core->core_threads = new Thread[core->num_of_threads];

	// Initialize every thread
	for (unsigned int i = 0; i < core->num_of_threads; i++)
	{
		core->core_threads[i].return_cycle = 0;
		core->core_threads[i].rip = 0;

		// Initialize register file
		for (unsigned int j = 0; j < REGS_COUNT; j++)
		{
			core->core_threads[i].regs.reg[j] = 0;
		}
	}
}
void CORE_BlockedMT()
{
	// Init core:
	CORE_init(&core_blocked);
	unsigned int halt_count=0;
	unsigned int idle_count=0;	

	// Run simulation as long as at least one thread is not halted
	while (halt_count<core_blocked.num_of_threads)
	{
		// Get current thread by reference according to Round Robin index
		Thread &current_thread = core_blocked.core_threads[core_blocked.rr_index];		

		// If the current thread selected is available
		if (core_blocked.cycle_count >= current_thread.return_cycle)
		{			
			// Execute the instruction fetched
			EXECUTE_inst(idle_count,halt_count,current_thread,core_blocked);
			
		}
		else
		{
			// Thread is still working
			idle_count =0;

			// As long as we didn't finish a full Round Robin cycle
			while (idle_count <core_blocked.num_of_threads)
			{
				core_blocked.rr_index = (core_blocked.rr_index + 1) % (core_blocked.num_of_threads);
				Thread &temp_thread = core_blocked.core_threads[core_blocked.rr_index];		
				if (core_blocked.cycle_count >= temp_thread.return_cycle)
				{
					// Found available thread different from current
					core_blocked.cycle_count+=SIM_GetSwitchCycles();
					
					break;
				}
				idle_count++;
			}
			if(idle_count == core_blocked.num_of_threads)
			{
				core_blocked.cycle_count++;
			}
			
		}
	}
}

void EXECUTE_inst(unsigned int &idle_count, unsigned int &halt_count, Thread &current_thread, Core &core)
{
	// Zero idle counting
	idle_count = 0;
	Instruction inst;

	// Fetch instruction
	SIM_MemInstRead(current_thread.rip, &inst, core.rr_index);

	// Increment threads rip 
	current_thread.rip++;

	// Initialize addend for use with LOAD & STORE
	int addend = 0;
	switch (inst.opcode)
	{
	case CMD_NOP:
		break;
	case CMD_ADD:
		// (dst <- src1 + src2)
		current_thread.regs.reg[inst.dst_index] = current_thread.regs.reg[inst.src1_index] + current_thread.regs.reg[inst.src2_index_imm];
		break;
	case CMD_ADDI:
		// (dst <- src1 + imm)
		current_thread.regs.reg[inst.dst_index] = current_thread.regs.reg[inst.src1_index] + inst.src2_index_imm;
		break;
	case CMD_SUB:
		// (dst <- src1 - src2)
		current_thread.regs.reg[inst.dst_index] = current_thread.regs.reg[inst.src1_index] - current_thread.regs.reg[inst.src2_index_imm];
		break;
	case CMD_SUBI:
		// (dst <- src1 - imm)
		current_thread.regs.reg[inst.dst_index] = current_thread.regs.reg[inst.src1_index] - inst.src2_index_imm;
		break;
	case CMD_LOAD:
		// (dst <- Mem[src1 + src2])
		// Update addend according to src2_index_imm flag
		addend = inst.isSrc2Imm ? inst.src2_index_imm : current_thread.regs.reg[inst.src2_index_imm];

		// Read data from memory
		SIM_MemDataRead(current_thread.regs.reg[inst.src1_index] + addend, &current_thread.regs.reg[inst.dst_index]);

		// Update thread's return cycle according to LOAD latency
		current_thread.return_cycle = core.cycle_count + 1 + SIM_GetLoadLat();

		// The thread will be idle after LOAD
		idle_count++;
		break;
	case CMD_STORE:
		// (Mem[dst + src2] <- src1)
		// Update addend according to src2_index_imm flag
		addend = inst.isSrc2Imm ? inst.src2_index_imm : current_thread.regs.reg[inst.src2_index_imm];

		// Write data to memory
		SIM_MemDataWrite(current_thread.regs.reg[inst.dst_index] + addend, current_thread.regs.reg[inst.src1_index]);

		// Update thread's return cycle according to LOAD latency
		current_thread.return_cycle = core.cycle_count + 1 + SIM_GetStoreLat();

		// The thread will be idle after STORE
		idle_count++;
		break;
	case CMD_HALT:

		// Set return cycle to be infinity to indicate thread has halted
		current_thread.return_cycle = UINT32_MAX;

		// Increment halt count
		halt_count++;
		break;
	default:
		break;
	}

	// Increment cycles and inst. retired
	core.cycle_count++;
	core.inst_count++;
}
void CORE_FinegrainedMT()
{

	CORE_init(&core_finegrained);
	unsigned int halt_count=0;
	unsigned int idle_count=0;
	// Run simulation as long as at least one thread is not halted
	while (halt_count<core_finegrained.num_of_threads)
	{
		// Get current thread by reference according to Round Robin index
		Thread &current_thread = core_finegrained.core_threads[core_finegrained.rr_index];
		if (core_finegrained.cycle_count >= current_thread.return_cycle)
		{
			// Execute the instruction fetched
			EXECUTE_inst(idle_count,halt_count,current_thread,core_finegrained);
		}
		else
		{
			// The thread is not available, increment idle count
			idle_count++;

			// If we made a full round-robin cycle, then no threads were availabe. increment cycle
			if(idle_count >= core_finegrained.num_of_threads)
			{
				// Core was idle
				core_finegrained.cycle_count++;
				idle_count =0;
			}
		}
		// Increment cyclic Round Robin index
		core_finegrained.rr_index = (core_finegrained.rr_index + 1) % (core_finegrained.num_of_threads);
	}
}

double CORE_CPI(Core &core)
{
	// Calc CPI for given core
	double cpi = (double)core.cycle_count / (double)core.inst_count;

	// Release allocated threads array
	delete[] core.core_threads;
	return cpi;
}
double CORE_BlockedMT_CPI()
{
	// Return CPI of Core block
	return CORE_CPI(core_blocked);
}

double CORE_FinegrainedMT_CPI()
{
	// Return Finegrain of Core block
	return CORE_CPI(core_finegrained);
}

void CORE_BlockedMT_CTX(tcontext *context, int threadid)
{
	// Return register file of given thread ID
	context[threadid] = core_blocked.core_threads[threadid].regs;
}

void CORE_FinegrainedMT_CTX(tcontext *context, int threadid)
{
	// Return register file of given thread ID
	context[threadid] = core_finegrained.core_threads[threadid].regs;
}
