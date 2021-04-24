
#include "bp_api.h"

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
bool isGlobalHist, bool isGlobalTable, int Shared)
{

}

bool BP_predict(uint32_t pc, uint32_t *dst)
{

}
void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst)
{

}
void BP_GetStats(SIM_stats *curStats)
{
    
}